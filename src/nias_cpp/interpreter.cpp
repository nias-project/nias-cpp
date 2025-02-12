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
                                   if line.startswith('version_info'):
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
