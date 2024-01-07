cmake_minimum_required(VERSION 3.5)

set(VCPKG_PATH "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg" CACHE PATH "vcpkg folder")
set(VCPKG_BRANCH "master" CACHE STRING "vcpkg branch to checkout if not present")

find_package(Git QUIET)
if(NOT EXISTS "${VCPKG_PATH}")
    option(GIT_SUBMODULE "Check submodules during build" ON)
        message(STATUS "Submodule update")
        execute_process(COMMAND ${GIT_EXECUTABLE} clone -b "${VCPKG_BRANCH}" --single-branch  https://github.com/microsoft/vcpkg "${VCPKG_PATH}"
                        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                        )
        execute_process(COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/bootstrap-vcpkg.bat
                        WORKING_DIRECTORY ${VCPKG_PATH}
                        )
endif()

execute_process(COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/vcpkg install 
                        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/vcpkg
                        )

set(CMAKE_TOOLCHAIN_FILE "${VCPKG_PATH}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "Vcpkg toolchain file")


project(yace VERSION 1.0
             DESCRIPTION "Yet Another Chess Engine"
             LANGUAGES CXX)

find_package(GTest CONFIG REQUIRED)
find_package(Threads)
find_package(OpenMP)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_COMPILE_FEATURES EHsc)
set(CMAKE_CXX_EXTENSIONS OFF)

# Flags on Linux
if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread -fopenmp -lomp -I/opt/homebrew/Cellar/libomp/14.0.6/include -L/opt/homebrew/Cellar/libomp/14.0.6/lib -march=native")
        add_compile_options("$<$<CONFIG:DEBUG>:-g;-Wall;-Wpedantic;-fsanitize=address>")
        add_link_options("$<$<CONFIG:DEBUG>:-fsanitize=address>")
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fopenmp=libomp")
    else()
        message(FATAL_ERROR "*** Compiler has not been set for Linux ***")
    endif()

# Flags on MacOS
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -openmp -v -I/opt/homebrew/Cellar/libomp/14.0.6/include -L/opt/homebrew/Cellar/libomp/14.0.6/lib")
        # set(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} libomp.lib")
    else()
        message(FATAL_ERROR "*** Compiler has not been set for MacOS ***")
    endif()
endif()

if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "*** Please switch to x64 build! ***")
endif()

include_directories(src)
include(CTest)

add_subdirectory(src)
add_subdirectory(test)