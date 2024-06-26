cmake_minimum_required(VERSION 3.15)
project(nias-cpp LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)

# If CMAKE_CXX_VISIBILITY_PRESET is not set, pybind11 will set the visibility for the library to hidden
set(CMAKE_CXX_VISIBILITY_PRESET default)

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

# add pybind11
add_subdirectory(extern/pybind11)

# find our own package
find_package(nias-cpp PATHS ${CMAKE_CURRENT_SOURCE_DIR}/cmake NO_DEFAULT_PATH)

# add tests if this is the master project
if(NIAS_CPP_MASTER_PROJECT)
    add_subdirectory(tests)
endif()
