#include <iostream>
#include <vector>

#include <nias-cpp/gram_schmidt.h>
#include <nias-cpp/vector.h>

#include "test_module.h"

template <nias::NiasVector V>
void print(const std::vector<V>& vecs, std::string_view name)
{
    std::cout << "======== " << name << " ========" << std::endl;
    int i = 0;
    for (auto& vec : vecs)
    {
        std::cout << "vec" << i++ << ": ";
        for (auto& entry : vec)
        {
            std::cout << entry << " ";
        }
        std::cout << std::endl;
    }

    // print result
    std::cout << "Inner products: " << std::endl;
    for (auto&& vec1 : vecs)
    {
        for (auto&& vec2 : vecs)
        {
            std::cout << vec1.dot(vec2) << " ";
        }
        std::cout << std::endl;
    }
    std::cout << "=======================" << std::endl;
}

int main()
{
    using namespace nias;
    namespace py = pybind11;

    // Create some input vectors and print them
    std::vector<ExampleVector> vectors {{1., 2., 3.}, {4., 5., 6.}, {7., 8., 9.}};
    print(vectors, "Input");

    VectorArray<ExampleVector> vec_array(vectors, 3);

    // Start the interpreter and import the test module
    py::scoped_interpreter guard {};  // start the interpreter and keep it alive
    py::module_ nias_cpp_test_module = py::module::import("nias_cpp_test");

    // Perform Gram-Schmidt orthogonalization and print result
    std::cout << "\n\nPerforming Gram-Schmidt orthogonalization... ";
    auto orthonormalized_vectors = nias::gram_schmidt(vectors, "ExampleVector");
    auto orthonormalized_vectors_2 = nias::gram_schmidt(vec_array, "ExampleVector");
    std::cout << "done!\n\n" << std::endl;
    print(orthonormalized_vectors, "Output Vectors");
    print(orthonormalized_vectors_2, "Output VecArray");

    return 0;
}
