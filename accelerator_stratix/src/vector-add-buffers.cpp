#include <sycl/sycl.hpp>
#include <vector>
#include <iostream>
#include <string>
#if FPGA_HARDWARE || FPGA_EMULATOR || FPGA_SIMULATOR
#include <sycl/ext/intel/fpga_extensions.hpp>
#endif
#include "io.hpp"
#include "util.hpp"

// DEFINITIONS //
#define ELEMENTS_PER_DDR_ACCESS 16
#ifdef __SYCL_DEVICE_ONLY__
  #define CL_CONSTANT __attribute__((opencl_constant))
#else
  #define CL_CONSTANT
#endif
#define PRINTF(format, ...) { \
            static const CL_CONSTANT char _format[] = format; \
            sycl::ext::oneapi::experimental::printf(_format, ## __VA_ARGS__); }

// TYPE DEFINITIONS  //
typedef std::vector<std::vector<int>> Vector2D;

// GLOBAL VARIABLES //
bool help = false;                      // If help message needs to print
constexpr int kMaxStringLen = 40;       // Max filename string legth
template <int N> class ProducerKernel;  // Forward declare kernel name
template <int N> class ConsumerKernel;  // Forward declare kernel name
size_t num_repetitions = 1;             // Times to repeat kernel outer loop
size_t inner_loops = 1;                 // Times to repeat kernel innter loop
using namespace sycl;                   // SYCL namespace

// PIPE DEFINITIONS
using ProducerToConsumerPipe = pipe<
    class ProducerConsumerPipe,
    size_t,
    1000>;

event Producer(queue &q, buffer<size_t, 1> &a_buf, size_t width, size_t height) {
    std::cout << "Starting producer...\n";

    size_t num_repetitions_local = num_repetitions;

    auto e = q.submit([&](handler &h) {

        accessor a(a_buf, h, read_only);

        h.single_task<class ProducerKernel<4>>(
            [=]() [[intel::kernel_args_restrict]] {

            size_t iters_per_row = (width / 16) + ((width % 16 == 0) ? 0 : 1);

            [[intel::loop_coalesce(4)]]
            for (size_t repetition = 0; repetition < num_repetitions_local; repetition++) {
                for (size_t i = 0; i < height; i++) { // for each row
                    for (size_t j = 0; j < iters_per_row; j++) {
                        #pragma unroll
                        for (size_t x = 0; x < ELEMENTS_PER_DDR_ACCESS; x++) {
                            size_t idx = j * ELEMENTS_PER_DDR_ACCESS + x;
                            ProducerToConsumerPipe::write(a[(i * width) + (width - 1) - idx]);
                        }
                    }
                }
            }
        });
    });

    return e;
}

event Consumer(queue &q, buffer<size_t, 1> &b_buf, size_t width, size_t height) {
    std::cout << "Starting consumer...\n";

    const size_t num_repetitions_local = num_repetitions;

    auto e = q.submit([&](handler &h) {

        accessor b(b_buf, h, write_only, no_init);

        h.single_task<class ConsumerKernel<4>>(
            [=]() [[intel::kernel_args_restrict]] {

            size_t iters_per_row = (width / 16) + ((width % 16 == 0) ? 0 : 1);

            [[intel::loop_coalesce(4)]]
            for (size_t repetition = 0; repetition < num_repetitions_local; repetition++) {
                for (size_t i = 0; i < height; i++) { // for each row
                    for (size_t j = 0; j < iters_per_row; j++) {
                        #pragma unroll
                        for (size_t x = 0; x < ELEMENTS_PER_DDR_ACCESS; x++) {
                            size_t idx = j * ELEMENTS_PER_DDR_ACCESS + x;
                            b[(i * width) + idx] = ProducerToConsumerPipe::read();
                        }
                    }
                }
            }
        });
    });

    return e;
}

/*
void VectorFlip(queue &q, const Vector2D &a_vector, Vector2D &b_vector) {
    std::vector<size_t> inner_loops_interm;
    size_t width = a_vector[0].size();
    size_t height = a_vector.size();
    std::vector<int> a_interm;
    std::vector<int> b_interm;

    for (size_t i = 0; i < height; i++) {
        for (size_t j = 0; j < width; j++) {
            a_interm.push_back(a_vector[i][j]);
        }
    }

    b_interm.resize(a_interm.size());
    buffer a_buf(a_interm, {property::buffer::mem_channel{1}});
    buffer b_buf(b_interm, {property::buffer::mem_channel{2}});
    size_t inner_loops_local = inner_loops;
    auto start_time_compute = std::chrono::high_resolution_clock::now();

    for (size_t repetition = 0; repetition < num_repetitions; repetition++) {
        q.submit([ & ](handler & h) {
            accessor a(a_buf, h, read_only);
            accessor b(b_buf, h, write_only, no_init);
            h.single_task<class KernelCompute<3>>(
                [=]() [[intel::kernel_args_restrict]] {
                size_t iters_per_row = (width / 16) + ((width % 16 == 0) ? 0 : 1);
                [[intel::loop_coalesce(3)]]
                for (size_t k = 0; k < inner_loops_local; k++) { // for n number of times
                    for (size_t i = 0; i < height; i++) { // for each row
                        for (size_t j = 0; j < iters_per_row; j++) {
                            #pragma unroll
                            for (size_t x = 0; x < ELEMENTS_PER_DDR_ACCESS; x++) {
                                int idx = j * ELEMENTS_PER_DDR_ACCESS + x;
                                if (idx < width) {
                                    b[(i * width) + idx] = a[(i * width) + (width - 1) - idx]; // flip
                                }
                            }
                        }
                    }
                }
            });
        });
    }
    q.wait();

    auto end_time_compute = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> process_time_compute(end_time_compute - start_time_compute);
    std::cout << "Computation was " << process_time_compute.count() << " milliseconds\n";

    for (size_t i = 0; i < height; i++) {
        for (size_t j = 0; j < width; j++) {
            b_vector[i][j] = b_interm[(i*width)+j];;
        }
    }
}
*/

//************************************
// Demonstrate vector add both in sequential on CPU and in parallel on device.
//************************************
int main(int argc, char * argv[]) {
    char out_file_str_buffer[kMaxStringLen] = {0};
    char in_file_str_buffer[kMaxStringLen] = {0};
    event producer_event, consumer_event;
    std::vector<size_t> outdata_flat;
    std::vector<size_t> indata_flat;
    std::string outfilename = "";
    std::string infilename = "";
    std::string command = "";
    Vector2D outdata;
    Vector2D indata;

    // Create device selector for the device of your interest.
    #if FPGA_EMULATOR
    // Intel extension: FPGA emulator selector on systems without FPGA card.
    auto selector = sycl::ext::intel::fpga_emulator_selector_v;
    #elif FPGA_SIMULATOR
    // Intel extension: FPGA simulator selector on systems without FPGA card.
    auto selector = sycl::ext::intel::fpga_simulator_selector_v;
    #elif FPGA_HARDWARE
    // Intel extension: FPGA selector on systems with FPGA card.
    auto selector = sycl::ext::intel::fpga_selector_v;
    #else
    // The default device selector will select the most performant device.
    auto selector = default_selector_v;
    #endif

    bool passed = true;

    // Argument processing
    if(argc != 6) {
        std::cerr << "Incorrect number of arguments. Correct usage: "
              << argv[0]
              << " [command] -i=<input file> -o=<output file> <# outer loops> <# inner loops>"
              << std::endl;
        return 1;
    }

    for(int i = 1; i < argc-2; i++) {
        if(argv[i][0] == '-') {
            std::string sarg(argv[i]);
            if(std::string(argv[i]) == "-h") {
                help = true;
            }
            if(std::string(argv[i]) == "--help") {
                help = true;
            }
            FindGetArgString(sarg, "-i=", in_file_str_buffer, kMaxStringLen);
            FindGetArgString(sarg, "-in=", in_file_str_buffer, kMaxStringLen);
            FindGetArgString(sarg, "--input-file=", in_file_str_buffer, kMaxStringLen);
            FindGetArgString(sarg, "-o=", out_file_str_buffer, kMaxStringLen);
            FindGetArgString(sarg, "-out=", out_file_str_buffer, kMaxStringLen);
            FindGetArgString(sarg, "--output-file=", out_file_str_buffer, kMaxStringLen);
        } else {
            command = std::string(argv[i]);
        }
    }

    if(help) {
        Help();
        return 1;
    }

    num_repetitions = atoi(argv[4]);
    inner_loops = atoi(argv[5]);
    infilename = std::string(in_file_str_buffer);
    outfilename = std::string(out_file_str_buffer);

    std::cout << "Command: " << command << ", input file: " << infilename << ", output file: " << outfilename << std::endl;

    auto start_time = std::chrono::high_resolution_clock::now();

    passed &= ReadInputData(infilename, indata);
    if(!passed)
        std::terminate();

    size_t width = indata[0].size();
    size_t height = indata.size();

    // Create 2d output vector
    outdata = create_blank_2d_vector(indata);

    // Flatten 2d vectors
    for (size_t i = 0; i < height; i++) {
        for (size_t j = 0; j < width; j++) {
            indata_flat.push_back(indata[i][j]);
        }
    }
    outdata_flat.resize(indata_flat.size());

    auto start_time_compute = std::chrono::high_resolution_clock::now();

    try {
        queue q(selector, exception_handler);
        std::cout << "Running on device: " << q.get_device().get_info < info::device::name > () << "\n";

        if(command.compare("flip") == 0) {

            // Create flat vector producer/consumer buffers
            buffer producer_buffer(indata_flat, {property::buffer::mem_channel{1}});
            buffer consumer_buffer(outdata_flat, {property::buffer::mem_channel{2}});

            // Run producer/consumer kernels
            producer_event = Producer(q, producer_buffer, width, height);
            consumer_event = Consumer(q, consumer_buffer, width, height);
        }

    } catch (exception const & e) {
        std::cout << "An exception is caught for vector add.\n";
        std::terminate();
    }

    auto end_time_compute = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> process_time_compute(end_time_compute -
                                                                           start_time_compute);
    std::cout << "Computation was " << process_time_compute.count() << " milliseconds\n";

    // Unflatten output data
    for (size_t i = 0; i < height; i++) {
        for (size_t j = 0; j < width; j++) {
            outdata[i][j] = outdata_flat[(i*width)+j];;
        }
    }

    passed &= WriteOutputData(outfilename, outdata);
    if(!passed)
        std::terminate();

    auto end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> process_time(end_time - start_time);
        std::cout << "Computation and I/O was " << process_time.count() << " milliseconds\n";

    std::cout << "Vector add successfully completed on device.\n";
    return 0;
}
