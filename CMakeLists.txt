cmake_minimum_required(VERSION 3.6)
project(simple-nes)

if(CMAKE_GENERATOR MATCHES "Visual Studio")
    add_definitions(-D_USING_MSVC)
elseif(CMAKE_GENERATOR MATCHES "MinGW Makefiles")
    add_definitions(-D_USING_MINGW)
endif()

add_subdirectory(third/glfw)
add_subdirectory(frontend/windows-frontend)