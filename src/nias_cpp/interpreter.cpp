#include "interpreter.h"

#include <pybind11/embed.h>

namespace nias
{


void ensure_interpreter_is_running()
{
    static auto interpreter = pybind11::scoped_interpreter{};
}


}  // namespace nias
