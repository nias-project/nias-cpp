#ifndef NIAS_CPP_INTERPRETER_H
#define NIAS_CPP_INTERPRETER_H

namespace nias
{


// In theory, we could start and finalize the Python interpreter in each function
// However, this always results in segfaults when trying to start the interpreter the second time.
// Similar issue to https://github.com/pybind/pybind11/issues/1439.
// If I understand correctly, it should work when using py::initialize_interpreter() and
// py::finalize_interpreter() instead of py::scoped_interpreter but that does not work for
// me either.
// Maybe https://github.com/pybind/pybind11/pull/4769 plays a role here, too.
void ensure_interpreter_is_running();


}  // namespace nias

#endif  // NIAS_CPP_INTERPRETER_H
