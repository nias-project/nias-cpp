#include <concepts>
#include <iostream>
#include <stdexcept>
#include <vector>

#include <nias_cpp/algorithms/gram_schmidt.h>
#include <nias_cpp/interfaces/vector.h>
#include <nias_cpp/interfaces/vectorarray.h>
#include <nias_cpp/interpreter.h>
#include <nias_cpp/vectorarray/list.h>
#include <nias_cpp/vectorarray/numpy.h>
#include <pybind11/numpy.h>

#include "test_module.h"

template <class V>
void print(const std::vector<std::shared_ptr<V>>& vecs, std::string_view name)
{
    std::cout << "======== " << name << " ========" << std::endl;
    int i = 0;
    for (auto& vec : vecs)
    {
        std::cout << "vec" << i++ << ": ";
        for (ssize_t i = 0; i < vec->dim(); ++i)
        {
            std::cout << vec->get(i) << " ";
        }
        std::cout << std::endl;
    }

    // print result
    std::cout << "Inner products: " << std::endl;
    for (auto&& vec1 : vecs)
    {
        for (auto&& vec2 : vecs)
        {
            std::cout << vec1->dot(*vec2) << " ";
        }
        std::cout << std::endl;
    }
    std::cout << "=======================" << std::endl;
}

template <nias::floating_point_or_complex F>
void print(const nias::VectorArrayInterface<F>& vec_array, std::string_view name)
{
    std::cout << "======== " << name << " ========" << std::endl;
    for (ssize_t i = 0; i < vec_array.size(); ++i)
    {
        for (ssize_t j = 0; j < vec_array.dim(); ++j)
        {
            std::cout << vec_array.get(i, j) << " ";
        }
        std::cout << std::endl;
    }
    std::cout << "Inner products: " << std::endl;
    for (ssize_t i = 0; i < vec_array.size(); ++i)
    {
        for (ssize_t j = 0; j < vec_array.size(); ++j)
        {
            std::cout << nias::dot_product(vec_array, vec_array, {i}, {j})[0] << " ";
        }
        std::cout << std::endl;
    }
}

void check(const bool condition, const std::string& message)
{
    if (!condition)
    {
        throw std::runtime_error(message);
    }
}

template <class F>
void test_gram_schmidt()
{
    using namespace nias;

    // Create some input vectors and print them
    std::vector<std::shared_ptr<VectorInterface<F>>> vectors {
        std::shared_ptr<VectorInterface<F>>(new DynamicVector {F(1.), F(2.), F(3.)}),
        std::shared_ptr<VectorInterface<F>>(new DynamicVector {F(4.), F(5.), F(6.)}),
        std::shared_ptr<VectorInterface<F>>(new DynamicVector {F(7.), F(8.), F(9.)})};
    print(vectors, "Input");

    // Perform Gram-Schmidt orthogonalization and print result
    auto vec_array = std::make_shared<ListVectorArray<F>>(vectors, 3);
    auto orthonormalized_vectorarray = nias::gram_schmidt(vec_array);
    print(orthonormalized_vectorarray->vectors(), "Output");
    // in-place
    nias::gram_schmidt_in_place(vec_array);
    print(vec_array->vectors(), "Output in-place");
}

template <std::floating_point F>
void test_numpy_vecarray()
{
    using namespace nias;
    ensure_interpreter_is_running();
    const ssize_t size = 3;
    const ssize_t dim = 4;

    // create array
    auto array = pybind11::array_t<F>({size, dim});
    std::iota(array.mutable_data(), array.mutable_data() + size * dim, 1);
    const NumpyVectorArray<F> vec_array(array);
    for (ssize_t i = 0; i < size; ++i)
    {
        for (ssize_t j = 0; j < dim; ++j)
        {
            check(vec_array.array().at(i, j) == F(i * dim + j + 1), "vec array has wrong entries");
        }
    }
    check(vec_array.dim() == 4, "vec_array.dim() != 4");
    check(vec_array.size() == 3, "vec_array.size() != 3");

    // check copy
    auto vec_array_copy = vec_array.copy();
    auto* numpy_vec_array_copy = dynamic_cast<NumpyVectorArray<F>*>(vec_array_copy.get());
    check(*numpy_vec_array_copy == vec_array, "vec_array_copy != vec_array");
    numpy_vec_array_copy->array().mutable_at(0, 0) = 42;
    check(vec_array.get(0, 0) == 1, "copy is not deep");

    // check copy with indices
    auto vec_array_copy_with_indices = vec_array.copy(Indices({0, 2}));
    auto* numpy_vec_array_copy_with_indices =
        dynamic_cast<NumpyVectorArray<F>*>(vec_array_copy_with_indices.get());
    check(vec_array_copy_with_indices->dim() == 4, "copy with indices results in wrong dim");
    check(vec_array_copy_with_indices->size() == 2, "copy with indices results in wrong size");
    for (ssize_t j = 0; j < dim; ++j)
    {
        check(vec_array_copy_with_indices->get(0, j) == vec_array.get(0, j),
              "copy with indices has wrong entries");
        check(vec_array_copy_with_indices->get(1, j) == vec_array.get(2, j),
              "copy with indices has wrong entries");
    }
    numpy_vec_array_copy_with_indices->array().mutable_at(0, 0) = 42;
    check(vec_array.get(0, 0) == 1, "copy is not deep");

    // check append (no remove_from_other, no indices)
    vec_array_copy = vec_array.copy();
    vec_array_copy_with_indices = vec_array.copy(Indices({0, 2}));
    vec_array_copy->append(*vec_array_copy_with_indices);
    std::vector<ssize_t> indices = {0, 1, 2, 0, 2};
    check(*dynamic_cast<const NumpyVectorArray<F>*>(vec_array_copy.get()) ==
              *dynamic_cast<const NumpyVectorArray<F>*>(vec_array.copy(indices).get()),
          "wrong entries after append");
    for (ssize_t i = 0; i < vec_array_copy->size(); ++i)
    {
        for (ssize_t j = 0; j < dim; ++j)
        {
            check(vec_array_copy->get(i, j) == vec_array.get(indices[i], j), "wrong entries after append");
        }
    }
    // check_append (no remove_from_other, with indices)
    auto vec_array_copy_2 = vec_array.copy();
    vec_array_copy_2->append(*vec_array.copy(), false, Indices({0, 2}));
    check(*dynamic_cast<const NumpyVectorArray<F>*>(vec_array_copy_2.get()) ==
              *dynamic_cast<const NumpyVectorArray<F>*>(vec_array_copy.get()),
          "wrong entries after append");

    // check append (remove_from_other, no indices)
    vec_array_copy = vec_array.copy();
    vec_array_copy_with_indices = vec_array.copy(Indices({0, 2}));
    vec_array_copy->append(*vec_array_copy_with_indices, true);
    check(*dynamic_cast<const NumpyVectorArray<F>*>(vec_array_copy.get()) ==
              *dynamic_cast<const NumpyVectorArray<F>*>(vec_array.copy(indices).get()),
          "wrong entries after append");
    check(vec_array_copy_with_indices->size() == 0, "wrong size after append");
    // check append (remove_from_other, with indices)
    vec_array_copy = vec_array.copy();
    vec_array_copy_with_indices = vec_array.copy(Indices({0, 2}));
    vec_array_copy->append(*vec_array_copy_with_indices, true, {1});
    check(*dynamic_cast<const NumpyVectorArray<F>*>(vec_array_copy.get()) ==
              *dynamic_cast<const NumpyVectorArray<F>*>(vec_array.copy(Indices({0, 1, 2, 2})).get()),
          "wrong entries after append");
    check(vec_array_copy_with_indices->size() == 1, "wrong size after append");
    for (ssize_t j = 0; j < dim; ++j)
    {
        check(vec_array_copy_with_indices->get(0, j) == vec_array.get(0, j), "wrong entries after append");
    }

    // check scal (scalar F, no indices)
    vec_array_copy = vec_array.copy();
    vec_array_copy->scal(F(2));
    for (ssize_t i = 0; i < vec_array_copy->size(); ++i)
    {
        for (ssize_t j = 0; j < dim; ++j)
        {
            check(vec_array_copy->get(i, j) == F(2) * vec_array.get(i, j), "wrong entries after scal");
        }
    }
    // check scal (scalar F, with indices)
    vec_array_copy = vec_array.copy();
    vec_array_copy->scal(F(2), {1});
    for (ssize_t i = 0; i < size; ++i)
    {
        for (ssize_t j = 0; j < dim; ++j)
        {
            const auto factor = i == 1 ? F(2) : F(1);
            check(vec_array_copy->get(i, j) == factor * vec_array.get(i, j), "wrong entries after scal");
        }
    }
    // check scal (vector F, no indices)
    vec_array_copy = vec_array.copy();
    vec_array_copy->scal({F(0), F(3), F(6)});
    for (ssize_t i = 0; i < size; ++i)
    {
        for (ssize_t j = 0; j < dim; ++j)
        {
            check(vec_array_copy->get(i, j) == F(3 * i) * vec_array.get(i, j), "wrong entries after scal");
        }
    }
    // check scal (vector F, with indices)
    vec_array_copy = vec_array.copy();
    vec_array_copy->scal({F(0), F(-1)}, Indices({1, 2}));
    for (ssize_t i = 0; i < size; ++i)
    {
        for (ssize_t j = 0; j < dim; ++j)
        {
            check(vec_array_copy->get(i, j) == (F(1) - F(i)) * vec_array.get(i, j),
                  "wrong entries after scal");
        }
    }

    // check axpy (scalar F, no indices)
    vec_array_copy = vec_array.copy();
    vec_array_copy->axpy(F(2), *vec_array.copy());
    for (ssize_t i = 0; i < size; ++i)
    {
        for (ssize_t j = 0; j < dim; ++j)
        {
            check(vec_array_copy->get(i, j) == F(3) * vec_array.get(i, j), "wrong entries after axpy");
        }
    }

    // check axpy (scalar F, indices)
    vec_array_copy = vec_array.copy();
    vec_array_copy->axpy(F(2), *vec_array.copy(), {1}, {2});
    for (ssize_t i = 0; i < size; ++i)
    {
        if (i == 1)
        {
            for (ssize_t j = 0; j < dim; ++j)
            {
                check(vec_array_copy->get(i, j) == vec_array.get(i, j) + F(2) * vec_array.get(2, j),
                      "wrong entries after axpy");
            }
        }
        else
        {
            for (ssize_t j = 0; j < dim; ++j)
            {
                check(vec_array_copy->get(i, j) == vec_array.get(i, j), "wrong entries after axpy");
            }
        }
    }

    // check axpy (vector F, no indices)
    vec_array_copy = vec_array.copy();
    vec_array_copy->axpy({F(1), F(2), F(3)}, *vec_array.copy());
    for (ssize_t i = 0; i < size; ++i)
    {
        for (ssize_t j = 0; j < dim; ++j)
        {
            check(vec_array_copy->get(i, j) == F(i + 2) * vec_array.get(i, j), "wrong entries after axpy");
        }
    }

    // check axpy (scalar F, indices)
    vec_array_copy = vec_array.copy();
    vec_array_copy->axpy({F(0.5), F(1.5)}, *vec_array.copy(), Indices({1, 2}), Indices({0, 1}));
    for (ssize_t j = 0; j < dim; ++j)
    {
        check(vec_array_copy->get(0, j) == vec_array.get(0, j), "wrong entries after axpy");
        check(vec_array_copy->get(1, j) == vec_array.get(1, j) + F(0.5) * vec_array.get(0, j),
              "wrong entries after axpy");
        check(vec_array_copy->get(2, j) == vec_array.get(2, j) + F(1.5) * vec_array.get(1, j),
              "wrong entries after axpy");
    }
}

template <std::floating_point F>
void test_cpp_gram_schmidt()
{
    using namespace nias;
    ensure_interpreter_is_running();
    const size_t size = 3;
    const size_t dim = 4;
    auto array = pybind11::array_t<F>({size, dim});
    std::iota(array.mutable_data(), array.mutable_data() + size * dim, 1);
    NumpyVectorArray<F> vec_array(array);
    print(vec_array, "Input");
    gram_schmidt_cpp(vec_array);
    print(vec_array, "Output");
}

int main()
{
    std::cout << "\n\n\n==========================\n";
    std::cout << "=== Testing gram_schmidt_cpp ===\n";
    std::cout << "================================\n" << std::endl;
    test_cpp_gram_schmidt<double>();
    // test NumpyVectorArray
    test_numpy_vecarray<double>();
    std::cout << "\n\n\n=====================================\n";
    std::cout << "=== Testing gram_schmidt (double) ===\n";
    std::cout << "=====================================\n" << std::endl;
    test_gram_schmidt<double>();
    std::cout << "\n\n\n==============================================\n";
    std::cout << "=== Testing gram_schmidt (complex<double>) ===\n";
    std::cout << "==============================================\n" << std::endl;
    test_gram_schmidt<std::complex<double>>();
    return 0;
}
