cmake_minimum_required(VERSION 3.8)
project ("SPVIEW_AppDemo")

if(FEATURE_GUI)
    if(FEATURE_DirectX)
        add_definitions(-DFEATURE_DirectX)
    else()
        add_definitions(-DFEATURE_OpenGL)
    endif()

    add_executable(exAppDemo app_demo.cpp)
    target_link_libraries(exAppDemo app_engine)

    add_executable(exMNIST MNIST.cpp)
    target_link_libraries(exMNIST app_engine)
endif()
