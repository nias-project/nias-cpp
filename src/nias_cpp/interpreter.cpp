#include "interpreter.h"

#include <mutex>

#include <pybind11/embed.h>
#include <pybind11/eval.h>

namespace nias
{


void ensure_interpreter_and_venv_are_active()
{
    static auto interpreter = pybind11::scoped_interpreter{};
    // For the moment, we simply prepend the virtualenv module path to Python's module search path.
    // This seems to work fine for now but we have to ensure that the python version that is linked to pybind11
    // is the same as the one in the virtualenv. In the future, we might want to
    // - use the Python C API to choose the virtualenv Python interpreter, see https://docs.python.org/3/c-api/init_config.html
    // - make sure (in CMake) that pybind11 is linked to the Python version from the virtualenv
    static std::once_flag flag;
    std::call_once(flag,
                   [&]()
                   {
                       pybind11::exec("nias_cpp_venv_dir = '" NIAS_CPP_VENV_DIR "'");
                       pybind11::exec(R"(
                           import sys
                           import pathlib
                           venv_module_path = list(pathlib.Path(nias_cpp_venv_dir).rglob('site-packages'))
                           if len(venv_module_path) != 1:
                               raise RuntimeError('Could not find virtualenv module path')
                           venv_module_path = venv_module_path[0]
                           python_version = sys.version.split()[0]
                           python_version_wo_patch = '.'.join(python_version.split('.')[:-1])
                           if not f'python{python_version_wo_patch}' in str(venv_module_path):
                               raise RuntimeError('Python versions (interpreter vs virtualenv) do not match!')
                           sys.path.insert(0, str(venv_module_path))
                           )");
                       pybind11::exec(
                           "import sys\n"
                           "sys.path.insert(0, '" NIAS_CPP_BUILD_DIR "')");
                   });
}


}  // namespace nias
