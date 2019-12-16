cmake_minimum_required(VERSION 3.5)
project(yace VERSION 1.0
             DESCRIPTION "Yet Another Chess Engine"
             LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_COMPILE_FEATURES EHsc)

if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "*** Please switch to x64 build! ***")
endif()

include_directories(src)

add_subdirectory(src)
add_subdirectory(test)
add_subdirectory(lib/googletest)