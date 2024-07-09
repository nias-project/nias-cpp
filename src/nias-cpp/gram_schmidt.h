#ifndef NIAS_CPP_GRAM_SCHMIDT_H
#define NIAS_CPP_GRAM_SCHMIDT_H

#include <iostream>

#include <nias-cpp/interpreter.h>
#include <nias-cpp/vector.h>
#include <pybind11/embed.h>

namespace nias
{


template <std::floating_point F>
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

template <std::floating_point F>
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


}  // namespace nias

#endif  //NIAS_CPP_GRAM_SCHMIDT_H
