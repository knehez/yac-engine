cmake_minimum_required(VERSION 3.10)
project(yace)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_COMPILE_FEATURES EHsc)

include_directories(src)

add_subdirectory(src)
add_subdirectory(test)
add_subdirectory(lib/googletest)