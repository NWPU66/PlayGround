list(APPEND CMAKE_PREFIX_PATH "E:/Qt/6.8.1/msvc2022_64")  
find_package(Qt6 COMPONENTS Core Widgets Gui REQUIRED)  

#搜索每一个前缀是"demo_"的文件夹，包含他们
file(GLOB demo_folders  "*")
foreach(folder ${demo_folders})
    if(IS_DIRECTORY ${folder})
        add_subdirectory(${folder})
    endif()
endforeach()
