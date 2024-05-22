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

# If CMAKE_CXX_VISIBILITY_PRESET is not set, pybind11 will set the visibility for the library to hidden
set(CMAKE_CXX_VISIBILITY_PRESET default)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# We want to include the tests below only if nias-cpp is the master project
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    set(NIAS_CPP_MASTER_PROJECT ON)
else()
    set(NIAS_CPP_MASTER_PROJECT OFF)
endif()

# set build type to Release if not specified
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    message(STATUS "nias-cpp: setting build type to 'Release' as none was specified.")
    set(CMAKE_BUILD_TYPE
        Release
        CACHE STRING "Choose the type of build." FORCE)
endif()

# enable warnings for gcc and clang
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall -Wextra -Wpedantic")
endif()

find_package(Python REQUIRED COMPONENTS Interpreter Development.Module)

# add pybind11
add_subdirectory(extern/pybind11)

# add dummy module
python_add_library(dummy MODULE src/nias_cpp/dummy.cpp WITH_SOABI)
target_link_libraries(dummy PRIVATE pybind11::headers)
install(TARGETS dummy DESTINATION nias_cpp)

# find our own package
find_package(nias-cpp PATHS ${CMAKE_CURRENT_SOURCE_DIR}/cmake NO_DEFAULT_PATH)

# add tests if this is the master project
if(NIAS_CPP_MASTER_PROJECT)
    add_subdirectory(tests)
endif()
