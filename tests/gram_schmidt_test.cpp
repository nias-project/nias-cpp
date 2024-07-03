#include <iostream>
#include <vector>

#include <nias-cpp/gram_schmidt.h>
#include <nias-cpp/vector.h>
#include <pybind11/embed.h>

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
            std::cout << (*vec)[i] << " ";
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

int main()
{
    using namespace nias;

    // Start the interpreter
    namespace py = pybind11;
    // We would like to keep the user code free of pybind11 and thus start and finalize the interpreters in each function
    // (so in this example, we would like to start the interpreter in the gram_schmidt function and finalize it before
    // returning, and then start the interpreter again in the gram_schmidt_in_place function. However, this always results in segfaults
    // when trying to start the interpreter the second time.
    // Similar issue to https://github.com/pybind/pybind11/issues/1439. If I understand correctly, it should work when using
    // py::initialize_interpreter() and py::finalize_interpreter() instead of py::scoped_interpreter but that does not work for
    // me either.
    // Maybe https://github.com/pybind/pybind11/pull/4769 plays a role here, too.
    py::scoped_interpreter guard {};

    // Create some input vectors and print them
    std::vector<std::shared_ptr<VectorInterface<double>>> vectors {
        std::shared_ptr<VectorInterface<double>>(new ExampleVector {1., 2., 3.}),
        std::shared_ptr<VectorInterface<double>>(new ExampleVector {4., 5., 6.}),
        std::shared_ptr<VectorInterface<double>>(new ExampleVector {7., 8., 9.})};
    print(vectors, "Input");

    // Perform Gram-Schmidt orthogonalization and print result
    auto vec_array = std::make_shared<ListVectorArray<double>>(vectors, 3);
    auto orthonormalized_vectorarray = nias::gram_schmidt(vec_array);
    print(orthonormalized_vectorarray->vectors(), "Output");
    // in-place
    nias::gram_schmidt_in_place(vec_array);
    print(vec_array->vectors(), "Output in-place");
    return 0;
}
