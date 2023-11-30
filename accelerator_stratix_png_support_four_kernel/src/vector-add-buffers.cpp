#include <sycl/sycl.hpp>
#include <vector>
#include <iostream>
#include <string>
#include <cmath>
#include <png.h>
#if FPGA_HARDWARE || FPGA_EMULATOR || FPGA_SIMULATOR
#include <sycl/ext/intel/fpga_extensions.hpp>
#endif

#include "io.hpp"
#include "util.hpp"
#include "PngImage.hpp"

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

// GLOBAL VARIABLES //
bool help = false;                      // If help message needs to print
constexpr int kMaxStringLen = 40;       // Max filename string legth
template <int N> class ProducerKernel1;  // Forward declare kernel name
template <int N> class ProducerKernel2;  // Forward declare kernel name
template <int N> class ConsumerKernel1;  // Forward declare kernel name
template <int N> class ConsumerKernel2;  // Forward declare kernel name
size_t num_repetitions = 1;             // Times to repeat kernel outer loop

// PIPE DEFINITIONS
using ProducerToConsumerPipe1 = sycl::ext::intel::pipe<
    class ProducerConsumerPipe1,
    uint64_t,
    1000>;
using ProducerToConsumerPipe2 = sycl::ext::intel::pipe<
    class ProducerConsumerPipe2,
    uint64_t,
    1000>;

sycl::event Producer1(sycl::queue &q, sycl::buffer<uint64_t, 1> &a_buf, size_t width, size_t height) {

    auto e = q.submit([&](sycl::handler &h) {

        sycl::accessor a(a_buf, h, sycl::read_only);

        h.single_task<class ProducerKernel1<3>>(
            [=]() [[intel::kernel_args_restrict]] {

            size_t iters_per_row = (width / 16) + ((width % 16 == 0) ? 0 : 1);

            [[intel::loop_coalesce(3)]]
            for (size_t i = 0; i < height; i++) { // for each row
                for (size_t j = 0; j < iters_per_row; j++) {
                    #pragma unroll
                    for (size_t x = 0; x < ELEMENTS_PER_DDR_ACCESS; x++) {
                        size_t idx = j * ELEMENTS_PER_DDR_ACCESS + x;
                        ProducerToConsumerPipe1::write(a[(i * width) + (width - 1) - idx]);
                    }
                }
            }
        });
    });

    return e;
}

sycl::event Producer2(sycl::queue &q, sycl::buffer<uint64_t, 1> &a_buf, size_t width, size_t height) {

    auto e = q.submit([&](sycl::handler &h) {

        sycl::accessor a(a_buf, h, sycl::read_only);

        h.single_task<class ProducerKernel2<3>>(
            [=]() [[intel::kernel_args_restrict]] {

            size_t iters_per_row = (width / 16) + ((width % 16 == 0) ? 0 : 1);

            [[intel::loop_coalesce(3)]]
            for (size_t i = 0; i < height; i++) { // for each row
                for (size_t j = 0; j < iters_per_row; j++) {
                    #pragma unroll
                    for (size_t x = 0; x < ELEMENTS_PER_DDR_ACCESS; x++) {
                        size_t idx = j * ELEMENTS_PER_DDR_ACCESS + x;
                        ProducerToConsumerPipe2::write(a[(i * width) + (width - 1) - idx]);
                    }
                }
            }
        });
    });

    return e;
}

sycl::event Consumer1(sycl::queue &q, sycl::buffer<uint64_t, 1> &b_buf, size_t width, size_t height) {

    auto e = q.submit([&](sycl::handler &h) {

        sycl::accessor b(b_buf, h, sycl::write_only, sycl::no_init);

        h.single_task<class ConsumerKernel1<3>>(
            [=]() [[intel::kernel_args_restrict]] {

            size_t iters_per_row = (width / 16) + ((width % 16 == 0) ? 0 : 1);

            [[intel::loop_coalesce(3)]]
                for (size_t i = 0; i < height; i++) { // for each row
                    for (size_t j = 0; j < iters_per_row; j++) {
                        #pragma unroll
                        for (size_t x = 0; x < ELEMENTS_PER_DDR_ACCESS; x++) {
                            size_t idx = j * ELEMENTS_PER_DDR_ACCESS + x;
                            b[(i * width) + idx] = ProducerToConsumerPipe1::read();
                        }
                    }
                }
        });
    });

    return e;
}

sycl::event Consumer2(sycl::queue &q, sycl::buffer<uint64_t, 1> &b_buf, size_t width, size_t height) {

    auto e = q.submit([&](sycl::handler &h) {

        sycl::accessor b(b_buf, h, sycl::write_only, sycl::no_init);

        h.single_task<class ConsumerKernel2<3>>(
            [=]() [[intel::kernel_args_restrict]] {

            size_t iters_per_row = (width / 16) + ((width % 16 == 0) ? 0 : 1);

            [[intel::loop_coalesce(3)]]
                for (size_t i = 0; i < height; i++) { // for each row
                    for (size_t j = 0; j < iters_per_row; j++) {
                        #pragma unroll
                        for (size_t x = 0; x < ELEMENTS_PER_DDR_ACCESS; x++) {
                            size_t idx = j * ELEMENTS_PER_DDR_ACCESS + x;
                            b[(i * width) + idx] = ProducerToConsumerPipe2::read();
                        }
                    }
                }
        });
    });

    return e;
}

int main(int argc, char * argv[]) {
    std::vector<img::PNG_PIXEL_RGBA<uint16_t>> outdata_flat;
    std::vector<img::PNG_PIXEL_RGBA<uint16_t>> indata_flat;
    std::vector<uint64_t> outdata_flat1, outdata_flat2;
    std::vector<uint64_t> indata_flat1, indata_flat2;
    char out_file_str_buffer[kMaxStringLen] = {0};
    char in_file_str_buffer[kMaxStringLen] = {0};
    sycl::event producer_event1, producer_event2;
    sycl::event consumer_event1, consumer_event2;
    img::PNG_PIXEL_RGBA_16_ROWS outdata;
    img::PNG_PIXEL_RGBA_16_ROWS indata;
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

    // Argument processing
    if(argc != 5) {
        std::cerr << "Incorrect number of arguments. Correct usage: "
              << argv[0]
              << " [command] -i=<input file> -o=<output file> <# times to perform command>"
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

    // Save parsed arguments
    num_repetitions = atoi(argv[4]);
    infilename = std::string(in_file_str_buffer);
    outfilename = std::string(out_file_str_buffer);

    // Start overall time
    std::cout << "Command: " << command << ", input file: " << infilename << ", output file: " << outfilename << std::endl;

    auto start_time = std::chrono::high_resolution_clock::now();

    // PNG Input
    img::PNG png(std::filesystem::path("../in/" + infilename));
    indata = png.asRGBA16();
    size_t width = indata[0].size();
    size_t height = indata.size();

    // Create 2d output vector
    outdata = create_blank_2d_vector(indata);

    // Flatten 2d vectors
    for (size_t i = 0; i < height/2; i++) {
        for (size_t j = 0; j < width; j++) {
            indata_flat1.push_back(static_cast<uint64_t>(indata[i][j]));
        }
    }
    for (size_t i = height/2; i < height; i++) {
        for (size_t j = 0; j < width; j++) {
            indata_flat2.push_back(static_cast<uint64_t>(indata[i][j]));
        }
    }
    outdata_flat1.resize(indata_flat1.size());
    outdata_flat2.resize(indata_flat2.size());

    for (size_t i = 0; i < height; i++) {
        for (size_t j = 0; j < width; j++) {
            indata_flat.push_back(indata[i][j]);
        }
    }
    outdata_flat.resize(indata_flat.size());

    // Start computation time
    auto start_time_compute = std::chrono::high_resolution_clock::now();

    try {
        sycl::queue q(selector, exception_handler);
        std::cout << "Running on device: " << q.get_device().get_info < sycl::info::device::name > () << "\n";

        if(command.compare("flip") == 0) {

            // Create flat vector producer/consumer buffers
            sycl::buffer producer_buffer1(indata_flat1, {sycl::property::buffer::mem_channel{1}});
            sycl::buffer producer_buffer2(indata_flat2, {sycl::property::buffer::mem_channel{2}});
            sycl::buffer consumer_buffer1(outdata_flat1, {sycl::property::buffer::mem_channel{3}});
            sycl::buffer consumer_buffer2(outdata_flat2, {sycl::property::buffer::mem_channel{4}});

            for (size_t repetition = 0; repetition < num_repetitions; repetition++) {
                // Run producer/consumer kernels
                producer_event1 = Producer1(q, producer_buffer1, width, indata_flat1.size()/width);
                producer_event2 = Producer2(q, producer_buffer2, width, indata_flat2.size()/width);
                consumer_event1 = Consumer1(q, consumer_buffer1, width, indata_flat1.size()/width);
                consumer_event2 = Consumer2(q, consumer_buffer2, width, indata_flat2.size()/width);
            }
        }

    } catch (std::exception const & e) {
        std::cout << "An exception is caught for vector add.\n";
        std::terminate();
    }

    // End computation time
    auto end_time_compute = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> process_time_compute(end_time_compute -
                                                                           start_time_compute);
    std::cout << "Computation was " << process_time_compute.count() << " milliseconds\n";

    // Unflatten top half of output data
    for (size_t i = 0; i < height/2; i++) {
        for (size_t j = 0; j < width; j++) {
            uint64_t val = outdata_flat1[(i*width)+j];

            // Convert uint64_t to PNG_PIXEL_RGBA
            img::PNG_PIXEL_RGBA<uint16_t> tmp;
            uint16_t r = (uint16_t)(val >> 48);
            uint16_t g = (uint16_t)(val >> 32) & 0xFFFF;
            uint16_t b = (uint16_t)(val >> 16) & 0xFFFF;
            uint16_t a = (uint16_t)(val & 0xFFFF);
            tmp.data[0] = r;
            tmp.data[1] = g;
            tmp.data[2] = b;
            tmp.data[3] = a;
            tmp.rgba.r = r;
            tmp.rgba.g = g;
            tmp.rgba.b = b;
            tmp.rgba.a = a;

            outdata[i][j] = tmp;
        }
    }

    // Unflatten bottom half of output data
    for (size_t i = height/2; i < height; i++) {
        for (size_t j = 0; j < width; j++) {
            uint64_t val = outdata_flat2[((i-(height/2))*width)+j];

            // Convert uint64_t to PNG_PIXEL_RGBA
            img::PNG_PIXEL_RGBA<uint16_t> tmp;
            uint16_t r = (uint16_t)(val >> 48);
            uint16_t g = (uint16_t)(val >> 32) & 0xFFFF;
            uint16_t b = (uint16_t)(val >> 16) & 0xFFFF;
            uint16_t a = (uint16_t)(val & 0xFFFF);
            tmp.data[0] = r;
            tmp.data[1] = g;
            tmp.data[2] = b;
            tmp.data[3] = a;
            tmp.rgba.r = r;
            tmp.rgba.g = g;
            tmp.rgba.b = b;
            tmp.rgba.a = a;

            outdata[i][j] = tmp;
        }
    }

    // PNG Output
    png.fromRGBA16(outdata);
    png.saveToFile(std::filesystem::path("../out/" + outfilename));

    // End overall time
    auto end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> process_time(end_time - start_time);
        std::cout << "Computation and I/O was " << process_time.count() << " milliseconds\n";

    std::cout << "Vector add successfully completed on device.\n";
    return 0;
}
