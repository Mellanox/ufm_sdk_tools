
#file(GLOB_RECURSE SOURCES "src/*.cpp")

#add_library(metrics ${SOURCES})

add_library(metrics INTERFACE)

target_include_directories(metrics INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include/)

target_link_libraries(metrics INTERFACE nvd_logger nvd_os)