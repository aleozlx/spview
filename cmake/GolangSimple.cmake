# https://github.com/cpconduce/go_cmake
# Note: it has been modified to support Windows
set(GOPATH "${CMAKE_CURRENT_BINARY_DIR}/go")
file(MAKE_DIRECTORY ${GOPATH})

function(GO_GET TARG)
    add_custom_target(${TARG} env GOPATH=${GOPATH} go get ${ARGN})
endfunction(GO_GET)

function(ADD_GO_INSTALLABLE_PROGRAM NAME MAIN_SRC)
    get_filename_component(MAIN_SRC_ABS ${MAIN_SRC} ABSOLUTE)
    if(WIN32)
        set(BIN_NAME ${NAME}.exe)
    else()
        set(BIN_NAME ${NAME})
    endif()
    add_custom_target(${NAME}
            ${CMAKE_COMMAND} -E env GOPATH=${GOPATH} go build
            -o "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${BIN_NAME}" # TODO Generated MSVC Project has an extra level
#            -o "${CMAKE_CURRENT_BINARY_DIR}/${BIN_NAME}"
            ${CMAKE_GO_FLAGS} ${MAIN_SRC}
            WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
            DEPENDS ${MAIN_SRC_ABS})
#    foreach(DEP ${ARGN})
#        add_dependencies(${NAME} ${DEP})
#    endforeach()

#    add_custom_target(${NAME}_all ALL DEPENDS ${NAME})
#    install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/${BIN_NAME} DESTINATION bin)
endfunction(ADD_GO_INSTALLABLE_PROGRAM)
