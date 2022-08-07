set(BINARY ${CMAKE_PROJECT_NAME})
find_package(GTest CONFIG REQUIRED)
enable_testing()

file(GLOB_RECURSE SOURCES ${PROJECT_SOURCE_DIR}/src false *.h *.cpp)

file(GLOB_RECURSE TEST_SOURCES ${PROJECT_SOURCE_DIR}/src false *.h *.cpp)

# exclude main.cpp from testing (prevents duplicated main() problem)
list(FILTER TEST_SOURCES EXCLUDE REGEX ".*main.cpp$")
# create a lib for unit testing
add_library(${BINARY}_lib STATIC ${TEST_SOURCES})

add_executable(${BINARY} ${SOURCES})
# MESSAGE(FATAL "********************* ${VCPKG_INSTALLED_DIR} ***************")

find_path(GTEST_INCLUDE_DIR gtest.h)

target_include_directories(${BINARY} PRIVATE ${VCPKG_INSTALLED_DIR}/arm64-osx/include)
target_include_directories(${BINARY}_lib PRIVATE ${VCPKG_INSTALLED_DIR}/arm64-osx/include)


if(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
        target_compile_options(${BINARY} PRIVATE -openmp:llvm -openmp:experimental)
        target_compile_options(${BINARY} PRIVATE /fp:fast)
        target_compile_options(${BINARY} PRIVATE /arch:AVX2)
endif()    

if(CMAKE_CXX_COMPILER_ID MATCHES "CLANG")
        target_compile_options(${BINARY} PRIVATE -fp:fast)
        target_compile_options(${BINARY} PRIVATE -march:native)
endif() 

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
        target_link_libraries(${BINARY} PRIVATE X11 Xext)
        target_compile_options(${BINARY} PRIVATE -fopenmp)
endif()     


# extra warnings
#target_compile_options(${BINARY}_lib PRIVATE -Werror -Wall -Wextra)
