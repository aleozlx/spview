cmake_minimum_required(VERSION 3.8)
project ("sproutine")
enable_language(CUDA)
set(CMAKE_CXX_STANDARD 17)
set(OpenGL_GL_PREFERENCE GLVND)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/modules)

find_package(OpenCV REQUIRED)
find_package(CUDA REQUIRED 10)

# gSLIC Library
if(CUDA_FOUND)
    if(NOT MSVC)
        set(CFLAGS_WARN "-Wall -Wextra -Wno-unused-parameter -Wno-strict-aliasing")
        set(CMAKE_CXX_FLAGS "-fPIC -O3 -march=native ${CFLAGS_WARN} ${CMAKE_CXX_FLAGS}")
    endif()
    include_directories(${CUDA_INCLUDE_DIRS})
    include_directories(${OpenCV_INCLUDE_DIRS})
    set(GSLICR_LIB
            gSLICr/gSLICr_Lib/engines/gSLICr_core_engine.h
            gSLICr/gSLICr_Lib/engines/gSLICr_seg_engine.h
            gSLICr/gSLICr_Lib/engines/gSLICr_seg_engine_GPU.h
            gSLICr/gSLICr_Lib/engines/gSLICr_seg_engine_shared.h
            gSLICr/gSLICr_Lib/engines/gSLICr_core_engine.cpp
            gSLICr/gSLICr_Lib/engines/gSLICr_seg_engine.cpp
            gSLICr/gSLICr_Lib/engines/gSLICr_seg_engine_GPU.cu
            gSLICr/gSLICr_Lib/objects/gSLICr_settings.h
            gSLICr/gSLICr_Lib/objects/gSLICr_spixel_info.h
            gSLICr/gSLICr_Lib/gSLICr_defines.h
            gSLICr/gSLICr_Lib/gSLICr.h
            )
    cuda_add_library(gSLICr
            ${GSLICR_LIB}
            OPTIONS
            -gencode arch=compute_61,code=compute_61
            -gencode arch=compute_70,code=compute_70
            -gencode arch=compute_75,code=compute_75)
    target_link_libraries(gSLICr ${CUDA_LIBRARY})
endif()