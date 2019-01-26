cmake_minimum_required(VERSION 3.1)

project(tsl_hopscotch_map VERSION 2.1.0)

add_library(hopscotch_map INTERFACE)
# Use tsl::hopscotch_map as target, more consistent with other libraries conventions (Boost, Qt, ...)
add_library(tsl::hopscotch_map ALIAS hopscotch_map)

target_include_directories(hopscotch_map INTERFACE
                           "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
                           "$<INSTALL_INTERFACE:include>")

list(APPEND headers "${CMAKE_CURRENT_SOURCE_DIR}/include/tsl/bhopscotch_map.h"
                    "${CMAKE_CURRENT_SOURCE_DIR}/include/tsl/bhopscotch_set.h"
                    "${CMAKE_CURRENT_SOURCE_DIR}/include/tsl/hopscotch_growth_policy.h"
                    "${CMAKE_CURRENT_SOURCE_DIR}/include/tsl/hopscotch_hash.h"
                    "${CMAKE_CURRENT_SOURCE_DIR}/include/tsl/hopscotch_map.h"
                    "${CMAKE_CURRENT_SOURCE_DIR}/include/tsl/hopscotch_set.h")
target_sources(hopscotch_map INTERFACE "$<BUILD_INTERFACE:${headers}>")

if(MSVC)
    target_sources(hopscotch_map INTERFACE
                   "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/tsl-hopscotch-map.natvis>"
                   "$<INSTALL_INTERFACE:tsl-hopscotch-map.natvis>")
endif()

# Only compatible with CMake version >= 3.8
if(${CMAKE_VERSION} VERSION_GREATER "3.7")
    target_compile_features(hopscotch_map INTERFACE cxx_std_11)
endif()




# Installation (only compatible with CMake version >= 3.3)
if(${CMAKE_VERSION} VERSION_GREATER "3.2")
    include(CMakePackageConfigHelpers)
    include(GNUInstallDirs)

    ## Install include directory and potential natvis file
    install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/include/tsl"
            DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")

    if(MSVC)
        install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/tsl-hopscotch-map.natvis"
                DESTINATION ".")
    endif()



    ## Create and install tsl_hopscotch_mapConfig.cmake
    configure_package_config_file("${CMAKE_CURRENT_SOURCE_DIR}/cmake/tsl_hopscotch_mapConfig.cmake.in"
                                  "${CMAKE_CURRENT_BINARY_DIR}/tsl_hopscotch_mapConfig.cmake"
                                  INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/tsl_hopscotch_map")

    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/tsl_hopscotch_mapConfig.cmake"
            DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/tsl_hopscotch_map")



    ## Create and install tsl_hopscotch_mapTargets.cmake
    install(TARGETS hopscotch_map
            EXPORT tsl_hopscotch_mapTargets)

    install(EXPORT tsl_hopscotch_mapTargets
            NAMESPACE tsl::
            DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/tsl_hopscotch_map")



    ## Create and install tsl_hopscotch_mapConfigVersion.cmake
    # tsl_hopscotch_map is header-only and does not depend on the architecture.
    # Remove CMAKE_SIZEOF_VOID_P from tsl_hopscotch_mapConfigVersion.cmake so that a
    # tsl_hopscotch_mapConfig.cmake generated for a 64 bit target can be used for 32 bit
    # targets and vice versa.
    set(TSL_HOPSOTCH_MAP_CMAKE_SIZEOF_VOID_P ${CMAKE_SIZEOF_VOID_P})
    unset(CMAKE_SIZEOF_VOID_P)
    write_basic_package_version_file("${CMAKE_CURRENT_BINARY_DIR}/tsl_hopscotch_mapConfigVersion.cmake"
                                     COMPATIBILITY SameMajorVersion)
    set(CMAKE_SIZEOF_VOID_P ${TSL_HOPSOTCH_MAP_CMAKE_SIZEOF_VOID_P})

    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/tsl_hopscotch_mapConfigVersion.cmake"
            DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/tsl_hopscotch_map")
endif()
