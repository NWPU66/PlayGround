# GoogleTest
enable_testing()
find_package(GTest CONFIG REQUIRED)

file(GLOB targets  "*.cpp")
foreach(target ${targets})
    get_filename_component(TARGET_NAME ${target} NAME)
    add_executable(${TARGET_NAME} ${target})
    set_target_properties(${TARGET_NAME} PROPERTIES
        FOLDER ${FOLDER_NAME}
    )

    target_include_directories(${TARGET_NAME} PRIVATE
    )
    target_link_libraries(${TARGET_NAME} PRIVATE
        # GTest::gtest 
        GTest::gtest_main 
        # GTest::gmock 
        # GTest::gmock_main
    )

    add_test(AllTestsInMain main)
endforeach()
