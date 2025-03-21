#include <memory>
#include <vector>

#include <nias_cpp/algorithms/gram_schmidt.h>
#include <nias_cpp/interfaces/vector.h>
#include <nias_cpp/vectorarray/list.h>

#include "test_vector.h"

int main()
{
    using namespace nias;
    // Create some input vectorsj
    const std::vector<std::shared_ptr<VectorInterface<double>>> vectors{
        std::shared_ptr<VectorInterface<double>>(new DynamicVector{1., 2., 3.}),
        std::shared_ptr<VectorInterface<double>>(new DynamicVector{4., 5., 6.}),
        std::shared_ptr<VectorInterface<double>>(new DynamicVector{7., 8., 9.})};

    // Perform Gram-Schmidt orthogonalization
    auto vec_array = ListVectorArray<double>(vectors, 3);
    [[maybe_unused]] auto orthonormalized_vectorarray = gram_schmidt(vec_array);

    return 0;
}
