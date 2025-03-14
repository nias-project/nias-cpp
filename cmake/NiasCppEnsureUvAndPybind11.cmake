macro(ENSURE_UV_AND_PYBIND11_ARE_AVAILABLE)

    # make sure uv is available to run the python script for parsing pyproject.toml
    # first try to find uv in the environment
    find_program(UV_EXECUTABLE NAMES uv uv.exe)
    # if we could not find uv, download it
    if(NOT UV_EXECUTABLE)
        include(FetchContent)
        set(_uv_version "0.6.6")
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

endmacro()
