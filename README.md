# Yet Another Chess Engine

Simple 64-bit bitboard based chess engine. Developed in TDD style.

### GTest installation
**Manual installation steps:**

    cd lib
    git clone https://github.com/google/googletest.git
    git checkout tags/release-1.10.0

**Usage**

    cmake .
    cd build/test
    yace-test

## Development tools
**Bitboard calculator**

    http://cinnamonchess.altervista.org/bitboard_calculator/Calc.html

**Code Coverage Analysis** 

To generate code coverage analysis reports, setup and use OpenCppCoverage.

Setup from here

    https://github.com/OpenCppCoverage/OpenCppCoverage
    
Usage

    opencppcoverage --sources src -- yace_test.exe

