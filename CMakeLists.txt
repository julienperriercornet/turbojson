cmake_minimum_required( VERSION 3.1 )

project(
    libturbojson
    VERSION 0.1
    LANGUAGES CXX)

option(AVX2 "Build with Avx2 optimisations" OFF)
    
include(CTest)

if (AVX2)
add_definitions(-mavx2)
add_definitions(-DAVX2)
message( "AVX2 enabled" )
else()
message( "AVX2 disabled" )
endif()

set(
    SOURCE_FILES
    aligned_string.h
    turbojson.h
    platform.h)

add_library( turbojson STATIC ${SOURCE_FILES} )

add_subdirectory(sample)

if (${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_CURRENT_SOURCE_DIR})
add_subdirectory(test)
endif()

enable_testing()

