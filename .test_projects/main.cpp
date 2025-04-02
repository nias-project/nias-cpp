#include <vector>

#include <nias_cpp/algorithms/gram_schmidt.h>
#include <nias_cpp/interfaces/vector.h>
#include <nias_cpp/vector/stl.h>
#include <nias_cpp/vectorarray/list.h>

int main()
{
    using namespace nias;

    // Create some input vectors
    using VectorType = std::vector<double>;
    const std::vector<VectorType> vectors{{1., 2., 3.}, {4., 5., 6.}, {7., 8., 9.}};

    // Perform Gram-Schmidt orthogonalization
    auto vec_array = ListVectorArray<VectorType>(vectors, 3);
    [[maybe_unused]] auto orthonormalized_vectorarray = gram_schmidt(vec_array);

    return 0;
}
