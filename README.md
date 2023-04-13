# biolib
Library containing useful building blocks for bioinformatics software

#### Table of contents
* [Compiling the Code](#compiling-the-code)
* [Dependencies](#dependencies)
* [Authors](#authors)

Compiling the Code
------------------

The code is tested on Linux (gcc) and macOS (clang, Apple Silicon only).
To build the code, [`CMake`](https://cmake.org/) is required.

Clone the repository with

    git clone https://github.com/yhhshb/biolib.git

For a testing environment, use the following:

    mkdir debug_build
    cd debug_build
    cmake .. -D CMAKE_BUILD_TYPE=Debug -D BIOLIB_USE_SANITIZERS=On -D MAKE_TESTS=On
    make -j
