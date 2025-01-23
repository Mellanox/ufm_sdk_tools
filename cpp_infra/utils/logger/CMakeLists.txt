set (logger_sources
    src/Logger.cpp)

find_package(spdlog REQUIRED)

add_library (nvd_logger ${logger_sources})

target_include_directories(nvd_logger PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include/)

target_link_libraries(nvd_logger PUBLIC spdlog::spdlog)