cmake_minimum_required(VERSION 3.8)

project(tsl_hopscotch_map_tests)

add_executable(tsl_hopscotch_map_tests "main.cpp" 
                                       "custom_allocator_tests.cpp"
                                       "hopscotch_map_tests.cpp" 
                                       "hopscotch_set_tests.cpp" 
                                       "policy_tests.cpp")

target_compile_features(tsl_hopscotch_map_tests PRIVATE cxx_std_17)

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    target_compile_options(tsl_hopscotch_map_tests PRIVATE -Werror -Wall -Wextra -Wold-style-cast -DTSL_DEBUG -UNDEBUG)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    target_compile_options(tsl_hopscotch_map_tests PRIVATE /bigobj /WX /W3 /DTSL_DEBUG /UNDEBUG)
endif()

# Boost::unit_test_framework
set(Boost_USE_STATIC_LIBS ON)
find_package(Boost REQUIRED COMPONENTS unit_test_framework)
target_link_libraries(tsl_hopscotch_map_tests PRIVATE Boost::unit_test_framework)   

# tsl::hopscotch_map
add_subdirectory(../ ${CMAKE_CURRENT_BINARY_DIR}/tsl)
target_link_libraries(tsl_hopscotch_map_tests PRIVATE tsl::hopscotch_map)  
