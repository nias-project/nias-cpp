macro(ENSURE_UV_IS_AVAILABLE)
    if(NOT UV_EXECUTABLE)
        set(NIAS_CPP_UV_DEFAULT_VERSION "0.8.22")

        # first try to find uv in the environment
        find_program(UV_EXECUTABLE NAMES uv uv.exe)

        # if we could not find uv, download it
        if(NOT UV_EXECUTABLE)
            set(_message "Could not find uv executable.")
            set(_message "${_message} Downloading and installing uv ${NIAS_CPP_UV_DEFAULT_VERSION}.")
            message(STATUS "nias-cpp: ${_message}")
            include(FetchContent)
            if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
                set(_uv_archive_name "uv-x86_64-pc-windows-msvc.zip")
            elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
                set(_uv_archive_name "uv-x86_64-unknown-linux-gnu.tar.gz")
            else()
                message(WARNING "Could not download uv binary, unsupported system: ${CMAKE_SYSTEM_NAME}.")
                message(WARNING "Please make sure uv is available in your environment.")
            endif()
            set(UV_DOWNLOAD_URL_BASE "https://github.com/astral-sh/uv/releases/download")
            FetchContent_Declare(
                uv URL "${UV_DOWNLOAD_URL_BASE}/${NIAS_CPP_UV_DEFAULT_VERSION}/${_uv_archive_name}")
            FetchContent_MakeAvailable(uv)
        endif()
        # now we should have uv
        find_program(
            UV_EXECUTABLE
            NAMES uv uv.exe
            HINTS ${uv_BINARY_DIR} REQUIRED)
        message(STATUS "nias-cpp: Using uv executable at ${UV_EXECUTABLE}")
    endif()
endmacro()

# parse pyproject.toml to get package version
# package must be either Python or be listed in project.dependencies in the pyproject.toml file
macro(GET_VERSION_FROM_PYPROJECT_TOML package_name version_output_variable)
    ensure_uv_is_available()
    execute_process(
        COMMAND ${UV_EXECUTABLE} run --no-project --with toml cmake/parse_pyproject_toml.py ${package_name}
        WORKING_DIRECTORY ${_NIAS_CPP_DIR}
        OUTPUT_VARIABLE ${version_output_variable}
        RESULT_VARIABLE _version_result)

    string(FIND "${version_output_variable}" "Error:" _error_in_result)
    if(_version_result OR _error_in_result GREATER -1)
        set(_message
            "Parsing pyproject.toml to get ${package_name} version failed with the following error:\n")
        if(_version_result)
            set(_message "${_message}${_version_result}\n")
        endif()
        if(_error_in_result GREATER -1)
            set(_message "${_message}${version_output_variable}\n")
        endif()
        if(NOT DEFINED NIAS_CPP_${package_name}_DEFAULT_VERSION)
            message(FATAL_ERROR "No default version for ${package_name} is defined.")
        endif()
        set(_default_version ${NIAS_CPP_${package_name}_DEFAULT_VERSION})
        set(${version_output_variable} _default_version)
        set(_message "${_message}Setting ${package_name} version to default value ${_default_version}.")
        message(WARNING "${_message}")
    endif()
endmacro()

macro(ENSURE_PYTHON_IS_AVAILABLE)
    set(NIAS_CPP_PYTHON_DEFAULT_VERSION "3.13")
    get_version_from_pyproject_toml(Python NIAS_CPP_MIN_PYTHON_VERSION)
    find_package(Python ${NIAS_CPP_MIN_PYTHON_VERSION} COMPONENTS Interpreter Development)
    if(NOT Python_FOUND)
        set(_message "Could not find Python >=${NIAS_CPP_MIN_PYTHON_VERSION}")
        set(_message "${_message} with components Interpreter Development.")
        message(STATUS "nias-cpp: ${_message}")
        message(
            STATUS "nias-cpp: Installing default Python version ${NIAS_CPP_PYTHON_DEFAULT_VERSION} using uv")
        # install Python using uv
        execute_process(
            COMMAND ${UV_EXECUTABLE} install python ${NIAS_CPP_PYTHON_DEFAULT_VERSION} --quiet
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            RESULT_VARIABLE _python_install_result)
        set(_message "Installing Python ${NIAS_CPP_PYTHON_DEFAULT_VERSION} using uv failed")
        set(_message "${_message} with exit code ${_python_install_result}.")
        if(_python_install_result)
            message(FATAL_ERROR _message)
        endif()
        set(_python_version ${NIAS_CPP_PYTHON_DEFAULT_VERSION})
        find_package(
            Python ${_python_version} EXACT
            COMPONENTS Interpreter Development
            REQUIRED)
    endif()
    message(STATUS "nias-cpp: Using Python version ${Python_VERSION}")
endmacro()

macro(ENSURE_PYBIND11_IS_AVAILABLE)
    get_version_from_pyproject_toml(pybind11 NIAS_CPP_PYBIND11_VERSION)
    set(PYBIND11_FINDPYTHON ON)
    find_package(pybind11 ${NIAS_CPP_PYBIND11_VERSION} EXACT CONFIG QUIET)
    if(NOT pybind11_FOUND)
        # add pybind11
        include(FetchContent)
        FetchContent_Declare(
            pybind11
            GIT_REPOSITORY https://github.com/pybind/pybind11
            GIT_TAG "v${NIAS_CPP_PYBIND11_VERSION}"
            OVERRIDE_FIND_PACKAGE)
        FetchContent_MakeAvailable(pybind11)
    endif()
endmacro()
