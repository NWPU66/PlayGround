cmake_minimum_required(VERSION 3.25)

if(CMAKE_HOST_SYSTEM_NAME MATCHES "Linux")
	message("current platform: Linux")
elseif(CMAKE_HOST_SYSTEM_NAME MATCHES "Windows")
	message("current platform: Windows")
    set(CMAKE_TOOLCHAIN_FILE "E:/vcpkg/scripts/buildsystems/vcpkg.cmake")
    set(VCPKG_INCLUDE_DIR "E:/vcpkg/installed/x64-windows/include")
else()
	message("current platform: unkonw")
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS true)
project(PlayGround
    VERSION 0.1.0
    DESCRIPTION "Play Ground"
    LANGUAGES CXX)

# add subdirectory
set(ThirdParty ${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty)
add_subdirectory(ThirdParty)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/Source)
# NOTE - subdir() & add_subdirectory() 作用一样，但是官方文档建议使用后者

# install path
set(CMAKE_INSTALL_PREFIX ${CMAKE_CURRENT_SOURCE_DIR}/Install)
