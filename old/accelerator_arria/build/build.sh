#!/bin/bash
source /opt/intel/inteloneapi/setvars.sh
export QUARTUS_ROOTDIR_OVERRIDE=/glob/development-tools/versions/intelFPGA_pro/19.2/quartus
export PATH=/glob/intel-python/python2/bin:$PATH
make fpga
