# Macro to add Google Test executable and link libraries
macro(add_gtest TARGET_NAME SOURCE_FILE)
    if(ENABLE_TESTING)

        set(BOOST_ROOT /usr)
        find_package(Boost 1.82 REQUIRED COMPONENTS program_options)

        add_executable(${TARGET_NAME} ${SOURCE_FILE})
        target_link_libraries(${TARGET_NAME} ${Boost_LIBRARIES} gtest gtest_main ${ARGN})
        add_test(NAME ${TARGET_NAME} COMMAND ${TARGET_NAME})

    endif()
endmacro()
