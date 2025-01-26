
file(GLOB_RECURSE SOURCES "src/*.cpp")

add_library (nvd_os ${SOURCES} ${Boost_LIBRARIES})

target_include_directories(nvd_os PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include/)

target_link_libraries(nvd_os PUBLIC nvd_logger nvd_errh)