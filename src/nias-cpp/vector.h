#ifndef NIAS_CPP_VECTOR_H
#define NIAS_CPP_VECTOR_H

#include <concepts>
#include <vector>

#include <pybind11/pybind11.h>

namespace nias
{


// C++20 concept for a vector of doubles which has copy, dot, scal, and axpy methods
template <class V>
concept NiasVector = requires(V v) {
    { v.size() } -> std::convertible_to<std::size_t>;
    { v.copy() } -> std::same_as<V>;
    { v.dot(v) } -> std::convertible_to<double>;
    { v.scal(1.0) };
    { v.axpy(1.0, v) };
};

template <NiasVector V>
auto bind_nias_vector(pybind11::module& m, const std::string name)
{
    namespace py = pybind11;
    auto ret = py::class_<V>(m, name.c_str())
                   .def("__len__",
                        [](const V& v)
                        {
                            return v.size();
                        })
                   .def("copy", &V::copy)
                   .def("dot", &V::dot)
                   .def("scal", &V::scal)
                   .def("axpy", &V::axpy);
    return ret;
}


}  // namespace nias

#endif  // NIAS_CPP_VECTOR_H
