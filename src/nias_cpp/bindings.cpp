#include <complex>
#include <vector>

#include <nias_cpp/bindings.h>
#include <nias_cpp/indices.h>
#include <nias_cpp/type_traits.h>
#include <pybind11/complex.h>  // IWYU pragma: keep
#include <pybind11/pybind11.h>
#include <pybind11/pytypes.h>
#include <pybind11/stl.h>  // IWYU pragma: keep

PYBIND11_MODULE(nias_cpp, m)
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

    nias::bind_nias_vectorinterface<float>(m, "FloatVectorInterface");
    nias::bind_nias_vectorinterface<double>(m, "DoubleVectorInterface");
    nias::bind_nias_vectorinterface<long double>(m, "LongDoubleVectorInterface");
    nias::bind_nias_vectorinterface<std::complex<float>>(m, "ComplexFloatVectorInterface");
    nias::bind_nias_vectorinterface<std::complex<double>>(m, "ComplexDoubleVectorInterface");
    nias::bind_nias_vectorinterface<std::complex<long double>>(m, "ComplexLongDoubleVectorInterface");

    nias::bind_nias_listvectorarray<float>(m, "Float");
    nias::bind_nias_listvectorarray<double>(m, "Double");
    nias::bind_nias_listvectorarray<long double>(m, "LongDouble");
    nias::bind_nias_listvectorarray<std::complex<float>>(m, "ComplexFloat");
    nias::bind_nias_listvectorarray<std::complex<double>>(m, "ComplexDouble");
    nias::bind_nias_listvectorarray<std::complex<long double>>(m, "ComplexLongDouble");

    nias::bind_cpp_gram_schmidt<float>(m, "float");
    nias::bind_cpp_gram_schmidt<double>(m, "double");
    nias::bind_cpp_gram_schmidt<long double>(m, "long_double");
}
