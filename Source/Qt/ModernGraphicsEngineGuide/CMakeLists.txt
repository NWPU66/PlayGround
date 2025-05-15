file(GLOB targets  "*.cpp")
foreach(target ${targets})
    get_filename_component(PROJECT_NAME ${target} NAME_WE)
    add_executable(${PROJECT_NAME} ${target})
    set_target_properties(${PROJECT_NAME} PROPERTIES
        FOLDER ${FOLDER_NAME}
    )

    target_include_directories(${PROJECT_NAME} PRIVATE
        E:/Qt/6.8.1/msvc2022_64/include
        ${3RDPARTY}/QEngineUtilities/Source/Core/Source/Public
        ${3RDPARTY}/QEngineUtilities/Source/Launch/Source/Public
    )
    target_link_libraries(${PROJECT_NAME} PRIVATE 
        Qt6::Core  
        Qt6::Widgets
        Qt6::Gui
        Qt6::GuiPrivate    
        QEngineCore   
        QEngineEditor
        QEngineLaunch
    )
endforeach()
