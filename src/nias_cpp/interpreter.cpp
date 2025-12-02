#include "interpreter.h"

#include <cstdlib>
#include <filesystem>
#include <mutex>

#include <boost/process/v2/environment.hpp>
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
        return python_lib_dir.parent_path().native();
    }();
#else
    static const auto pythonHome = python_lib_dir.native();
#endif
    // The issue linked above suggests to use Py_SetPythonHome which, however, is deprecated in Python 3.11+.
    // The suggested alternative is PyConfig.home (see https://docs.python.org/3/c-api/init.html#c.Py_SetPythonHome)
    // but using that seems to a bit more involved.
    // TODO: Figure out whether we can/should use PyConfig.home here instead of setting the environment variable.
    // PyConfig might also help us to set up the other paths that we set below using pybind11::exec.
    boost::process::v2::environment::set(boost::process::v2::environment::key("PYTHONHOME"),
                                         pythonHome.data());
    static auto interpreter = pybind11::scoped_interpreter{};
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
                       // check that Python versions match
                       pybind11::exec(R"(
                           with open(venv_path / 'pyvenv.cfg', 'r') as f:
                               for line in f:
                                   if line.startswith('version_info') or line.startswith('version'):
                                       venv_version = line.split('=')[1].strip()
                                       break
                           interpreter_version = sys.version.split()[0]
                           if venv_version != interpreter_version:
                               raise RuntimeError(f'Python versions (interpreter {interpreter_version}'
                                                  f' vs virtualenv {venv_version}) do not match!')
                           )");
                   });
}


}  // namespace nias
