#include <sycl/sycl.hpp>
#include <vector>
#include <iostream>
#include <string>
#if FPGA_HARDWARE || FPGA_EMULATOR || FPGA_SIMULATOR
#include <sycl/ext/intel/fpga_extensions.hpp>
#endif

#include "PngImage.hpp"

// Determine if help message needs to print
bool help = false;

// Max filename string legth
constexpr int kMaxStringLen = 40;

using namespace sycl;

// Forward declare the kernel name in the global scope.
// This FPGA best practice reduces name mangling in the optimization reports.
template <int N> class KernelCompute;

// num_repetitions: How many times to repeat the kernel invocation
size_t num_repetitions = 1;

// Vector type and data size for this example.
size_t vector_size = 10000;

typedef std::vector < int > IntVector;

// Create an exception handler for asynchronous SYCL exceptions
static auto exception_handler = [](sycl::exception_list e_list) {
    for(std::exception_ptr
        const & e: e_list) {
        try {
            std::rethrow_exception(e);
        } catch (std::exception
            const & e) {
            #if _DEBUG
            std::cout << "Failure" << std::endl;
            #endif
            std::terminate();
        }
    }
};

void VectorFlip(queue &q, const std::vector<uint64_t> &a, std::vector<uint64_t> &b, const size_t width, const size_t height) {
    buffer a_buf(a);
    buffer b_buf(b);
    auto start_time_compute_verbose = std::chrono::high_resolution_clock::now();
    for (size_t repetition = 0; repetition < num_repetitions; repetition++) {
        q.submit([ & ](handler & h) {
            accessor a(a_buf, h, read_only);
            accessor b(b_buf, h, write_only);
            h.parallel_for(height, [ = ](auto i) { // for each row
                for (size_t j = 0; j < width; j++) // row each column
                {
                    b[(i*width)+j] = a[(i*width)+(width-1-j)]; // flip
                }
            });
        });
    };
    q.wait();

    auto end_time_compute_verbose = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> process_time_compute_verbose(end_time_compute_verbose - start_time_compute_verbose);
    std::cout << "Verbose computation was " << process_time_compute_verbose.count() << " milliseconds\n";
}

//************************************
// Initialize the vector from 0 to vector_size - 1
//************************************
void InitializeVector(IntVector & a) {
    for(size_t i = 0; i < a.size(); i++) a.at(i) = i;
}

void Help(void) {
    // Command line arguments.
    // accelerator [command] -i=[input file] -o=[output file]
    // -h, --help
    // future options?
    // -p,performance : output perf metrics
    std::cout << "accelerator [command] -i=<input file> -o=<output file>\n";
    std::cout << "  -h,--help                                : this help text\n";
    std::cout << "  [command]                                                \n";
    std::cout << "      flip                             : flip vectors  \n";
}

bool FindGetArg(std::string & arg,
    const char * str, int defaultval, int * val) {
    std::size_t found = arg.find(str, 0, strlen(str));
    if(found != std::string::npos) {
        int value = atoi( & arg.c_str()[strlen(str)]);* val = value;
        return true;
    }
    return false;
}

bool FindGetArgString(std::string & arg,
    const char * str, char * str_value, size_t maxchars) {
    std::size_t found = arg.find(str, 0, strlen(str));
    if(found != std::string::npos) {
        const char * sptr = & arg.c_str()[strlen(str)];
        for(int i = 0; i < maxchars - 1; i++) {
            char ch = sptr[i];
            switch(ch) {
                case ' ':
                case '\t':
                case '\0':
                    str_value[i] = 0;
                    return true;
                    break;
                default:
                    str_value[i] = ch;
                    break;
            }
        }
        return true;
    }
    return false;
}

img::PNG_PIXEL_RGBA_16_ROWS create_blank_2d_vector(img::PNG_PIXEL_RGBA_16_ROWS template_vector) {
    img::PNG_PIXEL_RGBA_16_ROWS ret;

    unsigned int height = template_vector.size();
    unsigned int width  = template_vector[0].size();

    ret.reserve(height);

    for(unsigned int i = 0; i < height; i++) {
        img::PNG_PIXEL_RGBA_16_ROW row;
        row.resize(width);
        ret.push_back(row);
    }

    return ret;
}

//************************************
// Demonstrate vector add both in sequential on CPU and in parallel on device.
//************************************
int main(int argc, char * argv[]) {
    std::vector<std::vector<uint64_t>> indata_vec, outdata_vec;
    std::vector<uint64_t> indata_vec_flat, outdata_vec_flat;
    char out_file_str_buffer[kMaxStringLen] = {0};
    char in_file_str_buffer[kMaxStringLen] = {0};
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
              << " [command] -i=<input-file> -o=<output-file> <# repetitions> <# threads>"
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
    infilename = std::string(in_file_str_buffer);
    outfilename = std::string(out_file_str_buffer);
    std::cout << "Command: " << command << ", input file: " << infilename << ", output file: " << outfilename << std::endl;

    auto start_time = std::chrono::high_resolution_clock::now();

    // PNG Input
    img::PNG png(std::filesystem::path("../in/" + infilename));
    indata = png.asRGBA16();
    size_t width = indata[0].size();
    size_t height = indata.size();

    outdata = create_blank_2d_vector(indata);

    // Create 2d uint64_t vectors from PNG data
    for (size_t i = 0; i < height; i++) {
        std::vector<uint64_t> new_row;

        for (size_t j = 0; j < width; j++) {
            new_row.push_back(static_cast<uint64_t>(indata[i][j]));
        }

        indata_vec.push_back(new_row);
    }
    for (auto row : indata_vec)
    {
        std::vector<uint64_t> new_row;
        new_row.resize(row.size());
        outdata_vec.push_back(new_row);
    }

    // Flatten the 2d vectors
    for (size_t i = 0; i < height; i++) {
        for (size_t j = 0; j < width; j++) {
            indata_vec_flat.push_back(indata_vec[i][j]);
        }
    }
    outdata_vec_flat.resize(indata_vec_flat.size());

    try {
        queue q(selector, exception_handler);

        // Print out the device information used for the kernel code.
        std::cout << "Running on device: " << q.get_device().get_info < info::device::name > () << "\n";

        auto start_time_compute = std::chrono::high_resolution_clock::now();

        if(command.compare("flip") == 0) {
            std::cout << "Preforming data flip\n";
            VectorFlip(q, indata_vec_flat, outdata_vec_flat, width, height);
        }

        auto end_time_compute = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> process_time_compute(end_time_compute - start_time_compute);
        std::cout << "Computation was " << process_time_compute.count() << " milliseconds\n";

    } catch (exception const & e) {
        std::cout << "An exception is caught for vector add.\n";
        std::terminate();
    }
    std::cout << "W: " << width << " H: " << height << " oudata_vec_flat size: " << outdata_vec_flat.size() << std::endl;
    std::cout << "Outdata size: " << outdata.size() << std::endl;
    // Convert uint64_t data to PNG output data
    for (size_t i = 0; i < height; i++) {
        for (size_t j = 0; j < width; j++) {
            uint64_t val = outdata_vec_flat[(i*width)+j];

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
    png.saveToFile(std::filesystem::path("../out/test.png"));

    auto end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> process_time(end_time - start_time);

    std::cout << "Computation and I/O was " << process_time.count() << " milliseconds\n";

    std::cout << "Vector add successfully completed on device.\n";
    return 0;
}
