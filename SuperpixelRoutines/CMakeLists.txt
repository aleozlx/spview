cmake_minimum_required(VERSION 3.8)
project ("sproutine")

set(CMAKE_CXX_STANDARD 17)
set(OpenGL_GL_PREFERENCE GLVND)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/modules)

# Computer Vision Packages
#find_package(OpenCV REQUIRED)
