#ifndef NIAS_CPP_GRAM_SCHMIDT_H
#define NIAS_CPP_GRAM_SCHMIDT_H

#include <functional>
#include <vector>

#include <nias-cpp/vector.h>
#include <pybind11/embed.h>

namespace nias
{


// python_name is the name under which V is exposed (via pybind11 bindings) on the Python side
template <NiasVector V>
std::vector<V> gram_schmidt(const std::vector<V>& vecs, const std::string python_name)
{
    // get the classes we need from the Python nias module
    namespace py = pybind11;
    py::module_ nias_cpp_vector = py::module::import("nias.bindings.nias_cpp.vector");
    auto NiasCppVector = nias_cpp_vector.attr("NiasCppVector");
    auto NiasCppListVectorSpace = nias_cpp_vector.attr("NiasCppListVectorSpace");
    auto NiasCppInnerProduct =
        py::module::import("nias.bindings.nias_cpp.product").attr("NiasCppInnerProduct");
    auto NiasGramSchmidt = py::module::import("nias.linalg.gram_schmidt").attr("gram_schmidt");

    // create space and and a vectorarray containing vecs
    assert(!vecs.empty());
    const auto dim = vecs[0].size();
    auto space = NiasCppListVectorSpace(dim, python_name);
    py::list py_vecs;
    for (const auto& vec : vecs)
    {
        py_vecs.append(space.attr("make_vector")(std::cref(vec)));
    }
    auto nias_vec_array = space.attr("from_vectors")(py_vecs);

    // execute the Python gram_schmidt function
    using namespace pybind11::literals;
    py::object result = NiasGramSchmidt(nias_vec_array, NiasCppInnerProduct(), "copy"_a = true);

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
VectorArray<V> gram_schmidt(const VectorArray<V>& vec_array)
{
    // get the classes we need from the Python nias module
    namespace py = pybind11;
    py::module_ nias_cpp_vectorarray = py::module::import("nias.bindings.nias_cpp.vectorarray");
    auto NiasVecArrayImpl = nias_cpp_vectorarray.attr("NiasCppVectorArrayImpl");
    auto NiasVecArray = nias_cpp_vectorarray.attr("NiasCppVectorArray");
    auto NiasCppInnerProduct =
        py::module::import("nias.bindings.nias_cpp.product").attr("NiasCppInnerProduct");
    auto NiasGramSchmidt = py::module::import("nias.linalg.gram_schmidt").attr("gram_schmidt");

    // create a Python VectorArray
    using namespace pybind11::literals;  // for the _a literal
    auto nias_vec_array = NiasVecArray("impl"_a = NiasVecArrayImpl(std::cref(vec_array)));

    // execute the Python gram_schmidt function
    py::object result = NiasGramSchmidt(nias_vec_array, NiasCppInnerProduct(), "copy"_a = true);

    // We could also just do
    // return result.attr("impl").attr("impl").cast<VectorArray<V>>();
    // but that would copy the VectorArray instead of moving it
    return std::move(*result.attr("impl").attr("impl").cast<VectorArray<V>*>());
}

template <NiasVector V>
void gram_schmidt_in_place(VectorArray<V>& vec_array)
{
    // get the classes we need from the Python nias module
    namespace py = pybind11;
    py::module_ nias_cpp_vectorarray = py::module::import("nias.bindings.nias_cpp.vectorarray");
    auto NiasVecArrayImpl = nias_cpp_vectorarray.attr("NiasCppVectorArrayImpl");
    auto NiasVecArray = nias_cpp_vectorarray.attr("NiasCppVectorArray");
    auto NiasCppInnerProduct =
        py::module::import("nias.bindings.nias_cpp.product").attr("NiasCppInnerProduct");
    auto NiasGramSchmidt = py::module::import("nias.linalg.gram_schmidt").attr("gram_schmidt");

    // create a Python VectorArray
    using namespace pybind11::literals;  // for the _a literal
    auto nias_vec_array = NiasVecArray("impl"_a = NiasVecArrayImpl(std::ref(vec_array)));

    // execute the Python gram_schmidt function
    NiasGramSchmidt(nias_vec_array, NiasCppInnerProduct(), "copy"_a = false);
}


}  // namespace nias

#endif  //NIAS_CPP_GRAM_SCHMIDT_H
