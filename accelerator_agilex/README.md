# `Accelerator`

## Prerequisites

| Optimized for                     | Description
|:---                               |:---
| OS                                | Ubuntu* 18.04 <br> Windows* 10
| Hardware                          | GEN9 or newer <br> Intel® Agilex®, Arria® 10, and Stratix® 10 FPGAs
| Software                          | Intel® oneAPI DPC++/C++ Compiler

> **Note**: Even though the Intel DPC++/C++ OneAPI compiler is enough to compile for CPU, GPU, FPGA emulation, generating FPGA reports and generating RTL for FPGAs, there are extra software requirements for the FPGA simulation flow and FPGA compiles.
>
> For using the simulator flow, Intel® Quartus® Prime Pro Edition and one of the following simulators must be installed and accessible through your PATH:
> - Questa*-Intel® FPGA Edition
> - Questa*-Intel® FPGA Starter Edition
> - ModelSim® SE
>
> When using the hardware compile flow, Intel® Quartus® Prime Pro Edition must be installed and accessible through your PATH.
> **Warning** Make sure you add the device files associated with the FPGA that you are targeting to your Intel® Quartus® Prime installation.

## Set Environment Variables

When working with the command-line interface (CLI), you should configure the oneAPI toolkits using environment variables. Set up your CLI environment by sourcing the `setvars` script every time you open a new terminal window. This practice ensures that your compiler, libraries, and tools are ready for development.

## Build the `Base: Vector Add` Sample

> **Note**: If you have not already done so, set up your CLI
> environment by sourcing  the `setvars` script in the root of your oneAPI installation.
>
> Linux*:
> - For system wide installations: `. /opt/intel/oneapi/setvars.sh`
> - For private installations: ` . ~/intel/oneapi/setvars.sh`
> - For non-POSIX shells, like csh, use the following command: `bash -c 'source <install-dir>/setvars.sh ; exec csh'`
>
> Windows*:
> - `C:\Program Files(x86)\Intel\oneAPI\setvars.bat`
> - Windows PowerShell*, use the following command: `cmd.exe "/K" '"C:\Program Files (x86)\Intel\oneAPI\setvars.bat" && powershell'`
>
> For more information on configuring environment variables, see [Use the setvars Script with Linux* or macOS*](https://www.intel.com/content/www/us/en/develop/documentation/oneapi-programming-guide/top/oneapi-development-environment-setup/use-the-setvars-script-with-linux-or-macos.html) or [Use the setvars Script with Windows*](https://www.intel.com/content/www/us/en/develop/documentation/oneapi-programming-guide/top/oneapi-development-environment-setup/use-the-setvars-script-with-windows.html).

### On Linux*

#### Configure the build system

1. Change to the sample directory.
2. 
   Configure the project to use the buffer-based implementation. This should already be included in the repo.
   ```
   mkdir build
   cd build
   cmake ..
   ```
   or

   Configure the project to use the Unified Shared Memory (USM) based implementation (unused in this repo since USM isn't on DevCloud. But useful to know).
   ```
   mkdir build
   cd build
   cmake .. -DUSM=1
   ```

   > **Note**: When building for FPGAs, the default FPGA family will be used (Intel® Agilex®).
   > You can change the default target by using the command:
   >  ```
   >  cmake .. -DFPGA_DEVICE=/opt/intel/oneapi/intel_a10gx_pac
   >  ``` 
   >
   > Alternatively, you can target an explicit FPGA board variant and BSP by using the following command (we don't use this): 
   >  ```
   >  cmake .. -DFPGA_DEVICE=<board-support-package>:<board-variant>
   >  ``` 
   >
   > You will only be able to run an executable on the FPGA if you specified a BSP.

#### Build for CPU and GPU
    
1. Build the program.
   ```
   make cpu-gpu
   ```   
2. Clean the program. (Optional)
   ```
   make clean
   ```

#### Build for FPGA

1. Compile for FPGA emulation.
   ```
   qsub -l nodes=1:fpga_compile:ppn=2 -d .
   make fpga_emu
   exit
   ```
2. Compile for simulation (fast compile time, targets simulator FPGA device):
   ```
   qsub -l nodes=1:fpga_compile:ppn=2 -d .
   make fpga_sim
   exit
   ```
3. Generate HTML performance reports.
   ```
   qsub -l nodes=1:fpga_compile:ppn=2 -d .
   make report
   exit
   ```
   The reports reside at `simple-add_report.prj/reports/report.html`.

4. Compile the program for FPGA hardware. (Compiling for hardware can take a long
time.)
   ```
   qsub -l nodes=1:fpga_compile:ppn=2 -d . -b build.sh
   make fpga
   watch -n 1 qstat -n -1 (this is to view build progress and status)
   ```

5. Clean the program. (Optional)
   ```
   make clean
   ```

#### Troubleshooting

If an error occurs, you can get more details by running `make` with
the `VERBOSE=1` argument:
```
make VERBOSE=1
```
If you receive an error message, troubleshoot the problem using the **Diagnostics Utility for Intel® oneAPI Toolkits**. The diagnostic utility provides configuration and system checks to help find missing dependencies, permissions errors, and other issues. See the [Diagnostics Utility for Intel® oneAPI Toolkits User Guide](https://www.intel.com/content/www/us/en/develop/documentation/diagnostic-utility-user-guide/top.html) for more information on using the utility.


## Run the program

### Configurable Parameters

The source files (`vector-add-buffers.cpp`).

### On Linux

#### Run for CPU and GPU

1. Change to the output directory.

2. Run the program for Unified Shared Memory (USM) and buffers.
    ```
    ./vector-add-buffers
    ./vector-add-usm
    ```

### Build and Run in Intel® DevCloud

When running a sample in the Intel® DevCloud, you must specify the compute node (CPU, GPU, FPGA) and whether to run in batch or interactive mode.

>**Note**: Since Intel® DevCloud for oneAPI includes the appropriate development environment already configured, you do not need to set environment variables.

Use the Linux instructions to build and run the program.

You can specify a GPU node using a single line script.

```
qsub  -I  -l nodes=1:gpu:ppn=2 -d .
```

- `-I` (upper case I) requests an interactive session.
- `-l nodes=1:gpu:ppn=2` (lower case L) assigns one full GPU node. 
- `-d .` makes the current folder as the working directory for the task.

  |Available Nodes           |Command Options
  |:---                      |:---
  |GPU	                    |`qsub -l nodes=1:gpu:ppn=2 -d .`
  |CPU	                    |`qsub -l nodes=1:xeon:ppn=2 -d .`
  |FPGA Compile Time         |`qsub -l nodes=1:fpga_compile:ppn=2 -d .`
  |FPGA Runtime (Arria 10)   |`qsub -l nodes=1:fpga_runtime:arria10:ppn=2 -d . -b run_100.sh`


>**Note**: For more information on how to specify compute nodes, read [Launch and manage jobs](https://devcloud.intel.com/oneapi/documentation/job-submission/) in the Intel® DevCloud for oneAPI Documentation.

Only `fpga_compile` nodes support compiling to FPGA. When compiling for FPGA hardware, increase the job timeout to **24 hours**.

Executing programs on FPGA hardware is only supported on `fpga_runtime` nodes of the appropriate type, such as `fpga_runtime:arria10, fpga_runtime:stratix10`.

Neither compiling nor executing programs on FPGA hardware are supported on the login nodes. For more information, see the Intel® DevCloud for oneAPI [*Intel® oneAPI Base Toolkit Get Started*](https://devcloud.intel.com/oneapi/get_started/) page.


## Example Output
```
Command: flip, input file: data_in_10k_10k.csv, output file: data_out_10k_10k_fpga.csv
Running on device: pac_s10 : Intel PAC Platform (pac_ee00000)
Reading data from '../in/data_in_10k_10k.csv'
Preforming data flip
Computation was 6637.53 milliseconds
Writing data to '../out/data_out_10k_10k_fpga.csv'
Computation and I/O was 18407.9 milliseconds
Vector add successfully completed on device.
```

## License

Code samples are licensed under the MIT license. See [License.txt](https://github.com/oneapi-src/oneAPI-samples/blob/master/License.txt) for details.

Third-party program Licenses can be found here: [third-party-programs.txt](https://github.com/oneapi-src/oneAPI-samples/blob/master/third-party-programs.txt).
