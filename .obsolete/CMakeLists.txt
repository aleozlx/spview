cmake_minimum_required(VERSION 3.3)
project(spview)
set(CMAKE_CXX_STANDARD 17)
set(OpenGL_GL_PREFERENCE GLVND)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/modules)

# Feature options
option(FEATURE_GUI "Enable GUI (requires OpenGL)")

# GUI Packages
if(FEATURE_GUI)
    if(FEATURE_DX)
        # TODO check DX version?
    else()
        find_package(OpenGL REQUIRED)
        find_package(GLEW REQUIRED)
        find_package(SOIL REQUIRED)
        find_package(PkgConfig REQUIRED)
        pkg_search_module(GLFW REQUIRED glfw3)
    endif()
endif()

# Computer Vision Packages
#find_package(OpenCV REQUIRED)

if(FEATURE_GUI)
# ImGui Library
set(root ${CMAKE_CURRENT_SOURCE_DIR}/imgui)
set(phdrs ${root}/imgui.h)
set(hdrs ${root}/imstb_rectpack.h ${root}/imstb_textedit.h ${root}/imstb_truetype.h
    ${root}/imgui_internal.h)
set(srcs ${root}/imgui.cpp
    ${root}/imgui_demo.cpp ${root}/imgui_draw.cpp ${root}/imgui_widgets.cpp)

add_library(imgui STATIC ${phdrs} ${hdrs} ${srcs})
target_include_directories(imgui PUBLIC
    $<BUILD_INTERFACE:${root}>
    $<INSTALL_INTERFACE:include>)
target_compile_definitions(imgui PUBLIC IMGUI_DISABLE_INCLUDE_IMCONFIG_H)
endif()

# Global Config
#include_directories(SYSTEM "/usr/include/opencv4")
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
#include_directories(${CMAKE_CURRENT_SOURCE_DIR}/gSLICr)

if(FEATURE_GUI)
# ImGui Demo
add_executable(gui_demo ${CMAKE_CURRENT_SOURCE_DIR}/demos/imgui.cpp ${root}/examples/imgui_impl_glfw.cpp ${root}/examples/imgui_impl_opengl3.cpp)
target_compile_definitions(gui_demo PUBLIC IMGUI_IMPL_OPENGL_LOADER_GLEW)
target_include_directories(gui_demo PUBLIC ${root}/examples)
target_link_libraries(gui_demo imgui ${SOIL_LIBRARY} ${GLFW_LIBRARIES} ${GLEW_LIBRARIES} ${OPENGL_LIBRARIES})
endif()

# Tests
if(Boost_FOUND)
    set(CMAKE_BUILD_TYPE Debug)
    set(CMAKE_EXE_LINKER_FLAGS  "-D_GLIBCXX_USE_CXX11_ABI=${TensorFlow_ABI}" )
    include_directories(SYSTEM "tests")

    if(LIBPQXX_FOUND)
        add_executable(test_pq "tests/test_pq.cpp")
        target_include_directories(test_pq PUBLIC ${LIBPQXX_INCLUDE_DIR})
        target_link_libraries(test_pq fpconv pqxx)
    endif()
endif()

# set(targets imgui)
# set(headers_to_install ${phdrs})

# install(TARGETS ${targets}
#     EXPORT imgui-targets DESTINATION lib)
# install(EXPORT imgui-targets
#     FILE imgui-config.cmake
#     NAMESPACE imgui::
#     DESTINATION lib/cmake/imgui)
# install(FILES ${headers_to_install}
#     DESTINATION include)
