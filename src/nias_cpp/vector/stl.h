#ifndef NIAS_CPP_VECTOR_STL_H
#define NIAS_CPP_VECTOR_STL_H

#include <vector>

#include <nias_cpp/checked_integer_cast.h>
#include <nias_cpp/concepts.h>
#include <nias_cpp/interfaces/vector.h>
#include <nias_cpp/type_traits.h>
#include <nias_cpp/vectorarray/list.h>

template <nias::floating_point_or_complex F>
struct nias::VectorTraits<std::vector<F>>
{
    using VectorType = std::vector<F>;
    using ScalarType = F;
    static constexpr auto dim_ = [](const VectorType& vec) -> ssize_t
    {
        return as_ssize_t(vec.size());
    };
    static constexpr auto copy_ = [](const VectorType& vec) -> VectorType
    {
        return vec;
    };
    static constexpr auto get_ = [](VectorType& vec, ssize_t i) -> F&
    {
        return vec[as_size_t(i)];
    };
    static constexpr auto const_get_ = [](const VectorType& vec, ssize_t i) -> const F&
    {
        return vec[as_size_t(i)];
    };
};


#endif  // NIAS_CPP_VECTOR_STL_H
