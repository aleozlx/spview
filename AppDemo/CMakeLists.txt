cmake_minimum_required(VERSION 3.8)
project ("SPVIEW_AppDemo")

if(FEATURE_GUI)
    if(FEATURE_DirectX)
        add_definitions(-DFEATURE_DirectX)
    else()
        add_definitions(-DFEATURE_OpenGL)
    endif()

    add_executable(app_demo ${CMAKE_CURRENT_SOURCE_DIR}/app_demo.cpp)
    target_link_libraries(app_demo app_engine)

    add_executable(DatasetScanner ${CMAKE_CURRENT_SOURCE_DIR}/DatasetScanner.cpp)
    target_link_libraries(DatasetScanner app_engine)
    add_dependencies(DatasetScanner dirscan)
endif()
