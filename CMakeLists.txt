# This file is part of the NiAS project (https://github.com/nias-project).
# Copyright NiAS developers and contributors. All rights reserved.
# License: BSD 2-Clause License (https://opensource.org/licenses/BSD-2-Clause)

# Require CMake 3.15+ (matching scikit-build-core) Use new versions of all
# policies up to CMake 3.30
cmake_minimum_required(VERSION 3.15...3.30)
project(
    ${SKBUILD_PROJECT_NAME}
    VERSION ${SKBUILD_PROJECT_VERSION}
    LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

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

# installation rules
include(GNUInstallDirs)
set(NIAS_CPP_INSTALL_DIR
    "${CMAKE_INSTALL_PREFIX}/nias_cpp"
    CACHE PATH "")

set(NIAS_CPP_INCLUDE_INSTALL_DIR "${NIAS_CPP_INSTALL_DIR}/src")
set(NIAS_CPP_CMAKE_INSTALL_DIR "${NIAS_CPP_INSTALL_DIR}/cmake")

install(
    DIRECTORY src/
    DESTINATION "${NIAS_CPP_INCLUDE_INSTALL_DIR}"
    PATTERN "*.py" EXCLUDE)

install(DIRECTORY cmake/ DESTINATION "${NIAS_CPP_CMAKE_INSTALL_DIR}")

# enable warnings for gcc and clang
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall -Wextra -Wpedantic")
endif()

find_package(Python REQUIRED COMPONENTS Interpreter Development.Module)

# make sure uv is available to run the python script for parsing pyproject.toml
# first try to find uv in the environment
find_program(UV_EXECUTABLE uv)
# if we could not find uv, download it
if(NOT UV_EXECUTABLE)
    include(FetchContent)
    set(_uv_version "0.5.1")
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
find_program(UV_EXECUTABLE uv HINTS ${uv_BINARY_DIR} REQUIRED)

# parse pyproject.toml to get pybind11 version
execute_process(
    COMMAND ${UV_EXECUTABLE} run --no-project --with toml cmake/parse_pyproject_toml.py pybind11
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    OUTPUT_VARIABLE PYBIND11_VERSION)

# add pybind11
include(FetchContent)
FetchContent_Declare(
    pybind11
    GIT_REPOSITORY https://github.com/pybind/pybind11
    GIT_TAG "v${PYBIND11_VERSION}"
    OVERRIDE_FIND_PACKAGE)
FetchContent_MakeAvailable(pybind11)
find_package(pybind11 REQUIRED)

# find our own package
find_package(nias_cpp PATHS ${CMAKE_CURRENT_SOURCE_DIR}/cmake NO_DEFAULT_PATH)

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
    include(FetchContent)
    # add boost-ext/ut testing framework
    FetchContent_Declare(
        ut
        GIT_REPOSITORY https://github.com/boost-ext/ut
        GIT_TAG v2.0.0
        # CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}/extern/ut
        OVERRIDE_FIND_PACKAGE)
    FetchContent_MakeAvailable(ut)
    find_package(ut REQUIRED)
    add_subdirectory(tests)
endif()
