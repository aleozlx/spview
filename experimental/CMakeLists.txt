#set(CMAKE_CXX_STANDARD 17)
enable_language(CUDA)

add_executable(exbbox bbox.cpp)

find_package(CUDA REQUIRED 10)
if(CUDA_FOUND)
    CUDA_SELECT_NVCC_ARCH_FLAGS(ARCH_FLAGS Auto)
endif()

if(CUDA_FOUND)
    cuda_add_executable(exvgng vgng.cu)
#    set_target_properties(exvgng PROPERTIES CUDA_SEPARABLE_COMPILATION ON)
    target_include_directories(exvgng PUBLIC ${CUDA_INCLUDE_DIRS})
    target_link_libraries(exvgng ${CUDA_LIBRARY})
endif()
