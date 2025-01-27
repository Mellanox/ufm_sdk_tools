file(GLOB_RECURSE SOURCES "src/*.cpp")

find_package(Boost REQUIRED COMPONENTS program_options)

add_library (nvd_config ${SOURCES})

#${Boost_LIBRARIES}
target_link_libraries(nvd_config PUBLIC nvd_logger Boost::program_options)

target_include_directories(nvd_config PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include/)

#add_subdirectory(test)