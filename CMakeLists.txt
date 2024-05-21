# Require CMake 3.15+ (matching scikit-build-core) Use new versions of all
# policies up to CMake 3.27
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
