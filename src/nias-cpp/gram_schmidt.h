#ifndef NIAS_CPP_GRAM_SCHMIDT_H
#define NIAS_CPP_GRAM_SCHMIDT_H

#include <vector>

#include <nias-cpp/vector.h>
#include <pybind11/embed.h>

namespace nias
{


template <NiasVector V>
std::vector<V> gram_schmidt(const std::vector<V>& vecs, const std::string python_name)
{
    namespace py = pybind11;
    py::module_ nias_bindings = py::module::import("nias.bindings.nias_cpp.nias_cpp");

    // get the classes we need from the Python nias module
    auto NiasCppVector = nias_bindings.attr("NiasCppVector");
    auto NiasCppListVectorSpace = nias_bindings.attr("NiasCppListVectorSpace");
    auto NiasCppInnerProduct = nias_bindings.attr("NiasCppInnerProduct");
    auto NiasGramSchmidt = py::module::import("nias.linalg.gram_schmidt").attr("gram_schmidt");

    // create space and and a vectorarray containing vecs
    assert(!vecs.empty());
    auto dim = vecs[0].size();
    auto space = NiasCppListVectorSpace(dim, python_name);
    py::list py_vecs;
    for (const auto& vec : vecs)
    {
        py_vecs.append(space.attr("make_vector")(vec));
    }
    auto nias_vec_array = space.attr("from_vectors")(py_vecs);

    // execute the Python gram_schmidt function
    using namespace pybind11::literals;
    py::object result = NiasGramSchmidt(nias_vec_array, NiasCppInnerProduct(), "copy"_a = false);

    // convert result back to C++ vectors
    auto py_result = result.attr("vectors");
    std::vector<V> ret;
    for (auto& py_vec : py_result)
    {
        ret.emplace_back(py::cast<V>(py_vec.attr("impl")));
    }

    return ret;
}

template <NiasVector V>
std::vector<V> gram_schmidt(const VectorArray<V>& vec_array, const std::string python_name)
{
    namespace py = pybind11;
    py::module_ nias_bindings = py::module::import("nias.bindings.nias_cpp.nias_cpp");
    py::module_ nias_interfaces = py::module::import("nias.bindings.nias_cpp.interfaces");

    // get the classes we need from the Python nias module
    auto NiasVecArrayImpl = nias_interfaces.attr("NiasCppVectorArrayImpl");
    auto NiasVecArray = nias_interfaces.attr("NiasCppVectorArray");
    auto NiasCppInnerProduct = nias_bindings.attr("NiasCppInnerProduct");
    auto NiasGramSchmidt = py::module::import("nias.linalg.gram_schmidt").attr("gram_schmidt");

    using namespace pybind11::literals;

    // create space and and a vectorarray containing vecs
    auto nias_vec_array = NiasVecArray("impl"_a = NiasVecArrayImpl(vec_array));

    // execute the Python gram_schmidt function
    py::object result = NiasGramSchmidt(nias_vec_array, NiasCppInnerProduct(), "copy"_a = false);

    // convert result back to C++ vectors
    auto py_result = result.attr("vectors");
    std::vector<V> ret;
    for (auto& py_vec : py_result)
    {
        // ret.emplace_back(py::cast<V>(py_vec.attr("impl")));
        ret.emplace_back(py::cast<V>(py_vec));
    }

    return ret;
}


}  // namespace nias

#endif  //NIAS_CPP_GRAM_SCHMIDT_H
