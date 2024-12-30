# Raylib
set(WIN10_SDK "C:/Program Files (x86)/Windows Kits/10/Lib/10.0.22621.0")

file(GLOB targets  "*.cpp")
foreach(target ${targets})
    get_filename_component(TARGET_NAME ${target} NAME)
    add_executable(${TARGET_NAME} ${target})
    set_target_properties(${TARGET_NAME} PROPERTIES
        FOLDER ${FOLDER_NAME}
    )

    target_include_directories(${TARGET_NAME} PRIVATE
        ${VCPKG_INCLUDE_DIR}
        ${ThirdParty}/raylib/include
    )
    target_link_libraries(${TARGET_NAME} PRIVATE
        ${ThirdParty}/raylib/lib/raylib.lib

        # Additional Windows dependencies
        ${WIN10_SDK}/um/x64/WinMM.Lib
        ${WIN10_SDK}/um/x64/Gdi32.Lib
    )
endforeach()
