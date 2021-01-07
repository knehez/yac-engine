set(BINARY ${CMAKE_PROJECT_NAME})

include_directories(../lib/googletest/googletest/include ../lib/googletest/googletest/src)

file(GLOB_RECURSE SOURCES LIST_DIRECTORIES true *.h *.cpp)

set(SOURCES ${SOURCES})

# remove tests libs to avoid multiple main()
list(REMOVE_ITEM CONAN_LIBS gmock_main gmock gtest gmock_maind gmockd gtestd)

if (COVERALLS)
	coveralls_setup(
        "${SOURCES}" 
        ${COVERALLS_UPLOAD}                 
        "${PROJECT_SOURCE_DIR}/coveralls-cmake/cmake")
endif()

add_executable(${BINARY}_run ${SOURCES})

add_library(${BINARY}_lib STATIC ${SOURCES})

# extra warnings
#target_compile_options(${BINARY}_lib PRIVATE -Werror -Wall -Wextra)