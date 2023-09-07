# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.16

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:


#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:


# Remove some rules from gmake that .SUFFIXES does not remove.
SUFFIXES =

.SUFFIXES: .hpux_make_needs_suffix_list


# Suppress display of executed commands.
$(VERBOSE).SILENT:


# A target that is always out of date.
cmake_force:

.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /usr/bin/cmake

# The command to remove a file.
RM = /usr/bin/cmake -E remove -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/u192003/oneAPI-samples/DirectProgramming/C++SYCL/DenseLinearAlgebra/accelerator_cpu

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/u192003/oneAPI-samples/DirectProgramming/C++SYCL/DenseLinearAlgebra/accelerator_cpu/build

# Include any dependencies generated for this target.
include src/CMakeFiles/vector-add-buffers.fpga_sim.dir/depend.make

# Include the progress variables for this target.
include src/CMakeFiles/vector-add-buffers.fpga_sim.dir/progress.make

# Include the compile flags for this target's objects.
include src/CMakeFiles/vector-add-buffers.fpga_sim.dir/flags.make

src/CMakeFiles/vector-add-buffers.fpga_sim.dir/vector-add-buffers.cpp.o: src/CMakeFiles/vector-add-buffers.fpga_sim.dir/flags.make
src/CMakeFiles/vector-add-buffers.fpga_sim.dir/vector-add-buffers.cpp.o: ../src/vector-add-buffers.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/u192003/oneAPI-samples/DirectProgramming/C++SYCL/DenseLinearAlgebra/accelerator_cpu/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object src/CMakeFiles/vector-add-buffers.fpga_sim.dir/vector-add-buffers.cpp.o"
	cd /home/u192003/oneAPI-samples/DirectProgramming/C++SYCL/DenseLinearAlgebra/accelerator_cpu/build/src && /glob/development-tools/versions/oneapi/2023.1.2/oneapi/compiler/2023.1.0/linux/bin/icpx  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/vector-add-buffers.fpga_sim.dir/vector-add-buffers.cpp.o -c /home/u192003/oneAPI-samples/DirectProgramming/C++SYCL/DenseLinearAlgebra/accelerator_cpu/src/vector-add-buffers.cpp

src/CMakeFiles/vector-add-buffers.fpga_sim.dir/vector-add-buffers.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/vector-add-buffers.fpga_sim.dir/vector-add-buffers.cpp.i"
	cd /home/u192003/oneAPI-samples/DirectProgramming/C++SYCL/DenseLinearAlgebra/accelerator_cpu/build/src && /glob/development-tools/versions/oneapi/2023.1.2/oneapi/compiler/2023.1.0/linux/bin/icpx $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/u192003/oneAPI-samples/DirectProgramming/C++SYCL/DenseLinearAlgebra/accelerator_cpu/src/vector-add-buffers.cpp > CMakeFiles/vector-add-buffers.fpga_sim.dir/vector-add-buffers.cpp.i

src/CMakeFiles/vector-add-buffers.fpga_sim.dir/vector-add-buffers.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/vector-add-buffers.fpga_sim.dir/vector-add-buffers.cpp.s"
	cd /home/u192003/oneAPI-samples/DirectProgramming/C++SYCL/DenseLinearAlgebra/accelerator_cpu/build/src && /glob/development-tools/versions/oneapi/2023.1.2/oneapi/compiler/2023.1.0/linux/bin/icpx $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/u192003/oneAPI-samples/DirectProgramming/C++SYCL/DenseLinearAlgebra/accelerator_cpu/src/vector-add-buffers.cpp -o CMakeFiles/vector-add-buffers.fpga_sim.dir/vector-add-buffers.cpp.s

# Object files for target vector-add-buffers.fpga_sim
vector__add__buffers_fpga_sim_OBJECTS = \
"CMakeFiles/vector-add-buffers.fpga_sim.dir/vector-add-buffers.cpp.o"

# External object files for target vector-add-buffers.fpga_sim
vector__add__buffers_fpga_sim_EXTERNAL_OBJECTS =

vector-add-buffers.fpga_sim: src/CMakeFiles/vector-add-buffers.fpga_sim.dir/vector-add-buffers.cpp.o
vector-add-buffers.fpga_sim: src/CMakeFiles/vector-add-buffers.fpga_sim.dir/build.make
vector-add-buffers.fpga_sim: src/CMakeFiles/vector-add-buffers.fpga_sim.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/u192003/oneAPI-samples/DirectProgramming/C++SYCL/DenseLinearAlgebra/accelerator_cpu/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX executable ../vector-add-buffers.fpga_sim"
	cd /home/u192003/oneAPI-samples/DirectProgramming/C++SYCL/DenseLinearAlgebra/accelerator_cpu/build/src && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/vector-add-buffers.fpga_sim.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
src/CMakeFiles/vector-add-buffers.fpga_sim.dir/build: vector-add-buffers.fpga_sim

.PHONY : src/CMakeFiles/vector-add-buffers.fpga_sim.dir/build

src/CMakeFiles/vector-add-buffers.fpga_sim.dir/clean:
	cd /home/u192003/oneAPI-samples/DirectProgramming/C++SYCL/DenseLinearAlgebra/accelerator_cpu/build/src && $(CMAKE_COMMAND) -P CMakeFiles/vector-add-buffers.fpga_sim.dir/cmake_clean.cmake
.PHONY : src/CMakeFiles/vector-add-buffers.fpga_sim.dir/clean

src/CMakeFiles/vector-add-buffers.fpga_sim.dir/depend:
	cd /home/u192003/oneAPI-samples/DirectProgramming/C++SYCL/DenseLinearAlgebra/accelerator_cpu/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/u192003/oneAPI-samples/DirectProgramming/C++SYCL/DenseLinearAlgebra/accelerator_cpu /home/u192003/oneAPI-samples/DirectProgramming/C++SYCL/DenseLinearAlgebra/accelerator_cpu/src /home/u192003/oneAPI-samples/DirectProgramming/C++SYCL/DenseLinearAlgebra/accelerator_cpu/build /home/u192003/oneAPI-samples/DirectProgramming/C++SYCL/DenseLinearAlgebra/accelerator_cpu/build/src /home/u192003/oneAPI-samples/DirectProgramming/C++SYCL/DenseLinearAlgebra/accelerator_cpu/build/src/CMakeFiles/vector-add-buffers.fpga_sim.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : src/CMakeFiles/vector-add-buffers.fpga_sim.dir/depend

