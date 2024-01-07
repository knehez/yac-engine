enable_testing()

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/modules/")


# eliminate cxx_std_11 invlid flag problem
set_target_properties(GTest::gtest PROPERTIES INTERFACE_COMPILE_FEATURES "")
set_target_properties(GTest::gmock_main PROPERTIES INTERFACE_COMPILE_FEATURES "")
set_target_properties(GTest::gtest_main PROPERTIES INTERFACE_COMPILE_FEATURES "")
set_target_properties(GTest::gmock PROPERTIES INTERFACE_COMPILE_FEATURES "")

set(BINARY ${CMAKE_PROJECT_NAME}_test)

file(GLOB_RECURSE TEST_SOURCES LIST_DIRECTORIES false *.hpp *.cpp)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

set(SOURCES ${TEST_SOURCES})

add_executable(${BINARY} ${TEST_SOURCES})

add_test(NAME ${BINARY} COMMAND ${BINARY})

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    # gnu linux compiles needs X11 libs
    target_link_libraries(${BINARY} PUBLIC ${CMAKE_PROJECT_NAME}_lib GTest::gmock GTest::gtest GTest::gmock_main GTest::gtest_main X11 Xext)
else()
    target_link_libraries(${BINARY} PUBLIC ${CMAKE_PROJECT_NAME}_lib GTest::gmock GTest::gtest GTest::gmock_main GTest::gtest_main)
endif()