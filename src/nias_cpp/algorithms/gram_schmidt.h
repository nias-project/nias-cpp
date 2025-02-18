#ifndef NIAS_CPP_GRAM_SCHMIDT_H
#define NIAS_CPP_GRAM_SCHMIDT_H

#include <iostream>
#include <limits>
#include <memory>
#include <stdexcept>
#include <vector>

#include <nias_cpp/checked_integer_cast.h>
#include <nias_cpp/concepts.h>
#include <nias_cpp/interfaces/vectorarray.h>
#include <nias_cpp/interpreter.h>
#include <nias_cpp/type_traits.h>
#include <nias_cpp/vectorarray/list.h>
#include <pybind11/cast.h>
#include <pybind11/pybind11.h>
#include <pybind11/pytypes.h>

namespace nias
{


/**
 * \brief Gram-Schmidt orthogonalization via Python
 *
 * Performs Gram-Schmidt orthogonalization on a ListVectorArray by calling the
 * Gram-Schmidt orthogonalization algorithm from the NiAS Python module. The
 * resulting orthogonalized vectors are stored in a new ListVectorArray, the
 * original array is not modified.
 *
 * \returns A new ListVectorArray containing the orthogonalized vectors.
 */
template <floating_point_or_complex F>
std::shared_ptr<ListVectorArray<F>> gram_schmidt(std::shared_ptr<ListVectorArray<F>> vec_array)
{
    ensure_interpreter_and_venv_are_active();

    // import nias_cpp module
    namespace py = pybind11;
    const py::module_ nias_cpp_module = py::module::import("nias_cpp");

    // get the classes we need from the Python nias module
    const py::module_ nias_cpp_vectorarray = py::module::import("nias.bindings.nias_cpp.vectorarray");
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
    return ret;
}

/**
 * \brief Gram-Schmidt orthogonalization via Python (in-place)
 *
 * Performs Gram-Schmidt orthogonalization on a ListVectorArray by calling the
 * Gram-Schmidt orthogonalization algorithm from the NiAS Python module. Directly
 * modifies the input ListVectorArray in-place.
 */
template <floating_point_or_complex F>
void gram_schmidt_in_place(std::shared_ptr<ListVectorArray<F>> vec_array)
{
    ensure_interpreter_and_venv_are_active();

    // import nias_cpp module
    namespace py = pybind11;
    const py::module_ nias_cpp_module = py::module::import("nias_cpp");

    // get the classes we need from the Python nias module
    const py::module_ nias_cpp_vectorarray = py::module::import("nias.bindings.nias_cpp.vectorarray");
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
}

/**
 * \brief Component-wise Euclidean dot product of two vector arrays
*/
template <floating_point_or_complex F>
std::vector<F> dot_product(const VectorArrayInterface<F>& lhs, const VectorArrayInterface<F>& rhs,
                           std::vector<ssize_t> lhs_indices, std::vector<ssize_t> rhs_indices)
{
    const auto lhs_size = lhs_indices.empty() ? lhs.size() : std::ssize(lhs_indices);
    const auto rhs_size = rhs_indices.empty() ? rhs.size() : std::ssize(rhs_indices);
    if (lhs_size != rhs_size || lhs.dim() != rhs.dim())
    {
        throw std::invalid_argument("lhs and rhs must have the same size and dimension");
    }
    std::vector<F> ret(checked_integer_cast<size_t>(lhs_size), F(0.));
    for (ssize_t i = 0; i < lhs_size; ++i)
    {
        for (ssize_t j = 0; j < lhs.dim(); ++j)
        {
            ret[checked_integer_cast<size_t>(i)] +=
                lhs.get(lhs_indices.empty() ? i : lhs_indices[checked_integer_cast<size_t>(i)], j) *
                rhs.get(rhs_indices.empty() ? i : rhs_indices[checked_integer_cast<size_t>(i)], j);
        }
    }
    return ret;
}

/**
 * \brief Simple C++ implementation of the Gram-Schmidt orthogonalization algorithm
 */
template <floating_point_or_complex F>
void gram_schmidt_cpp(VectorArrayInterface<F>& vec_array)
{
    constexpr F atol = std::numeric_limits<F>::epsilon() * F(10);
    std::vector<bool> remove(checked_integer_cast<size_t>(vec_array.size()), false);
    for (ssize_t i = 0; i < vec_array.size(); ++i)
    {
        for (ssize_t j = 0; j < i; j++)
        {
            if (remove[checked_integer_cast<size_t>(j)])
            {
                continue;
            }
            F projection = dot_product(vec_array, vec_array, {i}, {j})[0] /
                           dot_product(vec_array, vec_array, {j}, {j})[0];
            vec_array.axpy(-projection, vec_array, {i}, {j});
        }
        const auto norm2 = dot_product(vec_array, vec_array, {i}, {i})[0];
        if (norm2 < atol)
        {
            remove[checked_integer_cast<size_t>(i)] = true;
        }
        else
        {
            vec_array.scal(1. / std::sqrt(norm2), {i});
        }
    }
    std::vector<ssize_t> indices_to_remove;
    for (ssize_t i = 0; i < vec_array.size(); ++i)
    {
        if (remove[checked_integer_cast<size_t>(i)])
        {
            indices_to_remove.push_back(i);
        }
    }
    vec_array.delete_vectors(indices_to_remove);
}


}  // namespace nias

#endif  //NIAS_CPP_GRAM_SCHMIDT_H
