cmake_minimum_required(VERSION 3.8)
project ("SPVIEW_AppDemo")

if(FEATURE_DirectX)
    add_definitions(-DFEATURE_DirectX)
else()
    add_definitions(-DFEATURE_OpenGL)
endif()

#set(imgui_root ${CMAKE_SOURCE_DIR}/AppEngine/imgui)
add_executable(app_demo ${CMAKE_CURRENT_SOURCE_DIR}/appdemo.cpp)
#target_include_directories(app_demo PUBLIC ${imgui_root}/examples)
target_link_libraries(app_demo app_engine)
