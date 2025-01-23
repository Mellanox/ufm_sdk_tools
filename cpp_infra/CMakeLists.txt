project(cpp_rest)

cmake_minimum_required(VERSION 3.10)

set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/libs)
set (CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/libs)

# this is temporary - should use conan pr other package manager!
set(CMAKE_PREFIX_PATH "${CMAKE_PREFIX_PATH};${CMAKE_SOURCE_DIR}/3rdparty/spdlog/build")
find_package(spdlog REQUIRED)

add_compile_options(
    $<$<COMPILE_LANGUAGE:CXX>:-Werror>
    $<$<COMPILE_LANGUAGE:CXX>:-Wconversion>
    $<$<COMPILE_LANGUAGE:CXX>:-Wshadow>
    $<$<COMPILE_LANGUAGE:CXX>:-Wwrite-strings>
)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")

    #-G "MinGW Makefiles
    add_compile_definitions(_DEBUG)
    set (CMAKE_CXX_FLAGS "-g3 -Wall -O0")

elseif(CMAKE_BUILD_TYPE STREQUAL "Release")

    add_compile_definitions(_RELEASE)
    set (CMAKE_CXX_FLAGS "-O3 -Wall -pedantic -DNDEBUG")

endif()

# Set C++ standard globally
set(CMAKE_CXX_STANDARD 20)

add_subdirectory(utils)
add_subdirectory(tools)
add_subdirectory(http_client)
#add_subdirectory(rest_client)
