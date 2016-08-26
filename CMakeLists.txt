cmake_minimum_required(VERSION 3.5)
project(test_hopscotch_map)
enable_testing()

find_package(Boost 1.55.0 REQUIRED COMPONENTS unit_test_framework) 

# set(CMAKE_CXX_FLAGS  "-std=c++11 -Werror -Wall -Wextra -Wconversion -g --coverage")
set(CMAKE_CXX_FLAGS  "-std=c++11 -Werror -Wall -Wextra -Wconversion -O3")


add_executable("test_hopscotch_map" "tests/main.cpp")
target_include_directories("test_hopscotch_map" PRIVATE "${Boost_INCLUDE_DIRS}" "src") 
target_link_libraries("test_hopscotch_map" "${Boost_LIBRARIES}")

add_test(NAME "all_tests" COMMAND "test_hopscotch_map")
