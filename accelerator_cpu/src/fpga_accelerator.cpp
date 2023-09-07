#include <iostream>

// oneAPI headers
#include <sycl/ext/intel/fpga_extensions.hpp>
#include <sycl/sycl.hpp>
#include "exception_handler.hpp"
#include "fast-cpp-csv-parser/csv.h"
#include "LoopbackTest.hpp"

// The type that will stream through the IO pipe. When using real IO pipes,
// make sure the width of this datatype matches the width of the IO pipe, which
// you can find in the BSP XML file.
using IOPipeType = int;

// Forward declare the kernel name in the global scope. This is an FPGA best
// practice that reduces name mangling in the optimization reports.
class AcceleratorID;

// Determine if help message needs to print
bool help = false;

// Max filename string legth
constexpr int kMaxStringLen = 40;

// DELETE THIS SOON
//constexpr int kVectSize = 256;

// Check if USM host allocations are enabled
/*
#if defined(USM_HOST_ALLOCATIONS)
constexpr bool kUseUSMHostAllocation = true;
#else
constexpr bool kUseUSMHostAllocation = false;
#endif
*/

////////////////////////////////////////////////////////////////////////////////
// File I/O
bool ReadInputData(std::string infilename, std::vector<std::vector<int>> &indata);
bool WriteOutputData(std::string outfilename, std::vector<std::vector<int>> &outdata);
////////////////////////////////////////////////////////////////////////////////

struct VectorFlip
{
	int *const a_in;
	int *const b_out;
	int len;

	void operator()() const
	{
		int a_idx;
		for (int b_idx = 0; b_idx < len; b_idx++)
		{
			a_idx = len - b_idx - 1;
			int a_val = a_in[a_idx];
			b_out[b_idx] = a_val;
		}
	}
};

struct VectorAdd
{
	int *const a_in;
	int *const b_in;
	int *const c_out;
	int len;

	void operator()() const
	{
		for (int idx = 0; idx < len; idx++)
		{
			int a_val = a_in[idx];
			int b_val = b_in[idx];
			int sum = a_val + b_val;
			c_out[idx] = sum;
		}
	}
};

struct VectorSubtract
{
	int *const a_in;
	int *const b_in;
	int *const c_out;
	int len;

	void operator()() const
	{
		for (int idx = 0; idx < len; idx++)
		{
			int a_val = a_in[idx];
			int b_val = b_in[idx];
			int sum = a_val - b_val;
			c_out[idx] = sum;
		}
	}
};

struct VectorMultiply
{
	int *const a_in;
	int *const b_in;
	int *const c_out;
	int len;

	void operator()() const
	{
		for (int idx = 0; idx < len; idx++)
		{
			int a_val = a_in[idx];
			int b_val = b_in[idx];
			int sum = a_val * b_val;
			c_out[idx] = sum;
		}
	}
};

struct VectorCrop
{
	int *const a_in;
	int *const b_out;
	int len;

	void operator()() const
	{
		for (int idx = 0; idx < len; idx++)
		{
			int val = a_in[idx];
			b_out[idx] = val;
		}
	}
};


void Help(void)
{
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

bool FindGetArg(std::string &arg, const char *str, int defaultval, int *val)
{
	std::size_t found = arg.find(str, 0, strlen(str));
	if (found != std::string::npos)
	{
		int value = atoi(&arg.c_str()[strlen(str)]);
		*val = value;
		return true;
	}
	return false;
}

bool FindGetArgString(std::string &arg, const char *str, char *str_value, size_t maxchars)
{
	std::size_t found = arg.find(str, 0, strlen(str));
	if (found != std::string::npos)
	{
		const char *sptr = &arg.c_str()[strlen(str)];
		for (int i = 0; i < maxchars - 1; i++)
		{
			char ch = sptr[i];
			switch (ch)
			{
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

int main(int argc, char *argv[])
{
	char out_file_str_buffer[kMaxStringLen] = {0};
	char in_file_str_buffer[kMaxStringLen] = {0};
	std::vector<std::vector<int>> outdata;
	std::vector<std::vector<int>> indata;
	std::string outfilename = "";
	std::string infilename = "";
	std::string command = "";
/*
#if defined(FPGA_EMULATOR)
	size_t count = 1 << 12;
#elif defined(FPGA_SIMULATOR)
	size_t count = 1 << 5;
#else
	size_t count = 1 << 24;
#endif
*/
	bool passed = true;

	// Check the number of arguments specified
	if (argc != 4)
	{
		std::cerr << "Incorrect number of arguments. Correct usage: "
			  << argv[0]
			  << " [command] -i=<input-file> -o=<output-file>"
			  << std::endl;
		return 1;
	}

	for (int i = 1; i < argc; i++)
	{
		if (argv[i][0] == '-')
		{
			std::string sarg(argv[i]);
			if (std::string(argv[i]) == "-h")
			{
				help = true;
			}
			if (std::string(argv[i]) == "--help")
			{
				help = true;
			}

			FindGetArgString(sarg, "-i=", in_file_str_buffer, kMaxStringLen);
			FindGetArgString(sarg, "-in=", in_file_str_buffer, kMaxStringLen);
			FindGetArgString(sarg, "--input-file=", in_file_str_buffer, kMaxStringLen);
			FindGetArgString(sarg, "-o=", out_file_str_buffer, kMaxStringLen);
			FindGetArgString(sarg, "-out=", out_file_str_buffer, kMaxStringLen);
			FindGetArgString(sarg, "--output-file=", out_file_str_buffer, kMaxStringLen);
		} 
		else
		{
			command = std::string(argv[i]);
		}
	}

	if (help)
	{
		Help();
		return 1;
	}

	infilename = std::string(in_file_str_buffer);
	outfilename = std::string(out_file_str_buffer);
	std::cout << "Command: " << command
		  << ", input file: " << infilename
		  << ", output file: " << outfilename
		  << std::endl;

	try {
		// Use compile-time macros to select either:
		//  - the FPGA emulator device (CPU emulation of the FPGA)
		//  - the FPGA device (a real FPGA)
		//  - the simulator device
#if FPGA_SIMULATOR
		auto selector = sycl::ext::intel::fpga_simulator_selector_v;
#elif FPGA_HARDWARE
		auto selector = sycl::ext::intel::fpga_selector_v;
#else	// #if FPGA_EMULATOR
		auto selector = sycl::ext::intel::fpga_emulator_selector_v;
#endif

		// queue properties to enable SYCL profiling of kernels
		auto prop_list = property_list{property::queue::enable_profiling()};

		// create the device queue
		queue q(selector, fpga_tools::exception_handler, prop_list);

		auto device = q.get_device();

		// make sure the device supports USM host allocations
		/*
		if (!device.has(sycl::aspect::usm_host_allocations))
		{
			std::cerr << "This design must either target a board that supports USM "
			"Host/Shared allocations, or IP Component Authoring. " <<
			std::endl;
			std::terminate();
		}
		*/

		std::cout << "Running on device: "
			  << device.get_info<sycl::info::device::name > ().c_str()
			  << std::endl;

		auto start_time = std::chrono::high_resolution_clock::now();

		passed &= ReadInputData(infilename, indata);
		if (!passed)
			std::terminate();

		std::cout << "Finished reading data\n";

		if (command.compare("flip") == 0)
		{
			std::cout << "Performing vector flip\n";
			int kVectSize;
			for (auto &row: indata)
			{
				std::vector<int> v;

				kVectSize = row.size();
				int *a = sycl::malloc_shared<int>(kVectSize, q);
				int *b = sycl::malloc_shared<int>(kVectSize, q);

				for (int i = 0; i < kVectSize; i++)
				{
					a[i] = row[i];
				}

				q.single_task<AcceleratorID> (VectorFlip{a, b, kVectSize}).wait();

				for (int i = 0; i < kVectSize; i++)
					v.push_back(b[i]);
				outdata.push_back(v);

				free(a, q);
				free(b, q);
			}
		}
		
		passed &= WriteOutputData(outfilename, outdata);
		if (!passed)
			std::terminate();

		auto end_time = std::chrono::high_resolution_clock::now();
		std::chrono::duration<double, std::milli> process_time(end_time - start_time);
    		std::cout << "Program took " << process_time.count() << " milliseconds\n";

//		passed &= RunLoopbackSystem<IOPipeType, kUseUSMHostAllocation>(q, count);

		// DELETE THIS SOON
/*
		// declare arrays and fill them
		// allocate in shared memory so the kernel can see them
		int *a = sycl::malloc_shared<int> (kVectSize, q);
		int *b = sycl::malloc_shared<int> (kVectSize, q);
		for (int i = 0; i < kVectSize; i++)
		{
			a[i] = i;
		}

		std::cout << "flip vector of size " << kVectSize << std::endl;

		q.single_task<AcceleratorID> (VectorFlip
		{
			a, b, kVectSize }).wait();

		// verify that VC is correct
		passed = true;
		for (int i = 0; i < kVectSize; i++)
		{
			int expected = a[kVectSize - i];
			if (b[i] != expected)
			{
				std::cout << "idx=" << i << ": result " << b[i] << ", expected (" <<
					expected << ") A=" << a[kVectSize - i] << std::endl;
				passed = false;
			}
		}

		std::cout << (passed ? "PASSED" : "FAILED") << std::endl;

		sycl::free(a, q);
		sycl::free(b, q);
*/
	}
	catch (sycl::exception
		const &e)
	{
		// Catches exceptions in the host code.
		std::cerr << "Caught a SYCL host exception:\n" << e.what() << "\n";

		// Most likely the runtime couldn't find FPGA hardware!
		if (e.code().value() == CL_DEVICE_NOT_FOUND)
		{
			std::cerr << "If you are targeting an FPGA, please ensure that your "
			"system has a correctly configured FPGA board.\n";
			std::cerr << "Run sys_check in the oneAPI root directory to verify.\n";
			std::cerr << "If you are targeting the FPGA emulator, compile with "
			"-DFPGA_EMULATOR.\n";
		}

		std::terminate();
	}

	std::cout << (passed ? "PASSED" : "FAILED") << std::endl;
	return passed ? EXIT_SUCCESS : EXIT_FAILURE;
}

bool ReadInputData(std::string infilename, std::vector<std::vector<int>> &indata)
{
	std::string infilepath = "../in/" + infilename;
	std::string line, val;

	std::cout << "Reading data from '" << infilepath << "'" << std::endl;

	std::ifstream infile_is;
	infile_is.open(infilepath);
	if(infile_is.fail())
	{
		std::cerr << "Failed to open '" << infilepath << "'" << std::endl;
		return false;
	}

	while(std::getline(infile_is, line))
	{
		std::vector<int> v;
		std::stringstream s (line);
		while (getline(s, val, ','))
			v.push_back(std::stoi(val));
		indata.push_back(v);
	}

	infile_is.close();

	return true;
}


bool WriteOutputData(std::string outfilename, std::vector<std::vector<int>> &outdata)
{
	std::string outfilepath = "../out/" + outfilename;
	std::string line, val;
	int i;

	std::cout << "Writing data to '" << outfilepath << "'" << std::endl;

	std::ofstream outfile_os;
	outfile_os.open(outfilepath);
	if(outfile_os.fail())
	{
		std::cerr << "Failed to open '" << outfilepath << "'" << std::endl;
		return false;
	}

	for (auto &row : outdata)
	{
		if (row.size() == 1)
		{
			outfile_os << row[0] << '\n';
		}
		else
		{
			for (i = 0; i < (row.size() - 1); i++)
				outfile_os << row[i] << ',';
			outfile_os << row[i] << '\n';
		}
	}

	outfile_os.close();

	return true;
}
