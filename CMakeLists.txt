cmake_minimum_required(VERSION 3.10)

project(my_project)

# message(STATUS "Source directory: ${CMAKE_SOURCE_DIR}")
# message("${CMAKE_SOURCE_FILE}")

link_directories(${CMAKE_SOURCE_DIR}/lib/gtest/build/lib)

include_directories( BEFORE
${CMAKE_SOURCE_DIR}/include
${CMAKE_SOURCE_DIR}/lib/gtest/googletest/include)

add_executable(my_test src/test.cpp test/utest.cpp)

#  add_compile_options(-fPIE)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIE -pie")

target_link_libraries(my_test gtest gtest_main)

