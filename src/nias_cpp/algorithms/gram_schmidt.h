#ifndef NIAS_CPP_ALGORITHMS_GRAM_SCHMIDT_H
#define NIAS_CPP_ALGORITHMS_GRAM_SCHMIDT_H

#include <limits>
#include <memory>
#include <vector>

#include <nias_cpp/checked_integer_cast.h>
#include <nias_cpp/concepts.h>
#include <nias_cpp/inner_products/euclidean.h>
#include <nias_cpp/interfaces/inner_products.h>
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
std::shared_ptr<ListVectorArray<F>> gram_schmidt(
    const std::shared_ptr<ListVectorArray<F>>& vec_array,
    const std::shared_ptr<InnerProductInterface<F>>& inner_product =
        std::make_shared<EuclideanInnerProduct<F>>())
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
    const py::object result =
        NiasGramSchmidt(nias_vec_array, NiasCppInnerProduct(inner_product), "copy"_a = true);

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
void gram_schmidt_in_place(const std::shared_ptr<ListVectorArray<F>>& vec_array,
                           const std::shared_ptr<InnerProductInterface<F>>& inner_product =
                               std::make_shared<EuclideanInnerProduct<F>>())
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
    NiasGramSchmidt(nias_vec_array, NiasCppInnerProduct(inner_product), "copy"_a = false);
}

/**
 * \brief Simple C++ implementation of the Gram-Schmidt orthogonalization algorithm
 */
template <floating_point_or_complex F>
void gram_schmidt_cpp(VectorArrayInterface<F>& vec_array,
                      const std::shared_ptr<InnerProductInterface<F>>& inner_product =
                          std::make_shared<EuclideanInnerProduct<F>>())
{
    constexpr F atol = std::numeric_limits<F>::epsilon() * F(10);
    std::vector<bool> remove(as_size_t(vec_array.size()), false);
    for (ssize_t i = 0; i < vec_array.size(); ++i)
    {
        for (ssize_t j = 0; j < i; j++)
        {
            if (remove[as_size_t(j)])
            {
                continue;
            }
            F projection = inner_product->apply_pairwise(vec_array, vec_array, {i}, {j}).at(0) /
                           inner_product->apply_pairwise(vec_array, vec_array, {j}, {j}).at(0);
            vec_array.axpy(-projection, vec_array, {i}, {j});
        }
        const auto norm2 = inner_product->apply_pairwise(vec_array, vec_array, {i}, {i}).at(0);
        if (norm2 < atol)
        {
            remove[as_size_t(i)] = true;
        }
        else
        {
            vec_array.scal(F(1.) / std::sqrt(norm2), {i});
        }
    }
    std::vector<ssize_t> indices_to_remove;
    for (ssize_t i = 0; i < vec_array.size(); ++i)
    {
        if (remove[as_size_t(i)])
        {
            indices_to_remove.push_back(i);
        }
    }
    vec_array.delete_vectors(indices_to_remove);
}


}  // namespace nias

#endif  // NIAS_CPP_ALGORITHMS_GRAM_SCHMIDT_H
