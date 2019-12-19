# Yet Another Chess Engine (YAC-Engine)

[![Travis build status](https://api.travis-ci.com/knehez/yac-engine.svg?branch=master)](https://travis-ci.com/knehez/yac-engine)

Simple 64-bit bitboard based chess engine written in C++. Developed in TDD style.

See .travis.yml for installation details.

# Installation steps

**1.) Clone repository**

**2.) Google test manual installation steps:**

    mkdir lib
    cd lib
    git clone https://github.com/google/googletest.git
    cd googletest
    git checkout tags/release-1.10.0

**3.) Build from source and launch tests**

    cd ../../
    mkdir build
    cd build
    cmake ..
    make
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
