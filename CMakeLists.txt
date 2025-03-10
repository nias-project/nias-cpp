# This file is part of the NiAS project (https://github.com/nias-project).
# Copyright NiAS developers and contributors. All rights reserved.
# License: BSD 2-Clause License (https://opensource.org/licenses/BSD-2-Clause)

# Require CMake 3.15+ (matching scikit-build-core) Use new versions of all
# policies up to CMake 3.31
cmake_minimum_required(VERSION 3.15...3.31)
if(SKBUILD)
    project(
        ${SKBUILD_PROJECT_NAME}
        VERSION ${SKBUILD_PROJECT_VERSION}
        LANGUAGES CXX)
else()
    project(
        nias_cpp
        VERSION 0.1.0
        LANGUAGES CXX)
endif()
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_POSITION_INDEPENDENT_CODE TRUE)

# We want to include the tests below only if nias-cpp is the master project
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    set(NIAS_CPP_MASTER_PROJECT ON)
else()
    set(NIAS_CPP_MASTER_PROJECT OFF)
endif()

# set build type to Release if not specified
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    message(STATUS "nias_cpp: setting build type to 'Release' as none was specified.")
    set(CMAKE_BUILD_TYPE
        Release
        CACHE STRING "Choose the type of build." FORCE)
endif()

# enable warnings for gcc and clang
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall -Wextra -Wpedantic")
endif()

get_filename_component(_NIAS_CPP_DIR "${CMAKE_CURRENT_LIST_FILE}" PATH)
set(_NIAS_CPP_DIR
    ${_NIAS_CPP_DIR}
    CACHE INTERNAL "")

include(GNUInstallDirs)
set(NIAS_CPP_INCLUDE_INSTALL_DIR ${CMAKE_INSTALL_INCLUDEDIR})
set(NIAS_CPP_REL_CMAKE_INSTALL_DIR "nias_cpp/cmake")

# make sure uv is available to run the python script for parsing pyproject.toml
# first try to find uv in the environment
find_program(UV_EXECUTABLE NAMES uv uv.exe)
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
find_program(
    UV_EXECUTABLE
    NAMES uv uv.exe
    HINTS ${uv_BINARY_DIR} REQUIRED)

# pybind11
find_package(pybind11 CONFIG QUIET)
if(NOT pybind11_FOUND)
    # parse pyproject.toml to get pybind11 version
    execute_process(
        COMMAND ${UV_EXECUTABLE} run --no-project --with toml cmake/parse_pyproject_toml.py pybind11
        WORKING_DIRECTORY ${_NIAS_CPP_DIR}
        OUTPUT_VARIABLE PYBIND11_VERSION
        RESULT_VARIABLE _pybind11_version_result)

    string(FIND "${PYBIND11_VERSION}" "Error:" _pybind11_error_in_result)
    if(_pybind11_version_result OR _pybind11_error_in_result GREATER -1)
        set(_message "Parsing pyproject.toml to get pybind11 version failed with the following error:\n")
        if(_pybind11_version_result)
            set(_message "${_message}${_pybind11_version_result}\n")
        endif()
        if(_pybind11_error_in_result GREATER -1)
            set(_message "${_message}${PYBIND11_VERSION}\n")
        endif()
        set(PYBIND11_VERSION 2.13.6)
        set(_message "${_message}Setting pybind11 version to default value ${PYBIND11_VERSION}.")
        message(WARNING "${_message}")
    endif()

    # add pybind11
    include(FetchContent)
    find_package(
        Python
        COMPONENTS Interpreter Development
        REQUIRED)
    set(PYBIND11_FINDPYTHON ON)
    FetchContent_Declare(
        pybind11
        GIT_REPOSITORY https://github.com/pybind/pybind11
        GIT_TAG "v${PYBIND11_VERSION}"
        OVERRIDE_FIND_PACKAGE)
    FetchContent_MakeAvailable(pybind11)
endif()

# add nias_cpp library target
function(nias_cpp_build_library)
    set(target_name nias_cpp)
    if(TARGET ${target_name})
        return()
    endif()

    # find C++ source files
    file(
        GLOB_RECURSE library_sources
        LIST_DIRECTORIES false
        "${_NIAS_CPP_DIR}/src/*.h" "${_NIAS_CPP_DIR}/src/*.cpp")

    find_package(pybind11 CONFIG REQUIRED)
    pybind11_add_module(${target_name} SHARED ${ARG_UNPARSED_ARGUMENTS} ${library_sources})

    target_include_directories(${target_name} PUBLIC $<BUILD_INTERFACE:${_NIAS_CPP_DIR}/src>
                                                     $<INSTALL_INTERFACE:${NIAS_CPP_INCLUDE_INSTALL_DIR}>)
    target_include_directories(${target_name} PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
                                                     $<INSTALL_INTERFACE:${NIAS_CPP_INCLUDE_INSTALL_DIR}>)

    target_link_libraries(${target_name} PUBLIC pybind11::pybind11 pybind11::embed)
    set_target_properties(${target_name} PROPERTIES LINKER_LANGUAGE CXX)
endfunction()

nias_cpp_build_library()
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

export(
    TARGETS nias_cpp
    NAMESPACE nias_cpp::
    FILE "${PROJECT_BINARY_DIR}/nias_cpp-targets.cmake")
export(PACKAGE nias_cpp)
configure_file("${_NIAS_CPP_DIR}/cmake/nias_cpp-config.cmake.in"
               "${PROJECT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/nias_cpp-config.cmake" @ONLY)
install(FILES "${PROJECT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/nias_cpp-config.cmake"
        DESTINATION "${NIAS_CPP_REL_CMAKE_INSTALL_DIR}")

# installation rules
install(
    DIRECTORY src/
    DESTINATION "${NIAS_CPP_INCLUDE_INSTALL_DIR}"
    PATTERN "*.py" EXCLUDE)

install(DIRECTORY cmake/ DESTINATION "${NIAS_CPP_REL_CMAKE_INSTALL_DIR}")

install(
    TARGETS nias_cpp
    EXPORT nias_cpp-targets
    LIBRARY DESTINATION nias_cpp)

install(
    EXPORT nias_cpp-targets
    NAMESPACE nias_cpp::
    DESTINATION ${NIAS_CPP_REL_CMAKE_INSTALL_DIR})

install(FILES "${_NIAS_CPP_DIR}/pyproject.toml" DESTINATION nias_cpp)

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

# add tests if this is the master project
if(NIAS_CPP_MASTER_PROJECT)
    # enable warnings in debug mode
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        set(CMAKE_CXX_FLAGS_DEBUG
            "${CMAKE_CXX_FLAGS_DEBUG} -Wall -Wextra -Wpedantic -Werror -Wsign-conversion")
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
        set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /W4 /WX")
    endif()
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DDEBUG")

    # add boost-ext/ut testing framework
    include(FetchContent)
    FetchContent_Declare(
        ut
        GIT_REPOSITORY https://github.com/boost-ext/ut
        GIT_TAG 69d500f3dc51eb020b7a91d8349cf9749a54b0b9
        OVERRIDE_FIND_PACKAGE)
    FetchContent_MakeAvailable(ut)
    find_package(ut REQUIRED)

    # add tests
    include(CTest)
    add_subdirectory(tests)

    # add header checks
    list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
    include(Headercheck)
    nias_cpp_add_headercheck()
endif()
