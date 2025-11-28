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
        VERSION 0.1.2
        LANGUAGES CXX)
endif()
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_POSITION_INDEPENDENT_CODE TRUE)

# We use nanobind's approach to Python handling:
# We move all Python-dependent code to the nias_cpp-config.cmake file in order
# to move Python detection to the find_package(nias_cpp) call in dependent projects.
# This way, we can be sure that we use the same Python as the downstream project.
# Even when nias-cpp is the main project, we can completely skip Python detection
# as long as the tests are disabled (for example in our nias_cpp vcpkg port).
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    set(NIAS_CPP_MASTER_PROJECT ON)
else()
    set(NIAS_CPP_MASTER_PROJECT OFF)
endif()
option(NIAS_CPP_ENABLE_TESTS "Enable building tests." ${NIAS_CPP_MASTER_PROJECT})

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

include(GNUInstallDirs)
if(NOT DEFINED NIAS_CPP_INSTALL_DATADIR)
    set(NIAS_CPP_INSTALL_DATADIR "nias_cpp")
endif()
set(NIAS_CPP_INSTALL_SRC_DIR "${NIAS_CPP_INSTALL_DATADIR}/src")
set(NIAS_CPP_INSTALL_CMAKE_DIR "${NIAS_CPP_INSTALL_DATADIR}/cmake")

# installation rules
cmake_path(GET CMAKE_CURRENT_LIST_FILE PARENT_PATH _NIAS_CPP_DIR)
set(_NIAS_CPP_DIR
    ${_NIAS_CPP_DIR}
    CACHE INTERNAL "")

install(
    DIRECTORY src/
    DESTINATION "${NIAS_CPP_INSTALL_SRC_DIR}"
    PATTERN "*.py" EXCLUDE)

install(
    DIRECTORY src/
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
    FILES_MATCHING
    PATTERN "*.h")

install(DIRECTORY cmake/ DESTINATION "${NIAS_CPP_INSTALL_CMAKE_DIR}")

install(FILES "${_NIAS_CPP_DIR}/pyproject.toml" DESTINATION "${NIAS_CPP_INSTALL_DATADIR}")

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")
include(NiasCppManageDependencies)
get_version_from_pyproject_toml(Python NIAS_CPP_MIN_PYTHON_VERSION)
get_version_from_pyproject_toml(pybind11 NIAS_CPP_MIN_PYBIND11_VERSION)

configure_file("${_NIAS_CPP_DIR}/cmake/nias_cpp-config.cmake.in"
               "${PROJECT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/nias_cpp-config.cmake" @ONLY)
install(FILES "${PROJECT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/nias_cpp-config.cmake"
        DESTINATION "${NIAS_CPP_INSTALL_CMAKE_DIR}")

# return early if the user only wants to install nias-cpp
if(NIAS_CPP_MASTER_PROJECT AND NOT NIAS_CPP_ENABLE_TESTS)
    return()
endif()

# ensure that uv, python, and pybind11 are available
ensure_python_is_available()
ensure_pybind11_is_available()

include("${PROJECT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/nias_cpp-config.cmake")

install(
    TARGETS nias_cpp nias_cpp_bindings
    EXPORT nias_cpp-targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})

install(TARGETS nias_cpp nias_cpp_bindings LIBRARY DESTINATION ${NIAS_CPP_INSTALL_DATADIR})

install(
    EXPORT nias_cpp-targets
    NAMESPACE nias_cpp::
    DESTINATION ${NIAS_CPP_INSTALL_CMAKE_DIR})

# enable warnings in debug mode
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall -Wextra -Wpedantic -Werror -Wsign-conversion")
elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /W4 /WX /bigobj")
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
