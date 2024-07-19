#ifndef NIAS_CPP_GRAM_SCHMIDT_H
#define NIAS_CPP_GRAM_SCHMIDT_H

#include <cstddef>
#include <iostream>

#include <nias_cpp/interpreter.h>
#include <nias_cpp/vector.h>
#include <pybind11/embed.h>

namespace nias
{


template <floating_point_or_complex F>
std::shared_ptr<ListVectorArray<F>> gram_schmidt(std::shared_ptr<ListVectorArray<F>> vec_array)
{
    std::cout << "\n\nPerforming Gram-Schmidt orthogonalization... ";
    ensure_interpreter_is_running();

    // import nias_cpp module
    namespace py = pybind11;
    py::module_ nias_cpp_module = py::module::import("nias_cpp");

    // get the classes we need from the Python nias module
    py::module_ nias_cpp_vectorarray = py::module::import("nias.bindings.nias_cpp.vectorarray");
    auto NiasVecArrayImpl = nias_cpp_vectorarray.attr("NiasCppVectorArrayImpl");
    auto NiasVecArray = nias_cpp_vectorarray.attr("NiasCppVectorArray");
    auto NiasCppInnerProduct =
        py::module::import("nias.bindings.nias_cpp.product").attr("NiasCppInnerProduct");
    auto NiasGramSchmidt = py::module::import("nias.linalg.gram_schmidt").attr("gram_schmidt");

    // create a Python VectorArray
    using namespace pybind11::literals;  // for the _a literal
    auto nias_vec_array = NiasVecArray("impl"_a = NiasVecArrayImpl(vec_array));

    // execute the Python gram_schmidt function
    py::object result = NiasGramSchmidt(nias_vec_array, NiasCppInnerProduct(), "copy"_a = true);

    auto ret = result.attr("impl").attr("impl").cast<std::shared_ptr<ListVectorArray<F>>>();
    std::cout << "done\n\n" << std::endl;
    return ret;
}

template <floating_point_or_complex F>
void gram_schmidt_in_place(std::shared_ptr<ListVectorArray<F>> vec_array)
{
    std::cout << "\n\nPerforming in-place Gram-Schmidt orthogonalization... ";
    ensure_interpreter_is_running();

    // import nias_cpp module
    namespace py = pybind11;
    py::module_ nias_cpp_module = py::module::import("nias_cpp");

    // get the classes we need from the Python nias module
    py::module_ nias_cpp_vectorarray = py::module::import("nias.bindings.nias_cpp.vectorarray");
    auto NiasVecArrayImpl = nias_cpp_vectorarray.attr("NiasCppVectorArrayImpl");
    auto NiasVecArray = nias_cpp_vectorarray.attr("NiasCppVectorArray");
    auto NiasCppInnerProduct =
        py::module::import("nias.bindings.nias_cpp.product").attr("NiasCppInnerProduct");
    auto NiasGramSchmidt = py::module::import("nias.linalg.gram_schmidt").attr("gram_schmidt");

    // create a Python VectorArray
    using namespace pybind11::literals;  // for the _a literal
    auto nias_vec_array = NiasVecArray("impl"_a = NiasVecArrayImpl(vec_array));

    // execute the Python gram_schmidt function
    NiasGramSchmidt(nias_vec_array, NiasCppInnerProduct(), "copy"_a = false);
    std::cout << "done\n\n" << std::endl;
}

template <floating_point_or_complex F>
std::vector<F> dot_product(const VectorArrayInterface<F>& lhs, const VectorArrayInterface<F>& rhs,
                           std::vector<size_t> lhs_indices, std::vector<size_t> rhs_indices)
{
    const auto lhs_size = lhs_indices.empty() ? lhs.size() : lhs_indices.size();
    const auto rhs_size = rhs_indices.empty() ? rhs.size() : rhs_indices.size();
    if (lhs_size != rhs_size || lhs.dim() != rhs.dim())
    {
        throw std::invalid_argument("lhs and rhs must have the same size and dimension");
    }
    std::vector<F> ret(lhs_size, F(0.));
    for (size_t i = 0; i < lhs_size; ++i)
    {
        for (size_t j = 0; j < lhs.dim(); ++j)
        {
            ret[i] += lhs.get(lhs_indices.empty() ? i : lhs_indices[i], j) *
                      rhs.get(rhs_indices.empty() ? i : rhs_indices[i], j);
        }
    }
    return ret;
}

template <floating_point_or_complex F>
void gram_schmidt_cpp(VectorArrayInterface<F>& vec_array)
{
    constexpr F atol = 1e-15;
    std::vector<bool> remove(vec_array.size(), false);
    for (size_t i = 0; i < vec_array.size(); ++i)
    {
        for (size_t j = 0; j < i; j++)
        {
            if (remove[j])
            {
                continue;
            }
            F projection = dot_product(vec_array, vec_array, {i}, {j})[0] /
                           dot_product(vec_array, vec_array, {j}, {j})[0];
            vec_array.axpy(-projection, vec_array, {i}, {j});
        }
        const auto norm2 = dot_product(vec_array, vec_array, {i}, {i})[0];
        std::cout << i << ", " << norm2 << std::endl;
        if (norm2 < atol)
        {
            remove[i] = true;
        }
        else
        {
            vec_array.scal(1. / std::sqrt(norm2), {i});
        }
    }
    std::vector<size_t> indices_to_remove;
    for (size_t i = 0; i < vec_array.size(); ++i)
    {
        if (remove[i])
        {
            indices_to_remove.push_back(i);
        }
    }
    vec_array.delete_vectors(indices_to_remove);
}


}  // namespace nias

#endif  //NIAS_CPP_GRAM_SCHMIDT_H
