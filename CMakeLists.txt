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

# add pybind11
include(FetchContent)
FetchContent_Declare(
    pybind11
    GIT_REPOSITORY https://github.com/pybind/pybind11
    GIT_TAG v2.13.6
    OVERRIDE_FIND_PACKAGE)
FetchContent_MakeAvailable(pybind11)
find_package(pybind11 REQUIRED)

# find our own package
find_package(nias_cpp PATHS ${CMAKE_CURRENT_SOURCE_DIR}/cmake NO_DEFAULT_PATH)

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
