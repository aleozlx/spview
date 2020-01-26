cmake_minimum_required(VERSION 3.8)
project(spview)
set(CMAKE_CXX_STANDARD 17)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/modules)

# Feature options
option(FEATURE_GUI "Enable GUI" ON)
option(FEATURE_DirectX "Use DirectX instead of OpenGL" OFF)

if(NOT FEATURE_DirectX)
set(OpenGL_GL_PREFERENCE GLVND)
endif()

# Global Config
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/AppEngine/imgui/examples)
include_directories(${CMAKE_SOURCE_DIR}/Interops/stb)
add_definitions(-DFIXTURES_DIR="${CMAKE_SOURCE_DIR}/Fixtures/") # suffix / is required

# GUI Packages
if(FEATURE_GUI)
    add_subdirectory(AppEngine)
    add_subdirectory(SuperpixelRoutines)
    add_subdirectory(AppDemo)
endif()

