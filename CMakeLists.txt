# This file is part of the NiAS project (https://github.com/nias-project).
# Copyright NiAS developers and contributors. All rights reserved.
# License: BSD 2-Clause License (https://opensource.org/licenses/BSD-2-Clause)

cmake_minimum_required(VERSION 3.15...3.27)

project(
    ${SKBUILD_PROJECT_NAME}
    VERSION ${SKBUILD_PROJECT_VERSION}
    LANGUAGES CXX)

find_package(Python REQUIRED COMPONENTS Interpreter Development.Module)
find_package(pybind11 CONFIG REQUIRED)

python_add_library(dummy MODULE src/nias_cpp/dummy.cpp WITH_SOABI)
target_link_libraries(dummy PRIVATE pybind11::headers)

install(TARGETS dummy DESTINATION nias_cpp)
