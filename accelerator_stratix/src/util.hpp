#include <sycl/sycl.hpp>
#include <vector>
#include <iostream>
#include <string>

using namespace sycl;			// SYCL namespace
typedef std::vector<std::vector<int>> Vector2D;

////////////////////////////////////////////////////////////////////////////////
// UTILS
void Help(void);
bool FindGetArg(std::string &arg, const char *str, int defaultval, int *val);
bool FindGetArgString(std::string &arg, const char *str, char *str_value, size_t maxchars);
Vector2D create_blank_2d_vector(Vector2D template_vector);
////////////////////////////////////////////////////////////////////////////////

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

Vector2D create_blank_2d_vector(Vector2D template_vector) {
	Vector2D ret;
	for (auto &t: template_vector) { // For each vector in the template
		std::vector<int> v;	
		v.resize(t.size());	 // Create a vector of same size
		ret.push_back(v);
	}
	return ret;
}
