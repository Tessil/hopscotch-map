cmake_minimum_required(VERSION 3.5)
project(test_hopscotch_map)
enable_testing()

find_package(Boost 1.55.0 REQUIRED COMPONENTS unit_test_framework) 

if(CMAKE_COMPILER_IS_GNUCXX)
    #set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -std=c++11 -Werror -Wall -Wextra -g --coverage")
    set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -std=c++11 -Werror -Wall -Wextra -O3")
endif()


set(TEST_EXECUTABLE "test_hopscotch_map")



add_executable("${TEST_EXECUTABLE}" "tests/main.cpp")
target_include_directories("${TEST_EXECUTABLE}" PRIVATE "${Boost_INCLUDE_DIRS}" "src") 
target_link_libraries("${TEST_EXECUTABLE}" "${Boost_LIBRARIES}")

add_test(NAME "all_tests" COMMAND "${TEST_EXECUTABLE}")
