cmake_minimum_required(VERSION 3.10)

# Set the project name and version
project(HttpClientExample VERSION 1.0)

# Find the required packages
find_package(cpprestsdk REQUIRED)
find_package(OpenSSL REQUIRED)

file(GLOB_RECURSE SOURCES "src/*.cpp")


add_library(http_client ${SOURCES})
target_link_libraries(http_client PRIVATE cpprest OpenSSL::SSL OpenSSL::Crypto pthread)
target_include_directories(http_client PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Include OpenSSL directories
#target_include_directories(ssl_client PRIVATE ${OPENSSL_INCLUDE_DIR})

# Link OpenSSL libraries
#target_link_libraries(ssl_client PRIVATE ${OPENSSL_LIBRARIES})
