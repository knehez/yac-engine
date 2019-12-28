cmake_minimum_required(VERSION 3.5)
project(yace VERSION 1.0
             DESCRIPTION "Yet Another Chess Engine"
             LANGUAGES CXX)

find_package (Threads)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_COMPILE_FEATURES EHsc)

option(COVERALLS "Turn on coveralls support" OFF)
option(COVERALLS_UPLOAD "Upload the generated coveralls json" ON)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} 
	${PROJECT_SOURCE_DIR}/coveralls-cmake/cmake)

if (COVERALLS)
	include(Coveralls)
	coveralls_turn_on_coverage()
endif()

if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "*** Please switch to x64 build! ***")
endif()

include_directories(src)
include(CTest)

enable_testing()

add_subdirectory(src)
add_subdirectory(test)
add_subdirectory(lib/googletest)