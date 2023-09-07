#include <sycl/sycl.hpp>
#include <vector>
#include <iostream>
#include <string>
#if FPGA_HARDWARE || FPGA_EMULATOR || FPGA_SIMULATOR
#include <sycl/ext/intel/fpga_extensions.hpp>
#endif

// Determine if help message needs to print
bool help = false;

// Max filename string legth
constexpr int kMaxStringLen = 40;

using namespace sycl;

////////////////////////////////////////////////////////////////////////////////
// File I/O
bool ReadInputData(std::string infilename, std::vector < std::vector < int >> & indata);
bool WriteOutputData(std::string outfilename, std::vector < std::vector < int >> & outdata);
////////////////////////////////////////////////////////////////////////////////

// Forward declare the kernel name in the global scope.
// This FPGA best practice reduces name mangling in the optimization reports.
template <int N> class KernelCompute;

// num_repetitions: How many times to repeat the kernel invocation
size_t num_repetitions = 1;
size_t inner_loops = 1;

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

void VectorFlip(queue &q, const std::vector<std::vector<int>> &a_vector, std::vector<std::vector<int>> &b_vector) {
	size_t height = a_vector.size();
	size_t width = a_vector[0].size();
	std::vector<int> a_interm;
	std::vector<int> b_interm;

	for (size_t i = 0; i < height; i++) {
		for (size_t j = 0; j < width; j++) {
			a_interm.push_back(a_vector[i][j]);
		}
	}
	
	b_interm.resize(a_interm.size());
	buffer a_buf(a_interm);
	buffer b_buf(b_interm);
	auto start_time_compute_verbose = std::chrono::high_resolution_clock::now();
	for (size_t repetition = 0; repetition < num_repetitions; repetition++) {
		q.submit([ & ](handler & h) {
			accessor a(a_buf, h, read_only);
			accessor b(b_buf, h, write_only);
				h.parallel_for(height, [ = ](auto i) { // for each row
					for (size_t j = 0; j < width; j++) // row each column
						b[(i*width)+j] = a[(i*width)+(width-1-j)]; // flip
				});
		});
	}
	q.wait();
	
	auto end_time_compute_verbose = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double, std::milli> process_time_compute_verbose(end_time_compute_verbose - start_time_compute_verbose);
	std::cout << "Verbose computation was " << process_time_compute_verbose.count() << " milliseconds\n";

	for (size_t i = 0; i < height; i++) {
		for (size_t j = 0; j < width; j++) {
			b_vector[i][j] = b_interm[(i*width)+j];;
		}
	}
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
	std::cout << "  	flip                             : flip vectors  \n";
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

//************************************
// Demonstrate vector add both in sequential on CPU and in parallel on device.
//************************************
int main(int argc, char * argv[]) {
	char out_file_str_buffer[kMaxStringLen] = {0};
	char in_file_str_buffer[kMaxStringLen] = {0};
	std::vector<std::vector<int>> outdata;
	std::vector<std::vector<int>> indata;
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
			  << " [command] -i=<input-file> -o=<output-file> <number of loops>"
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

		// Print out the device information used for the kernel code.
		std::cout << "Running on device: " << q.get_device().get_info < info::device::name > () << "\n";

		auto start_time = std::chrono::high_resolution_clock::now();

		// Read in data
		passed &= ReadInputData(infilename, indata);
		if(!passed)
			std::terminate();

		auto start_time_compute = std::chrono::high_resolution_clock::now();

		if(command.compare("flip") == 0) {
			for(auto &row: indata) {
				std::vector<int> v;
				v.resize(row.size());
				outdata.push_back(v);
			}
			std::cout << "Preforming data flip\n";
			VectorFlip(q, indata, outdata);
		}

		auto end_time_compute = std::chrono::high_resolution_clock::now();
		std::chrono::duration<double, std::milli> process_time_compute(end_time_compute - start_time_compute);

		passed &= WriteOutputData(outfilename, outdata);
		if(!passed)
			std::terminate();

		auto end_time = std::chrono::high_resolution_clock::now();
		std::chrono::duration<double, std::milli> process_time(end_time - start_time);

    		std::cout << "Computation was " << process_time_compute.count() << " milliseconds\n";
    		std::cout << "Computation and I/O was " << process_time.count() << " milliseconds\n";
	} catch (exception const & e) {
		std::cout << "An exception is caught for vector add.\n";
		std::terminate();
	}

	std::cout << "Vector add successfully completed on device.\n";
	return 0;
}

bool ReadInputData(std::string infilename, std::vector<std::vector<int>> &indata) {
	std::string infilepath = "../in/" + infilename;
	std::ifstream infile_is;
	std::string line, val;

	std::cout << "Reading data from '" << infilepath << "'" << std::endl;

	infile_is.open(infilepath);
	if(infile_is.fail()) {
		std::cerr << "Failed to open '" << infilepath << "'" << std::endl;
		return false;
	}

	while(std::getline(infile_is, line)) {
		std::vector<int> v;
		std::stringstream s(line);
		while(getline(s, val, ','))
			v.push_back(std::stoi(val));
		indata.push_back(v);
	}

	infile_is.close();
	return true;
}

bool WriteOutputData(std::string outfilename, std::vector<std::vector<int>> &outdata) {
	std::string outfilepath = "../out/" + outfilename;
	std::ofstream outfile_os;
	std::string line, val;
	int i;

	std::cout << "Writing data to '" << outfilepath << "'" << std::endl;

	outfile_os.open(outfilepath);
	if(outfile_os.fail()) {
		std::cerr << "Failed to open '" << outfilepath << "'" << std::endl;
		return false;
	}

	for(auto &row: outdata) {
		if(row.size() == 1) {
			outfile_os << row[0] << '\n';
		} else {
			for(i = 0; i < (row.size() - 1); i++)
				outfile_os << row[i] << ',';
			outfile_os << row[i] << '\n';
		}
	}

	outfile_os.close();
	return true;
}
