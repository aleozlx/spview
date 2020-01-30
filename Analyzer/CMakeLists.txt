cmake_minimum_required(VERSION 3.15)
project (SPVIEW)
set(CMAKE_CXX_STANDARD 17)
enable_language(CUDA)

if(FEATURE_DirectX)
    add_definitions(-DFEATURE_DirectX)
else()
    add_definitions(-DFEATURE_OpenGL)
endif()

find_package(OpenCV REQUIRED)
find_package(CUDA REQUIRED 10)
if(CUDA_FOUND)
    CUDA_SELECT_NVCC_ARCH_FLAGS(ARCH_FLAGS Auto)
endif()

if(FEATURE_GUI)
    add_executable(SuperpixelAnalyzer spview.cpp)
    target_link_libraries(SuperpixelAnalyzer
            app_engine
            sproutine)
endif()
