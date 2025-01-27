cmake_minimum_required(VERSION 3.10)

# Find the required packages
find_package(OpenSSL REQUIRED)
find_package(Boost REQUIRED COMPONENTS filesystem system)
find_package(fmt REQUIRED)

file(GLOB_RECURSE SOURCES "src/*.cpp")

# link with OpenSSL::SSL and OpenSSL::Crypto
# link also with cpprest for the current base64Encode using the ppt in cpprest lib
add_executable(pulse ${SOURCES})
target_link_libraries(pulse PRIVATE nvd_logger nvd_config http_client metrics nvd_os cpprest OpenSSL::SSL OpenSSL::Crypto pthread)
target_include_directories(pulse PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(pulse PRIVATE Boost::filesystem Boost::system fmt::fmt)

# Include OpenSSL directories
#target_include_directories(ssl_client PRIVATE ${OPENSSL_INCLUDE_DIR})

# Link OpenSSL libraries
#target_link_libraries(ssl_client PRIVATE ${OPENSSL_LIBRARIES})
