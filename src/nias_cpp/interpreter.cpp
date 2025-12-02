#include "interpreter.h"

#include <cstdlib>
#include <filesystem>
#include <mutex>
#include <stdexcept>
#include <string>

// We intentionally do not include Python.h ourselves since it uses autolinking of the Python library
// which causes compilation failures on Windows with MSVC. pybind11 has machinery in place to avoid the
// autolinking issue.
// See https://discourse.paraview.org/t/debug-build-fail-cannot-open-file-python310-lib/9000/2
// We can use https://github.com/python/cpython/pull/19740 once our minimum required
// Python version is high enough.
#include <pybind11/embed.h>
#include <pybind11/eval.h>

namespace nias
{


void ensure_interpreter_and_venv_are_active()
{
    // We have to ensure that we use the same Python interpreter (version) as the one linked to pybind11.
    // See https://github.com/pybind/pybind11/issues/2369
    static auto python_lib_dir = std::filesystem::path(NIAS_CPP_PYTHON_LIBRARY_DIR);
#ifndef _WIN32
    // On Linux, the python library might be in a subfolder of the lib dir (e.g., /lib/x86_64-linux-gnu/libpython3.12.so)
    // so we cannot just take the parent path of the library location as PYTHONHOME. Instead, we search upwards until we find
    // the "lib" folder.
    static const auto pythonHome = []()
    {
        while (python_lib_dir.filename() != "lib" && python_lib_dir.has_parent_path())
        {
            python_lib_dir = python_lib_dir.parent_path();
        }
        return python_lib_dir.parent_path().string();
    }();
#else
    static const auto pythonHome = python_lib_dir.string();
#endif
    // Use PyConfig.home as recommended in Python 3.11+ instead of the deprecated Py_SetPythonHome
    // or setting the PYTHONHOME environment variable.
    // See https://docs.python.org/3/c-api/init.html#c.Py_SetPythonHome
    static auto interpreter = []()
    {
        PyConfig config;
        PyConfig_InitPythonConfig(&config);
        const PyStatus status = PyConfig_SetBytesString(&config, &config.home, pythonHome.c_str());
        if (PyStatus_Exception(status))
        {
            std::string error_msg = "Failed to set PyConfig.home";
            if (PyStatus_IsError(status) && status.err_msg != nullptr)
            {
                error_msg += ": ";
                error_msg += status.err_msg;
            }
            PyConfig_Clear(&config);
            throw std::runtime_error(error_msg);
        }
        // Note: pybind11::scoped_interpreter will call PyConfig_Clear internally after initialization
        return pybind11::scoped_interpreter(&config);
    }();
    static std::once_flag flag;
    std::call_once(flag,
                   [&]()
                   {
                       // insert build directory into python module search path
                       pybind11::exec(
                           "import sys\n"
                           "import pathlib\n"
                           "sys.path.insert(0, '" NIAS_CPP_BUILD_DIR "')");
                       // insert virtualenv module path into python module search path
                       pybind11::exec("nias_cpp_venv_dir = '" NIAS_CPP_VENV_DIR "'");
                       pybind11::exec(R"(
                           venv_path = pathlib.Path(nias_cpp_venv_dir)
                           venv_module_path = list(venv_path.rglob('site-packages'))
                           if len(venv_module_path) != 1:
                               raise RuntimeError('Could not find virtualenv module path')
                           venv_module_path = venv_module_path[0]
                           sys.path.insert(0, str(venv_module_path))
                           )");
                   });
}


}  // namespace nias
