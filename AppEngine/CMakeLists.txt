cmake_minimum_required(VERSION 3.8)
project ("SPVIEW_AppEngine")

set(CMAKE_CXX_STANDARD 17)

option(FEATURE_DirectX "Use DirectX instead of OpenGL")

if(FEATURE_DirectX)
    add_definitions(-DFEATURE_DirectX)
    # TODO check DX version?
else()
    add_definitions(-DFEATURE_OpenGL)
    find_package(OpenGL REQUIRED)
    find_package(GLEW REQUIRED)
    find_package(SOIL REQUIRED)
    find_package(PkgConfig REQUIRED)
    pkg_search_module(GLFW REQUIRED glfw3)
endif()

# ImGui Library
set(root ${CMAKE_CURRENT_SOURCE_DIR}/imgui)
set(phdrs ${root}/imgui.h)
set(hdrs ${root}/imstb_rectpack.h ${root}/imstb_textedit.h ${root}/imstb_truetype.h
        ${root}/imgui_internal.h)
set(srcs ${root}/imgui.cpp
        ${root}/imgui_demo.cpp ${root}/imgui_draw.cpp ${root}/imgui_widgets.cpp ../AppDemo/app_demo.cpp)

add_library(imgui STATIC ${phdrs} ${hdrs} ${srcs})
target_include_directories(imgui PUBLIC
        $<BUILD_INTERFACE:${root}>
        $<INSTALL_INTERFACE:include>)
target_compile_definitions(imgui PUBLIC IMGUI_DISABLE_INCLUDE_IMCONFIG_H)

if(FEATURE_DirectX)
    add_library(app_engine STATIC app.cpp ${root}/examples/imgui_impl_win32.cpp ${root}/examples/imgui_impl_dx11.cpp)
    target_include_directories(app_engine PUBLIC ${root}/examples)
    target_link_libraries(app_engine imgui d3d11 dxguid)

#    Just don't do DLLs - global data gets obnoxious
#    include(GenerateExportHeader)
#    generate_export_header(app_engine)
else()
    add_library(app_engine STATIC app.cpp ${root}/examples/imgui_impl_glfw.cpp ${root}/examples/imgui_impl_opengl3.cpp)
    target_compile_definitions(app_engine PUBLIC IMGUI_IMPL_OPENGL_LOADER_GLEW)
    target_include_directories(app_engine PUBLIC ${root}/examples)
    target_link_libraries(app_engine imgui ${SOIL_LIBRARY} ${GLFW_LIBRARIES} ${GLEW_LIBRARIES} ${OPENGL_LIBRARIES})
endif()

if(FEATURE_GUI)
    if(FEATURE_DirectX)
        add_executable(exImGui ${root}/examples/example_win32_directx11/main.cpp)
    else()
        add_executable(exImGui ${root}/examples/example_glfw_opengl3/main.cpp)
    endif()
    target_link_libraries(exImGui app_engine)
endif()
