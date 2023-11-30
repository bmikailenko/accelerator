#include <sycl/sycl.hpp>
#include <vector>
#include <iostream>
#include <string>
#if FPGA_HARDWARE || FPGA_EMULATOR || FPGA_SIMULATOR
#include <sycl/ext/intel/fpga_extensions.hpp>
#endif
#include "io.hpp"
#include "util.hpp"

#define ELEMENTS_PER_DDR_ACCESS 16

bool help = false;                      // If help message needs to print
constexpr int kMaxStringLen = 40;       // Max filename string legth
template <int N> class KernelCompute;   // Forward declare kernel name
size_t num_repetitions = 1;             // Times to repeat kernel outer loop
size_t inner_loops = 1;                 // Times to repeat kernel innter loop
using namespace sycl;                   // SYCL namespace

typedef std::vector<std::vector<int>> Vector2D;

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
    auto start_time_compute_verbose = std::chrono::high_resolution_clock::now();

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

    auto end_time_compute_verbose = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> process_time_compute_verbose(end_time_compute_verbose - start_time_compute_verbose);
    std::cout << "Computation was " << process_time_compute_verbose.count() << " milliseconds\n";

    for (size_t i = 0; i < height; i++) {
        for (size_t j = 0; j < width; j++) {
            b_vector[i][j] = b_interm[(i*width)+j];;
        }
    }
}

//************************************
// Demonstrate vector add both in sequential on CPU and in parallel on device.
//************************************
int main(int argc, char * argv[]) {
    char out_file_str_buffer[kMaxStringLen] = {0};
    char in_file_str_buffer[kMaxStringLen] = {0};
    Vector2D outdata;
    Vector2D indata;
    std::string outfilename = "";
    std::string infilename = "";
    std::string command = "";

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

    try {
        queue q(selector, exception_handler);

        std::cout << "Running on device: " << q.get_device().get_info < info::device::name > () << "\n";

        auto start_time = std::chrono::high_resolution_clock::now();

        passed &= ReadInputData(infilename, indata);
        if(!passed)
            std::terminate();

        if(command.compare("flip") == 0) {
            outdata = create_blank_2d_vector(indata);
            std::cout << "Preforming data flip\n";
            VectorFlip(q, indata, outdata);
        }

        passed &= WriteOutputData(outfilename, outdata);
        if(!passed)
            std::terminate();

        auto end_time = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> process_time(end_time - start_time);
            std::cout << "Computation and I/O was " << process_time.count() << " milliseconds\n";

    } catch (exception const & e) {
        std::cout << "An exception is caught for vector add.\n";
        std::terminate();
    }
    std::cout << "Vector add successfully completed on device.\n";
    return 0;
}
