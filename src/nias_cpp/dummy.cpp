// This file is part of the NiAS project (https://github.com/nias-project).
// Copyright NiAS developers and contributors. All rights reserved.
// License: BSD 2-Clause License (https://opensource.org/licenses/BSD-2-Clause)

#include <pybind11/pybind11.h>

namespace py = pybind11;

PYBIND11_MODULE(dummy, m) {
  m.doc() = R"pbdoc(
        nias_cpp.dummy
        -----------------------

        .. currentmodule:: nias_cpp.dummy

        .. autosummary::
           :toctree: _generate
    )pbdoc";

  m.def(
      "hello_world", []() { return "Hello, World!"; }, R"pbdoc(
        Returns a string with the message "Hello, World!".)pbdoc");
}
