include_guard(GLOBAL)

get_filename_component(_NIAS_CPP_DIR "${CMAKE_CURRENT_LIST_FILE}" PATH)
get_filename_component(_NIAS_CPP_DIR "${_NIAS_CPP_DIR}" PATH)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# If CMAKE_CXX_VISIBILITY_PRESET is not set, pybind11 will set the visibility for the library to hidden
set(CMAKE_CXX_VISIBILITY_PRESET default)
set(CMAKE_POSITION_INDEPENDENT_CODE TRUE)

set(_NIAS_CPP_DIR
    ${_NIAS_CPP_DIR}
    CACHE INTERNAL "")

if(NOT COMMAND pybind11_add_module)
    include(CMakeFindDependencyMacro)
    set(PYBIND11_FINDPYTHON ON)
    find_dependency(Python COMPONENTS Interpreter Development REQUIRED)
    find_dependency(pybind11 CONFIG REQUIRED HINTS ${_NIAS_CPP_DIR}/../pybind11/share/cmake/pybind11)
endif()

function(nias_cpp_build_library target_name)
    if(TARGET ${target_name})
        return()
    endif()

    pybind11_add_module(
        ${target_name}
        SHARED
        ${ARG_UNPARSED_ARGUMENTS}
        ${_NIAS_CPP_DIR}/src/nias_cpp/algorithms/gram_schmidt.h
        ${_NIAS_CPP_DIR}/src/nias_cpp/concepts.h
        ${_NIAS_CPP_DIR}/src/nias_cpp/indices.h
        ${_NIAS_CPP_DIR}/src/nias_cpp/indices.cpp
        ${_NIAS_CPP_DIR}/src/nias_cpp/interpreter.h
        ${_NIAS_CPP_DIR}/src/nias_cpp/interpreter.cpp
        ${_NIAS_CPP_DIR}/src/nias_cpp/interfaces/vector.h
        ${_NIAS_CPP_DIR}/src/nias_cpp/interfaces/vectorarray.h
        ${_NIAS_CPP_DIR}/src/nias_cpp/vectorarray/list.h
        ${_NIAS_CPP_DIR}/src/nias_cpp/vectorarray/numpy.h
        ${_NIAS_CPP_DIR}/src/nias_cpp/bindings.h
        ${_NIAS_CPP_DIR}/src/nias_cpp/bindings.cpp)

    target_include_directories(${target_name} SYSTEM PUBLIC ${_NIAS_CPP_DIR}/src)

    target_include_directories(${target_name} PUBLIC ${CMAKE_BINARY_DIR} ${NIAS_CPP_INSTALL_INCLUDE_DIR})

    target_link_libraries(${target_name} PUBLIC pybind11::pybind11 pybind11::embed)

    set_target_properties(${target_name} PROPERTIES LINKER_LANGUAGE CXX)
endfunction()

function(nias_cpp_add_module name)
    cmake_parse_arguments(PARSE_ARGV 1 ARG "" "" "")

    pybind11_add_module(${name} SHARED ${ARG_UNPARSED_ARGUMENTS})

    target_link_libraries(${name} PUBLIC nias_cpp)
endfunction()

nias_cpp_build_library(nias_cpp)
