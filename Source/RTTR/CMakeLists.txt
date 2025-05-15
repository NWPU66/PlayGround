# RTTR
find_package(rttr CONFIG REQUIRED)

file(GLOB targets  "*.cpp")
foreach(target ${targets})
    get_filename_component(TARGET_NAME ${target} NAME)
    add_executable(${TARGET_NAME} ${target})
    set_target_properties(${TARGET_NAME} PROPERTIES
        FOLDER ${FOLDER_NAME}
    )

    target_include_directories(${TARGET_NAME} PRIVATE
        ${VCPKG_INCLUDE_DIR}
    )
    target_link_libraries(${TARGET_NAME} PRIVATE
        RTTR::Core
    )
endforeach()
