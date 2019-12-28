set(BINARY ${CMAKE_PROJECT_NAME}_test)

find_package (Threads)
file(GLOB_RECURSE TEST_SOURCES LIST_DIRECTORIES false *.h *.cpp)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
set(SOURCES ${TEST_SOURCES})

add_executable(${BINARY} ${TEST_SOURCES})

add_test(NAME ${BINARY} COMMAND ${BINARY})

target_link_libraries(${BINARY} PUBLIC ${CMAKE_PROJECT_NAME}_lib gtest)
target_link_libraries(${BINARY} ${CMAKE_THREAD_LIBS_INIT})
