# add nias_cpp library target
macro(NIAS_CPP_ADD_LIBRARY)
    set(lib_name nias_cpp)
    set(bindings_lib_name ${lib_name}_bindings)

    if(TARGET ${lib_name})
        return()
    endif()

    # find C++ source files
    file(
        GLOB_RECURSE library_sources
        LIST_DIRECTORIES false
        "${_NIAS_CPP_DIR}/src/nias_cpp/*.h" "${_NIAS_CPP_DIR}/src/nias_cpp/*.cpp")

    # add C++ library
    add_library(${lib_name} ${library_sources})

    # add python bindings module
    set(bindings_sources ${_NIAS_CPP_DIR}/src/bindings/bindings.h ${_NIAS_CPP_DIR}/src/bindings/bindings.cpp)
    find_package(pybind11 CONFIG REQUIRED)
    pybind11_add_module(${lib_name}_bindings MODULE ${bindings_sources} ${NIAS_CPP_PYBIND11_NO_EXTRAS})

    # specify include directories and link libraries
    target_include_directories(${lib_name} PUBLIC $<BUILD_INTERFACE:${_NIAS_CPP_DIR}/src>
                                                  $<INSTALL_INTERFACE:${NIAS_CPP_INCLUDE_INSTALL_DIR}>)
    target_include_directories(${lib_name} PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
                                                  $<INSTALL_INTERFACE:${NIAS_CPP_INCLUDE_INSTALL_DIR}>)
    target_link_libraries(${lib_name} PUBLIC pybind11::pybind11 pybind11::embed)
    target_link_libraries(${bindings_lib_name} PRIVATE ${lib_name})

    # aliases
    add_library(nias_cpp::nias_cpp ALIAS nias_cpp)
    add_library(nias_cpp::nias_cpp_bindings ALIAS nias_cpp_bindings)
endmacro()
