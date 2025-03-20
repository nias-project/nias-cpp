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
        VERSION 0.1.1
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

# Use lld linker for MinGW on Windows.
# With the standard linker, GCC fails with lots of undefined references,
# and Clang fails with relocation errors.
if(MINGW)
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fuse-ld=lld")
endif()

# Disable pybind11's binary stripping for MinGW Clang on Windows since it fails
# with an "unexpected associative section index" error.
if(MINGW AND CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    set(NIAS_CPP_PYBIND11_NO_EXTRAS NO_EXTRAS)
endif()

get_filename_component(_NIAS_CPP_DIR "${CMAKE_CURRENT_LIST_FILE}" PATH)
set(_NIAS_CPP_DIR
    ${_NIAS_CPP_DIR}
    CACHE INTERNAL "")

include(GNUInstallDirs)
set(NIAS_CPP_INCLUDE_INSTALL_DIR ${CMAKE_INSTALL_INCLUDEDIR})
set(NIAS_CPP_REL_CMAKE_INSTALL_DIR "nias_cpp/cmake")

# ensure that pybind11 and uv are available
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")
include(NiasCppEnsureUvAndPybind11)
ensure_uv_and_pybind11_are_available()

include(NiasCppAddLibrary)
nias_cpp_add_library()

if(DEFINED ENV{VIRTUAL_ENV})
    set(NIAS_CPP_VENV_DIR $ENV{VIRTUAL_ENV})
    # TODO: check that nias_cpp is installed in the virtual environment
else()
    # if no virtualenv is active, we set up our own virtual environment to install our python dependencies
    find_package(
        Python
        COMPONENTS Interpreter
        REQUIRED)
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
    TARGETS nias_cpp nias_cpp_bindings
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
    TARGETS nias_cpp nias_cpp_bindings
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
if(NIAS_CPP_MASTER_PROJECT AND NOT NIAS_CPP_DISABLE_TESTS)
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
