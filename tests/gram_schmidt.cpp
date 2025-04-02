#include <complex>
#include <concepts>
#include <iostream>
#include <string_view>
#include <tuple>
#include <vector>

#include <nias_cpp/algorithms/gram_schmidt.h>
#include <nias_cpp/concepts.h>
#include <nias_cpp/exceptions.h>
#include <nias_cpp/inner_products/function_based.h>
#include <nias_cpp/interfaces/inner_products.h>
#include <nias_cpp/interfaces/vector.h>
#include <nias_cpp/interfaces/vectorarray.h>
#include <nias_cpp/interpreter.h>
#include <nias_cpp/type_traits.h>
#include <nias_cpp/vector/stl.h>
#include <nias_cpp/vector/traits.h>
#include <nias_cpp/vectorarray/list.h>
#include <nias_cpp/vectorarray/numpy.h>
#include <pybind11/cast.h>
#include <pybind11/numpy.h>

#include "boost_ext_ut_no_module.h"

namespace
{

template <nias::floating_point_or_complex F>
std::string print_scalar(const F& scalar)
{
    if constexpr (nias::complex<F>)
    {
        return std::format("({:.2f}, {:.2f})", scalar.real(), scalar.imag());
    }
    else
    {
        return std::format("{:.2f}", scalar);
    }
}

template <nias::floating_point_or_complex F>
std::string print_vec(const nias::VectorInterface<F>& vec)
{
    std::string ret = "[";
    for (ssize_t i = 0; i < vec.dim(); ++i)
    {
        ret += print_scalar(vec[i]);
        if (i != vec.dim() - 1)
        {
            ret += ", ";
        }
    }
    ret += "]";
    return ret;
}

template <class VectorType>
    requires nias::has_vector_traits<VectorType> && (!nias::derived_from_vector_interface<VectorType>)
std::string print_vec(const VectorType& vec)
{
    return print_vec(nias::VectorWrapper<VectorType>(vec));
}

template <class VectorType>
void print(const std::vector<VectorType>& vecs, std::string_view name)
{
    std::cout << "======== " << name << " ========" << '\n';
    int i = 0;
    for (auto& vec : vecs)
    {
        std::cout << "vec" << i++ << ": " << print_vec(vec) << '\n';
    }

    // print result
    std::cout << "Euclidean inner products: " << '\n';
    for (auto&& vec1 : vecs)
    {
        for (auto&& vec2 : vecs)
        {
            std::cout << nias::dot_product(vec1, vec2) << " ";
        }
        std::cout << '\n';
    }
}

template <nias::floating_point_or_complex F>
void print(const nias::VectorArrayInterface<F>& vec_array, std::string_view name)
{
    std::cout << "======== " << name << " ========" << '\n';
    std::cout << vec_array << '\n';
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
    using VectorType = std::vector<F>;

    // Create some input vectors and print them
    const std::vector<VectorType> vectors{VectorType{F(1.), F(2.), F(3.)}, VectorType{F(4.), F(5.), F(6.)},
                                          VectorType{F(7.), F(8.), F(9.)}};
    print(vectors, "Input");
    std::cout << "\n";

    // Perform Gram-Schmidt orthogonalization and print result
    auto vec_array = ListVectorArray<VectorType>(vectors, 3);
    auto orthonormalized_vectorarray = nias::gram_schmidt(vec_array);
    print(orthonormalized_vectorarray->vectors(), "Output");

    // Use our own inner product (a, b) = a^T * D * b where D = diag(1, 2, 3) is a diagonal matrix
    const auto vector_inner_product = [](const auto& lhs, const auto& rhs)
    {
        F ret = 0;
        if (lhs.dim() != rhs.dim())
        {
            throw nias::InvalidArgumentError("lhs and rhs must have the same dimension");
        }
        for (ssize_t i = 0; i < lhs.dim(); ++i)
        {
            if constexpr (complex<F>)
            {
                using R = typename F::value_type;
                ret += std::conj(lhs[i]) * F(R(i + 1), R(0)) * rhs[i];
            }
            else
            {
                ret += lhs[i] * F(i + 1) * rhs[i];
            }
        }
        return ret;
    };
    const auto inner_product = VectorFunctionBasedInnerProduct<F>(vector_inner_product);
    auto orthonormalized_vectorarray_2 = nias::gram_schmidt(vec_array, inner_product);
    print(orthonormalized_vectorarray_2->vectors(), "Output with custom inner product");

    using namespace pybind11::literals;  // for the _a literal
    auto orthonormalized_vectorarray_3 =
        nias::gram_schmidt(vec_array, inner_product, "offset"_a = 1, "check"_a = false);
    print(orthonormalized_vectorarray_3->vectors(), "Output with offset");

    // in-place
    auto vec_array_copy = vec_array.copy();
    nias::gram_schmidt_in_place(vec_array);
    std::cout << "\n";
    print(vec_array.vectors(), "Output in-place");

    auto& vec_array_copy_as_list = dynamic_cast<ListVectorArray<F>&>(*vec_array_copy);
    nias::gram_schmidt_in_place(vec_array_copy_as_list, inner_product, "offset"_a = 1, "check"_a = false);
    print(vec_array_copy_as_list.vectors(), "Output in-place with custom inner product and offset");
}

template <std::floating_point F>
void test_cpp_gram_schmidt()
{
    using namespace nias;
    ensure_interpreter_and_venv_are_active();
    const ssize_t size = 3;
    const ssize_t dim = 4;
    auto array = pybind11::array_t<F>({size, dim});
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    std::iota(array.mutable_data(), array.mutable_data() + (size * dim), F(1));
    NumpyVectorArray<F> vec_array(array);
    print(vec_array, "Input");
    std::cout << "\n";
    gram_schmidt_cpp(vec_array);
    print(vec_array, "Output");
}
}  // namespace

int main()
{
    // TODO: This test needs to find the nias_cpp library (nias_cpp.cpython-312-x86_64-linux-gnu.so on Linux) or
    // (e.g. if run from VSCode) it fails with errors like:
    // "Unable to convert call argument '0' of type 'std::shared_ptr<nias::ListVectorArray<float> >' to Python object"
    // Copying the library from the build to the tests folder (build/tests) fixes the problem.
    using namespace boost::ut;
    using namespace nias;

    "gram_schmidt_cpp"_test = []<std::floating_point F>()
    {
        std::cout << "=============================================================================\n";
        std::cout << "Running Gram-Schmidt (C++ implementation) for type: " << reflection::type_name<F>()
                  << "\n";
        std::cout << "=============================================================================\n\n";
        test_cpp_gram_schmidt<F>();
        std::cout << "\n";
    } | std::tuple<float, double>{};

    "gram_schmidt"_test = []<floating_point_or_complex F>
    {
        std::cout << "=============================================================================\n";
        std::cout << "Running Gram-Schmidt (Python implementations) for type: " << reflection::type_name<F>()
                  << "\n";
        std::cout << "=============================================================================\n\n";
        test_gram_schmidt<F>();
        std::cout << "\n";
    } | std::tuple<float, double, std::complex<float>, std::complex<double>>{};
    return 0;
}
