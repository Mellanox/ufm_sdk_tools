#!/bin/bash

# Default build type is Release
BUILD_TYPE="Release"
ENABLE_TESTING="OFF"
CMAKE_ONLY="OFF"

# Print usage information
function print_help() {
    echo "Usage: build.sh [OPTION]"
    echo "Options:"
    echo "  -r [--release] Build in release mode (default)"
    echo "  -d [--debug]   Build in debug mode"
    echo "  -cmk [--cmkae_only] Run only Cmake."
    echo "  -t [--tests] Enable tests in the build"
    echo "  -h, --help   Display this help"
}

# Check for help option
for arg in "$@"; do
    if [[ "$arg" == "-h" || "$arg" == "--help" ]]; then
        print_help
        return
    fi
done

# Check for build type and testing options
for arg in "$@"; do
    case "$arg" in
        --debug|-d)
            echo "Build in debug mode : $arg"
            BUILD_TYPE="Debug"
            ;;
        --release|-r)
            echo "Build in release mode : $arg"
            BUILD_TYPE="Release"
            ;;
        --tests|-t)
            echo "Enable tests : $arg"
            ENABLE_TESTING="ON"
            ;;
        --cmake_only|-cmk)
            echo "Run cmake_only : $arg"
            CMAKE_ONLY="ON"
            ;;
        *)
            echo "Invalid option : $arg"
            print_help
            return
            ;;
    esac
done

# Create a build directory based on the build type
BUILD_DIR="build/$BUILD_TYPE"
mkdir -p "$BUILD_DIR"

# Run CMake to configure the build
cmake -S . -B "$BUILD_DIR" -G Ninja -DCMAKE_BUILD_TYPE="$BUILD_TYPE" -DENABLE_TESTING="$ENABLE_TESTING"


# Run Ninja to build the project, if cmake_only is not set
if [[ "$CMAKE_ONLY" == "OFF" ]]; then
    # Run Ninja to build the project
    cmake --build "$BUILD_DIR"
fi