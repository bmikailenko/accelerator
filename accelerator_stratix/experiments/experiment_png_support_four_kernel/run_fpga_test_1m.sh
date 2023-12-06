# Warm up the FPGA
./vector-add-buffers.fpga flip -in=test3.png -out=test3_out.png 100

# Tests
./vector-add-buffers.fpga flip -in=test3.png -out=test3_out.png 1000000
./vector-add-buffers.fpga flip -in=test3.png -out=test3_out.png 1000000
./vector-add-buffers.fpga flip -in=test3.png -out=test3_out.png 1000000
