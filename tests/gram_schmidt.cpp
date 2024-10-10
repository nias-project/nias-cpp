#include <complex>
#include <concepts>
#include <iostream>
#include <memory>
#include <string_view>
#include <vector>

#include <boost/ut.hpp>
#include <nias_cpp/algorithms/gram_schmidt.h>
#include <nias_cpp/concepts.h>
#include <nias_cpp/indices.h>
#include <nias_cpp/interfaces/vector.h>
#include <nias_cpp/interfaces/vectorarray.h>
#include <nias_cpp/interpreter.h>
#include <nias_cpp/type_traits.h>
#include <nias_cpp/vectorarray/list.h>
#include <nias_cpp/vectorarray/numpy.h>
#include <pybind11/numpy.h>

#include "test_module.h"

namespace
{
template <class V>
void print(const std::vector<std::shared_ptr<V>>& vecs, std::string_view name)
{
    std::cout << "======== " << name << " ========" << '\n';
    int i = 0;
    for (auto& vec : vecs)
    {
        std::cout << "vec" << i++ << ": ";
        for (ssize_t i = 0; i < vec->dim(); ++i)
        {
            std::cout << vec->get(i) << " ";
        }
        std::cout << '\n';
    }

    // print result
    std::cout << "Inner products: " << '\n';
    for (auto&& vec1 : vecs)
    {
        for (auto&& vec2 : vecs)
        {
            std::cout << vec1->dot(*vec2) << " ";
        }
        std::cout << '\n';
    }
    std::cout << "=======================" << '\n';
}

template <nias::floating_point_or_complex F>
void print(const nias::VectorArrayInterface<F>& vec_array, std::string_view name)
{
    std::cout << "======== " << name << " ========" << '\n';
    for (ssize_t i = 0; i < vec_array.size(); ++i)
    {
        for (ssize_t j = 0; j < vec_array.dim(); ++j)
        {
            std::cout << vec_array.get(i, j) << " ";
        }
        std::cout << '\n';
    }
    std::cout << "Inner products: " << '\n';
    for (ssize_t i = 0; i < vec_array.size(); ++i)
    {
        for (ssize_t j = 0; j < vec_array.size(); ++j)
        {
            std::cout << nias::dot_product(vec_array, vec_array, {i}, {j})[0] << " ";
        }
        std::cout << '\n';
    }
}

template <class F>
void test_gram_schmidt()
{
    using namespace nias;

    // Create some input vectors and print them
    const std::vector<std::shared_ptr<VectorInterface<F>>> vectors {
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
void test_cpp_gram_schmidt()
{
    using namespace nias;
    ensure_interpreter_is_running();
    const ssize_t size = 3;
    const ssize_t dim = 4;
    auto array = pybind11::array_t<F>({size, dim});
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    std::iota(array.mutable_data(), array.mutable_data() + (size * dim), 1);
    NumpyVectorArray<F> vec_array(array);
    print(vec_array, "Input");
    gram_schmidt_cpp(vec_array);
    print(vec_array, "Output");
}
}  // namespace

int main()
{
    using namespace boost::ut;
    using namespace nias;
    std::cout << "\n\n\n==========================\n";
    std::cout << "=== Testing gram_schmidt_cpp ===\n";
    std::cout << "================================\n" << '\n';
    "gram_schmidt_cpp"_test = []
    {
        test_cpp_gram_schmidt<double>();
    };
    std::cout << "\n\n\n=====================================\n";
    std::cout << "=== Testing gram_schmidt ===\n";
    std::cout << "=====================================\n" << '\n';
    "gram_schmidt"_test = []<floating_point_or_complex F>
    {
        test_gram_schmidt<F>();
    } | std::tuple<float, double, std::complex<float>, std::complex<double>> {};
    return 0;
}
