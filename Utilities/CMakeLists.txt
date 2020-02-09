cmake_minimum_required(VERSION 3.8)
project ("SPVIEW_Utilities")

if(FEATURE_GUI)
    if(FEATURE_DirectX)
        add_definitions(-DFEATURE_DirectX)
    else()
        add_definitions(-DFEATURE_OpenGL)
    endif()

    add_executable(DatasetScanner DatasetScanner.cpp
            ${INTEROPS_DIR}/tinyfiledialogs/tinyfiledialogs.c)
    target_link_libraries(DatasetScanner app_engine)
    add_dependencies(DatasetScanner dirscan)
endif()

add_library(georoutine STATIC bbox.cpp)
