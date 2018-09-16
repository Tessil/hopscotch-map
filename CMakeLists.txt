cmake_minimum_required(VERSION 3.1)

project(tsl_hopscotch_map)

add_library(tsl_hopscotch_map INTERFACE)
# Use tsl::hopscotch_map as target, more consistent with other libraries conventions (Boost, Qt, ...)
add_library(tsl::hopscotch_map ALIAS tsl_hopscotch_map)

target_include_directories(tsl_hopscotch_map INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_sources(tsl_hopscotch_map INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/include/tsl/bhopscotch_map.h"
                                           "${CMAKE_CURRENT_SOURCE_DIR}/include/tsl/bhopscotch_set.h"
                                           "${CMAKE_CURRENT_SOURCE_DIR}/include/tsl/hopscotch_growth_policy.h"
                                           "${CMAKE_CURRENT_SOURCE_DIR}/include/tsl/hopscotch_hash.h"
                                           "${CMAKE_CURRENT_SOURCE_DIR}/include/tsl/hopscotch_map.h"
                                           "${CMAKE_CURRENT_SOURCE_DIR}/include/tsl/hopscotch_set.h")

if(MSVC)
    target_sources(tsl_hopscotch_map INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/tsl_hopscotch_map.natvis")
endif()

if(${CMAKE_VERSION} VERSION_GREATER "3.7") 
    # Only available since version 3.8
    target_compile_features(tsl_hopscotch_map INTERFACE cxx_std_11)
endif()
