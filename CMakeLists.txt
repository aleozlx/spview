cmake_minimum_required(VERSION 3.8)
project(spview)
set(CMAKE_CXX_STANDARD 17)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/modules)

# Feature options
option(FEATURE_DirectX "Use DirectX instead of OpenGL" OFF)
option(FEATURE_GUI "Enable GUI" ON)

# Global Config
include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/AppEngine/imgui/examples)

# GUI Packages
if(FEATURE_GUI)
    add_subdirectory(AppEngine)
    add_subdirectory(AppDemo)
endif()

