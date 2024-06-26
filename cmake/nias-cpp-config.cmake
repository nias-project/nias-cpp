include_guard(GLOBAL)

get_filename_component(_NIAS_CPP_DIR "${CMAKE_CURRENT_LIST_FILE}" PATH)
get_filename_component(_NIAS_CPP_DIR "${_NIAS_CPP_DIR}" PATH)

set(_NIAS_CPP_DIR
    ${_NIAS_CPP_DIR}
    CACHE INTERNAL "")

function(nias_cpp_build_library target_name)
    if(TARGET ${target_name})
        return()
    endif()

    add_library(${target_name} SHARED ${_NIAS_CPP_DIR}/src/nias-cpp/gram_schmidt.h
                                      ${_NIAS_CPP_DIR}/src/nias-cpp/vector.h)

    target_include_directories(${target_name} SYSTEM PUBLIC ${_NIAS_CPP_DIR}/src)

    target_include_directories(${target_name} SYSTEM PUBLIC ${_NIAS_CPP_DIR}/extern)

    target_link_libraries(${target_name} PUBLIC pybind11::pybind11 pybind11::embed)

    set_target_properties(${target_name} PROPERTIES LINKER_LANGUAGE CXX)
endfunction()

function(nias_cpp_add_module name)
    cmake_parse_arguments(PARSE_ARGV 1 ARG "" "" "")

    pybind11_add_module(${name} SHARED ${ARG_UNPARSED_ARGUMENTS})

    set(libname "nias_cpp")

    nias_cpp_build_library(${libname})

    target_link_libraries(${name} PUBLIC ${libname})
endfunction()
