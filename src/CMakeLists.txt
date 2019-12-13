set(BINARY ${CMAKE_PROJECT_NAME})

include_directories(../lib/googletest/googletest/include ../lib/googletest/googletest/src)

file(GLOB_RECURSE SOURCES LIST_DIRECTORIES true *.h *.cpp)

set(SOURCES ${SOURCES})

add_executable(${BINARY}_run ${SOURCES})

add_library(${BINARY}_lib STATIC ${SOURCES})