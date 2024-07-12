include_guard(GLOBAL)

get_filename_component(_NIAS_CPP_DIR "${CMAKE_CURRENT_LIST_FILE}" PATH)
get_filename_component(_NIAS_CPP_DIR "${_NIAS_CPP_DIR}" PATH)

set(_NIAS_CPP_DIR
    ${_NIAS_CPP_DIR}
    CACHE INTERNAL "")

include(CMakeFindDependencyMacro)
find_dependency(pybind11 CONFIG REQUIRED HINTS ${_NIAS_CPP_DIR}/../pybind11/share/cmake/pybind11)

function(nias_cpp_build_library target_name)
    if(TARGET ${target_name})
        return()
    endif()

    pybind11_add_module(
        ${target_name}
        SHARED
        ${ARG_UNPARSED_ARGUMENTS}
        ${_NIAS_CPP_DIR}/src/nias_cpp/gram_schmidt.h
        ${_NIAS_CPP_DIR}/src/nias_cpp/vector.h
        ${_NIAS_CPP_DIR}/src/nias_cpp/bindings.h
        ${_NIAS_CPP_DIR}/src/nias_cpp/bindings.cpp)

    target_include_directories(${target_name} SYSTEM PUBLIC ${_NIAS_CPP_DIR}/src)

    target_include_directories(${target_name} SYSTEM PUBLIC ${_NIAS_CPP_DIR}/extern)

    target_link_libraries(${target_name} PUBLIC pybind11::pybind11 pybind11::embed)

    set_target_properties(${target_name} PROPERTIES LINKER_LANGUAGE CXX)
endfunction()

function(nias_cpp_add_module name)
    cmake_parse_arguments(PARSE_ARGV 1 ARG "" "" "")

    pybind11_add_module(${name} SHARED ${ARG_UNPARSED_ARGUMENTS})

    target_link_libraries(${name} PUBLIC nias_cpp)
endfunction()

nias_cpp_build_library(nias_cpp)
