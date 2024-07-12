#include <iostream>
#include <vector>

#include <nias_cpp/gram_schmidt.h>
#include <nias_cpp/vector.h>

#include "test_module.h"

template <class V>
void print(const std::vector<std::shared_ptr<V>>& vecs, std::string_view name)
{
    std::cout << "======== " << name << " ========" << std::endl;
    int i = 0;
    for (auto& vec : vecs)
    {
        std::cout << "vec" << i++ << ": ";
        for (size_t i = 0; i < vec->dim(); ++i)
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

template <class F>
void test_gram_schmidt()
{
    using namespace nias;

    // Create some input vectors and print them
    std::vector<std::shared_ptr<VectorInterface<F>>> vectors {
        std::shared_ptr<VectorInterface<F>>(new ExampleVector {F(1.), F(2.), F(3.)}),
        std::shared_ptr<VectorInterface<F>>(new ExampleVector {F(4.), F(5.), F(6.)}),
        std::shared_ptr<VectorInterface<F>>(new ExampleVector {F(7.), F(8.), F(9.)})};
    print(vectors, "Input");

    // Perform Gram-Schmidt orthogonalization and print result
    auto vec_array = std::make_shared<ListVectorArray<F>>(vectors, 3);
    auto orthonormalized_vectorarray = nias::gram_schmidt(vec_array);
    print(orthonormalized_vectorarray->vectors(), "Output");
    // in-place
    nias::gram_schmidt_in_place(vec_array);
    print(vec_array->vectors(), "Output in-place");
}

int main()
{
    std::cout << "\n\n\n======================\n";
    std::cout << "=== Testing double ===\n";
    std::cout << "======================\n" << std::endl;
    test_gram_schmidt<double>();
    std::cout << "\n\n\n===============================\n";
    std::cout << "=== Testing complex<double> ===\n";
    std::cout << "===============================\n" << std::endl;
    test_gram_schmidt<std::complex<double>>();
    return 0;
}
