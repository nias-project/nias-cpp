include_guard(GLOBAL)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_POSITION_INDEPENDENT_CODE TRUE)

include(GNUInstallDirs)

# set up paths
get_filename_component(_NIAS_CPP_DIR "${CMAKE_CURRENT_LIST_FILE}" PATH)
get_filename_component(_NIAS_CPP_DIR "${_NIAS_CPP_DIR}" PATH)
set(_NIAS_CPP_DIR
    ${_NIAS_CPP_DIR}
    CACHE INTERNAL "")

# make sure uv is available to run the python script for parsing pyproject.toml
# first try to find uv in the environment
find_program(UV_EXECUTABLE uv uv.exe)
# if we could not find uv, download it
if(NOT UV_EXECUTABLE)
    include(FetchContent)
    set(_uv_version "0.6.3")
    if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
        set(_uv_archive_name "uv-x86_64-pc-windows-msvc.zip")
    elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        set(_uv_archive_name "uv-x86_64-unknown-linux-gnu.tar.gz")
    else()
        message(WARNING "Could not download uv binary, unsupported system: ${CMAKE_SYSTEM_NAME}.")
        message(WARNING "Please make sure uv is available in your environment.")
    endif()
    FetchContent_Declare(
        uv URL "https://github.com/astral-sh/uv/releases/download/${_uv_version}/${_uv_archive_name}")
    FetchContent_MakeAvailable(uv)
endif()
# now we should have uv
find_program(UV_EXECUTABLE uv uv.exe HINTS ${uv_BINARY_DIR} REQUIRED)

if(NOT COMMAND pybind11_add_module)
    include(CMakeFindDependencyMacro)
    # parse pyproject.toml to get pybind11 version
    execute_process(
        COMMAND ${UV_EXECUTABLE} run --no-project --with toml cmake/parse_pyproject_toml.py pybind11
        WORKING_DIRECTORY ${_NIAS_CPP_DIR}
        OUTPUT_VARIABLE PYBIND11_VERSION)

    string(FIND "${PYBIND11_VERSION}" "Error:" _pybind11_version_error)
    if(_pybind11_version_error GREATER -1)
        message(WARNING "Could not parse pyproject.toml to get pybind11 version.")
        set(PYBIND11_VERSION 2.13.6)
        message(WARNING "Setting pybind11 version to default value ${PYBIND11_VERSION}.")
    endif()

    # add pybind11
    include(FetchContent)
    find_dependency(Python COMPONENTS Interpreter Development REQUIRED)
    set(PYBIND11_FINDPYTHON ON)
    FetchContent_Declare(
        pybind11
        GIT_REPOSITORY https://github.com/pybind/pybind11
        GIT_TAG "v${PYBIND11_VERSION}"
        OVERRIDE_FIND_PACKAGE)
    FetchContent_MakeAvailable(pybind11)
    find_dependency(pybind11 CONFIG REQUIRED)
endif()

set(NIAS_CPP_REL_INCLUDE_INSTALL_DIR "nias_cpp/src")
set(NIAS_CPP_REL_CMAKE_INSTALL_DIR "nias_cpp/cmake")

# add nias_cpp library target
function(nias_cpp_build_library target_name)
    if(TARGET ${target_name})
        return()
    endif()

    # find C++ source files
    file(
        GLOB_RECURSE library_sources
        LIST_DIRECTORIES false
        "${PROJECT_SOURCE_DIR}/src/*.h" "${PROJECT_SOURCE_DIR}/src/*.cpp")

    pybind11_add_module(${target_name} SHARED ${ARG_UNPARSED_ARGUMENTS} ${library_sources})

    target_include_directories(${target_name} PUBLIC $<BUILD_INTERFACE:${_NIAS_CPP_DIR}/src>
                                                     $<INSTALL_INTERFACE:${NIAS_CPP_REL_INCLUDE_INSTALL_DIR}>)
    target_include_directories(${target_name} PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
                                                     $<INSTALL_INTERFACE:${NIAS_CPP_REL_INCLUDE_INSTALL_DIR}>)

    target_link_libraries(${target_name} PUBLIC pybind11::pybind11 pybind11::embed)
    set_target_properties(${target_name} PROPERTIES LINKER_LANGUAGE CXX)
endfunction()

function(nias_cpp_add_module name)
    cmake_parse_arguments(PARSE_ARGV 1 ARG "" "" "")

    pybind11_add_module(${name} SHARED ${ARG_UNPARSED_ARGUMENTS})

    target_link_libraries(${name} PUBLIC nias_cpp)
endfunction()

nias_cpp_build_library(nias_cpp)
add_library(nias_cpp::nias_cpp ALIAS nias_cpp)

if(DEFINED ENV{VIRTUAL_ENV})
    set(NIAS_CPP_VENV_DIR $ENV{VIRTUAL_ENV})
    # TODO: check that nias_cpp is installed in the virtual environment
else()
    # if no virtualenv is active, we set up our own virtual environment to install our python dependencies
    set(NIAS_CPP_VENV_DIR ${CMAKE_CURRENT_BINARY_DIR}/nias_cpp_venv/$<CONFIG>)
    add_custom_target(create_venv COMMAND ${UV_EXECUTABLE} venv --python ${Python_EXECUTABLE} --quiet
                                          ${NIAS_CPP_VENV_DIR})
    add_custom_target(
        install_dependencies_into_venv
        COMMAND ${UV_EXECUTABLE} pip compile --python ${Python_EXECUTABLE} --quiet
                ${_NIAS_CPP_DIR}/pyproject.toml -o ${CMAKE_CURRENT_BINARY_DIR}/requirements.txt
        COMMAND ${UV_EXECUTABLE} venv --python ${Python_EXECUTABLE} ${NIAS_CPP_VENV_DIR} --quiet
        COMMAND ${CMAKE_COMMAND} -E env "VIRTUAL_ENV=${NIAS_CPP_VENV_DIR}" ${UV_EXECUTABLE} pip install
                --requirements ${CMAKE_CURRENT_BINARY_DIR}/requirements.txt --quiet
        COMMENT "Installing nias_cpp dependencies into virtual environment")
    add_dependencies(nias_cpp install_dependencies_into_venv)
    add_dependencies(install_dependencies_into_venv create_venv)
endif()

# pass the virtual environment and library directory to the C++ code
target_compile_definitions(nias_cpp PRIVATE NIAS_CPP_VENV_DIR="${NIAS_CPP_VENV_DIR}")
target_compile_definitions(nias_cpp PRIVATE NIAS_CPP_BUILD_DIR="$<TARGET_FILE_DIR:nias_cpp>")

# installation rules
install(
    DIRECTORY src/
    DESTINATION "${NIAS_CPP_REL_INCLUDE_INSTALL_DIR}"
    PATTERN "*.py" EXCLUDE)

install(DIRECTORY cmake/ DESTINATION "${NIAS_CPP_REL_CMAKE_INSTALL_DIR}")

install(
    TARGETS nias_cpp
    EXPORT nias_cpp
    LIBRARY DESTINATION nias_cpp)

install(
    EXPORT nias_cpp
    NAMESPACE nias_cpp::
    DESTINATION ${NIAS_CPP_REL_CMAKE_INSTALL_DIR})

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
install(FILES "${PROJECT_BINARY_DIR}/nias_cpp_export.h" DESTINATION "${NIAS_CPP_REL_INCLUDE_INSTALL_DIR}")
