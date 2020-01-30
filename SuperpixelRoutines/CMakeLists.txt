cmake_minimum_required(VERSION 3.15)
project ("sproutine")
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

# gSLIC Library
if(CUDA_FOUND)
    if(NOT MSVC)
        set(CFLAGS_WARN "-Wall -Wextra -Wno-unused-parameter -Wno-strict-aliasing")
        set(CMAKE_CXX_FLAGS "-fPIC -O3 -march=native ${CFLAGS_WARN} ${CMAKE_CXX_FLAGS}")
    endif()
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
            gSLICr/gSLICr_Lib/gSLICr.h)
    cuda_add_library(gSLICr STATIC ${GSLICR_LIB} OPTIONS ${ARCH_FLAGS})
    target_include_directories(gSLICr PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/gSLICr/gSLICr_Lib)
    target_include_directories(gSLICr PUBLIC ${CUDA_INCLUDE_DIRS})
    target_include_directories(gSLICr PRIVATE ${OpenCV_INCLUDE_DIRS})
    target_link_libraries(gSLICr ${CUDA_LIBRARY})
endif()

# gSLIC Demo
if(FEATURE_GUI)
    add_executable(exGSLIC gslic_demo.cpp superpixel.cpp)
    if(CUDA_FOUND)
        target_link_libraries(exGSLIC gSLICr)
        target_compile_definitions(exGSLIC PUBLIC -DFEATURE_GSLICR)
    endif()

    # TODO check whether opencv_ximgproc exists
    target_link_libraries(exGSLIC
            app_engine
            opencv_core
            opencv_imgproc
#            opencv_ximgproc
            opencv_imgcodecs
            opencv_videoio
            opencv_video)
endif()

add_library(sproutine STATIC superpixel.cpp)
if(CUDA_FOUND)
    target_link_libraries(sproutine gSLICr)
    target_compile_definitions(sproutine PUBLIC -DFEATURE_GSLICR)
endif()
target_link_libraries(sproutine
        opencv_core
        opencv_imgproc
        opencv_imgcodecs
        opencv_videoio
        opencv_video)
