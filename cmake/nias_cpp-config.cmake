include_guard(GLOBAL)

get_filename_component(_NIAS_CPP_DIR "${CMAKE_CURRENT_LIST_FILE}" PATH)
get_filename_component(_NIAS_CPP_DIR "${_NIAS_CPP_DIR}" PATH)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# If CMAKE_CXX_VISIBILITY_PRESET is not set, pybind11 will set the visibility for the library to hidden
set(CMAKE_CXX_VISIBILITY_PRESET default)
set(CMAKE_POSITION_INDEPENDENT_CODE TRUE)

include(GNUInstallDirs)
set(_NIAS_CPP_DIR
    ${_NIAS_CPP_DIR}
    CACHE INTERNAL "")
set(NIAS_CPP_INSTALL_DIR
    "${CMAKE_INSTALL_PREFIX}/nias_cpp"
    CACHE PATH "")
set(NIAS_CPP_INCLUDE_INSTALL_DIR "${NIAS_CPP_INSTALL_DIR}/src")
set(NIAS_CPP_CMAKE_INSTALL_DIR "${NIAS_CPP_INSTALL_DIR}/cmake")

if(NOT COMMAND pybind11_add_module)
    include(CMakeFindDependencyMacro)
    set(PYBIND11_FINDPYTHON ON)
    find_dependency(Python COMPONENTS Interpreter Development REQUIRED)
    find_dependency(pybind11 CONFIG REQUIRED HINTS ${_NIAS_CPP_DIR}/../pybind11/share/cmake/pybind11)
endif()

file(WRITE /home/tobias/nias_cpp_config.log
     "nias include dirs: ${CMAKE_CURRENT_BINARY_DIR} ${NIAS_CPP_INCLUDE_INSTALL_DIR} ${_NIAS_CPP_DIR}/src")

function(nias_cpp_build_library target_name)
    if(TARGET ${target_name})
        return()
    endif()

    pybind11_add_module(
        ${target_name}
        SHARED
        ${ARG_UNPARSED_ARGUMENTS}
        ${_NIAS_CPP_DIR}/src/nias_cpp/algorithms/gram_schmidt.h
        ${_NIAS_CPP_DIR}/src/nias_cpp/checked_integer_cast.h
        ${_NIAS_CPP_DIR}/src/nias_cpp/concepts.h
        ${_NIAS_CPP_DIR}/src/nias_cpp/exceptions.h
        ${_NIAS_CPP_DIR}/src/nias_cpp/exceptions.cpp
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
    target_include_directories(${target_name} PUBLIC ${CMAKE_CURRENT_BINARY_DIR}
                                                     ${NIAS_CPP_INCLUDE_INSTALL_DIR})
    target_link_libraries(${target_name} PUBLIC pybind11::pybind11 pybind11::embed)
    set_target_properties(${target_name} PROPERTIES LINKER_LANGUAGE CXX)
endfunction()

function(nias_cpp_add_module name)
    cmake_parse_arguments(PARSE_ARGV 1 ARG "" "" "")

    pybind11_add_module(${name} SHARED ${ARG_UNPARSED_ARGUMENTS})

    target_link_libraries(${name} PUBLIC nias_cpp)
endfunction()

nias_cpp_build_library(nias_cpp)

# setup virtual environment
set(NIAS_CPP_VENV_DIR ${CMAKE_CURRENT_BINARY_DIR}/python/$<CONFIG>)
add_custom_target(create_venv COMMAND ${UV_EXECUTABLE} venv --python ${Python_EXECUTABLE}
                                      ${NIAS_CPP_VENV_DIR})

target_compile_definitions(nias_cpp PRIVATE NIAS_CPP_VENV_DIR="${NIAS_CPP_VENV_DIR}")
target_compile_definitions(nias_cpp PRIVATE NIAS_CPP_BUILD_DIR="$<TARGET_FILE_DIR:nias_cpp>")
add_custom_target(
    install_dependencies_into_venv
    COMMAND ${UV_EXECUTABLE} pip compile --python ${Python_EXECUTABLE} --quiet
            ${CMAKE_CURRENT_SOURCE_DIR}/pyproject.toml -o ${CMAKE_CURRENT_BINARY_DIR}/requirements.txt
    COMMAND ${UV_EXECUTABLE} venv --python ${Python_EXECUTABLE} ${NIAS_CPP_VENV_DIR} --quiet
    COMMAND ${CMAKE_COMMAND} -E env "VIRTUAL_ENV=${NIAS_CPP_VENV_DIR}" ${UV_EXECUTABLE} pip install
            --requirements ${CMAKE_CURRENT_BINARY_DIR}/requirements.txt --quiet
    COMMENT "Installing nias_cpp dependencies into virtual environment")
add_dependencies(install_dependencies_into_venv create_venv)
add_dependencies(nias_cpp install_dependencies_into_venv)
add_library(nias_cpp::nias_cpp ALIAS nias_cpp)

# installation rules
install(
    DIRECTORY src/
    DESTINATION "${NIAS_CPP_INCLUDE_INSTALL_DIR}"
    PATTERN "*.py" EXCLUDE)

install(DIRECTORY cmake/ DESTINATION "${NIAS_CPP_CMAKE_INSTALL_DIR}")

install(TARGETS nias_cpp LIBRARY DESTINATION ${NIAS_CPP_INSTALL_DIR})

# generate export header
include(GenerateExportHeader)
generate_export_header(
    nias_cpp
    BASE_NAME
    nias_cpp
    EXPORT_MACRO_NAME
    NIAS_CPP_EXPORT
    EXPORT_FILE_NAME
    nias_cpp_export.h
    STATIC_DEFINE
    NIAS_CPP_STATIC)
install(FILES "${PROJECT_BINARY_DIR}/nias_cpp_export.h" DESTINATION "${NIAS_CPP_INCLUDE_INSTALL_DIR}")
