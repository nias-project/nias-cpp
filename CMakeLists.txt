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

# find our own package
find_package(nias_cpp PATHS ${CMAKE_CURRENT_SOURCE_DIR}/cmake NO_DEFAULT_PATH)

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
