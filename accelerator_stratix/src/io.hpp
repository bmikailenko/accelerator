#include <sycl/sycl.hpp>
#include <vector>
#include <iostream>
#include <string>

////////////////////////////////////////////////////////////////////////////////
// File I/O
bool ReadInputData(std::string infilename, std::vector < std::vector < int >> & indata);
bool WriteOutputData(std::string outfilename, std::vector < std::vector < int >> & outdata);
////////////////////////////////////////////////////////////////////////////////

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
