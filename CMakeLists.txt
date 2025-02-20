cmake_minimum_required(VERSION 3.10)

project(my_project)

link_directories(${CMAKE_SOURCE_DIR}/lib/gtest/build/lib)


add_executable(my_test src/test.cpp src/test/utest.cpp)

include_directories(my_test PRIVATE ${CMAKE_SOURCE_FILE}/src
        ${CMAKE_SOURCE_FILE}/lib/gtest/googletest/include)

#  add_compile_options(-fPIE)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIE -pie")

target_link_libraries(my_test gtest gtest_main)

