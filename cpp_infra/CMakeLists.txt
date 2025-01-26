cmake_minimum_required(VERSION 3.10)

# include custom macros file
include(cmake/Macros.cmake)

project(ufm_cpp)

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

include_directories(${Boost_INCLUDE_DIRS})
link_directories(${Boost_LIBRARY_DIRS})

# read the ENABLE_TESTING option to toggle test mode (default is OFF)
option(ENABLE_TESTING "Enable testing mode with GoogleTest" OFF)

# Add test-specific includes if ENABLE_TEST is ON
if(ENABLE_TESTING)
    message(STATUS "Test mode enabled, adding GoogleTest paths")
    message("Test mode enabled, adding GoogleTest paths")

    # set(GTEST_LIBRARY "${PROJECT_SOURCE_DIR}/3rdparty/googletest/build/lib/libgtest.a")
    # set(GTEST_MAIN_LIBRARY "${PROJECT_SOURCE_DIR}/3rdparty/googletest/build/lib/libgtest_main.a")
    
    # Add GoogleMock include path
    # include_directories(${PROJECT_SOURCE_DIR}/3rdparty/googletest/googlemock/include)
    # include_directories(${PROJECT_SOURCE_DIR}/3rdparty/googletest/googletest/include)

    # link_directories(${PROJECT_SOURCE_DIR}/3rdparty/googletest/build/lib/)

    # Add additional settings for tests if needed
    # e.g., enable testing framework, add test sources, etc.
    enable_testing()

    # todo - ENABLE_METRICS has small performance penalty. We do not want it in release.
    #        consider add build configuration for tests.
    add_compile_definitions(ENABLE_METRICS)

    # Find and add GoogleTest/GoogleMock
    #add_subdirectory(${PROJECT_SOURCE_DIR}/3rdparty/googletest)

    # Example test target
    # add_executable(runTests test/test_main.cpp)
    # target_link_libraries(runTests gmock gtest)
endif()

add_subdirectory(utils)
add_subdirectory(tools)
add_subdirectory(http_client)
#add_subdirectory(rest_client)
