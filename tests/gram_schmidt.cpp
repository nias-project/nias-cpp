#include <complex>
#include <concepts>
#include <iostream>
#include <memory>
#include <string_view>
#include <tuple>
#include <vector>

#include <nias_cpp/algorithms/gram_schmidt.h>
#include <nias_cpp/concepts.h>
#include <nias_cpp/interfaces/vector.h>
#include <nias_cpp/interfaces/vectorarray.h>
#include <nias_cpp/interpreter.h>
#include <nias_cpp/type_traits.h>
#include <nias_cpp/vectorarray/list.h>
#include <nias_cpp/vectorarray/numpy.h>
#include <pybind11/numpy.h>

#include "boost_ext_ut_no_module.h"
#include "test_vector.h"

namespace
{
template <class F>
void print(const std::vector<std::shared_ptr<nias::VectorInterface<F>>>& vecs, std::string_view name)
{
    std::cout << "======== " << name << " ========" << '\n';
    int i = 0;
    for (auto& vec : vecs)
    {
        std::cout << "vec" << i++ << ": " << *vec << '\n';
    }

    // print result
    std::cout << "Inner products: " << '\n';
    for (auto&& vec1 : vecs)
    {
        for (auto&& vec2 : vecs)
        {
            std::cout << nias::dot_product(*vec1, *vec2) << " ";
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

    // Create some input vectors and print them
    const std::vector<std::shared_ptr<VectorInterface<F>>> vectors{
        std::shared_ptr<VectorInterface<F>>(new DynamicVector{F(1.), F(2.), F(3.)}),
        std::shared_ptr<VectorInterface<F>>(new DynamicVector{F(4.), F(5.), F(6.)}),
        std::shared_ptr<VectorInterface<F>>(new DynamicVector{F(7.), F(8.), F(9.)})};
    print(vectors, "Input");
    std::cout << "\n";

    // Perform Gram-Schmidt orthogonalization and print result
    auto vec_array = std::make_shared<ListVectorArray<F>>(vectors, 3);
    auto orthonormalized_vectorarray = nias::gram_schmidt(vec_array);
    print(orthonormalized_vectorarray->vectors(), "Output");
    // in-place
    nias::gram_schmidt_in_place(vec_array);
    std::cout << "\n";
    print(vec_array->vectors(), "Output in-place");
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
