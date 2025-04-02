#include "bindings.h"

#include <complex>
#include <vector>

#include <nias_cpp/indices.h>
#include <nias_cpp/type_traits.h>
#include <pybind11/complex.h>  // IWYU pragma: keep
#include <pybind11/pybind11.h>
#include <pybind11/pytypes.h>
#include <pybind11/stl.h>  // IWYU pragma: keep

PYBIND11_MODULE(nias_cpp_bindings, m)
{
    namespace py = pybind11;

    m.doc() = "nias-cpp bindings";  // optional module docstring

    // bind Indices class
    pybind11::class_<nias::Indices>(m, "Indices")
        .def(py::init<>())
        .def(py::init<ssize_t>())
        .def(py::init<const std::vector<ssize_t>>())
        .def(py::init<const py::slice&>())
        .def(py::init(
            [](const py::list& indices)
            {
                std::vector<ssize_t> indices_vec;
                for (auto index : indices)
                {
                    indices_vec.push_back(index.cast<ssize_t>());
                }
                return nias::Indices(indices_vec);
            }))
        .def("size", &nias::Indices::size)
        .def("get", &nias::Indices::get)
        .def("as_vec", &nias::Indices::as_vec);

    // for some reason, the Indices constructors are not enough for implicit conversions
    py::implicitly_convertible<py::list, nias::Indices>();
    py::implicitly_convertible<py::slice, nias::Indices>();

    nias::bind_vector_interface<float>(m, "FloatVectorInterface");
    nias::bind_vector_interface<double>(m, "DoubleVectorInterface");
    nias::bind_vector_interface<long double>(m, "LongDoubleVectorInterface");
    nias::bind_vector_interface<std::complex<float>>(m, "ComplexFloatVectorInterface");
    nias::bind_vector_interface<std::complex<double>>(m, "ComplexDoubleVectorInterface");
    nias::bind_vector_interface<std::complex<long double>>(m, "ComplexLongDoubleVectorInterface");

    nias::bind_vectorarray_interface<float>(m, "Float");
    nias::bind_vectorarray_interface<double>(m, "Double");
    nias::bind_vectorarray_interface<long double>(m, "LongDouble");
    nias::bind_vectorarray_interface<std::complex<float>>(m, "ComplexFloat");
    nias::bind_vectorarray_interface<std::complex<double>>(m, "ComplexDouble");
    nias::bind_vectorarray_interface<std::complex<long double>>(m, "ComplexLongDouble");

    nias::bind_function_based_inner_product<float>(m, "Float");
    nias::bind_function_based_inner_product<double>(m, "Double");
    nias::bind_function_based_inner_product<long double>(m, "LongDouble");
    nias::bind_function_based_inner_product<std::complex<float>>(m, "ComplexFloat");
    nias::bind_function_based_inner_product<std::complex<double>>(m, "ComplexDouble");
    nias::bind_function_based_inner_product<std::complex<long double>>(m, "ComplexLongDouble");

    nias::bind_cpp_gram_schmidt<float>(m, "float");
    nias::bind_cpp_gram_schmidt<double>(m, "double");
    nias::bind_cpp_gram_schmidt<long double>(m, "long_double");
}
