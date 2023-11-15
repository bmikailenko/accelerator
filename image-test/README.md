Steps to install ImageMagick locally:
    - cd image-test
    - qsub -I -l nodes=1:fpga_compile:ppn=2 -d .
    - git clone https://github.com/ImageMagick/ImageMagick.git
    - cd ImageMagick
    - ./configure --prefix=/home/<User home path>
        - User home path is a path to your directory (ex: u192003)
    - make
    - make install
    - export PKG_CONFIG_PATH=/home/<User home path>/lib/pkgconfig
        - User home path is a path to your directory (ex: u192003)
    - export LD_LIBRARY_PATH=/home/<User home path>/lib
        - User home path is a path to your directory (ex: u192003)

How to compile a CPP file with ImageMagick:
    - c++ `Magick++-config --cxxflags --cppflags` -O2 -o main main.cpp   `Magick++-config --ldflags --libs`
