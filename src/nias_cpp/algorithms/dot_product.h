#ifndef NIAS_CPP_ALGORITHMS_DOT_PRODUCT_H
#define NIAS_CPP_ALGORITHMS_DOT_PRODUCT_H

#include <optional>
#include <stdexcept>
#include <vector>

#include <nias_cpp/checked_integer_cast.h>
#include <nias_cpp/concepts.h>
#include <nias_cpp/indices.h>
#include <nias_cpp/interfaces/vector.h>
#include <nias_cpp/interfaces/vectorarray.h>
#include <nias_cpp/type_traits.h>

namespace nias
{


/**
 * \brief Euclidean dot product of vectors
*/
template <floating_point_or_complex F>
F dot_product(const VectorInterface<F>& lhs, const VectorInterface<F>& rhs)
{
    if (lhs.dim() != rhs.dim())
    {
        throw std::invalid_argument("lhs and rhs must have the same size and dimension");
    }
    auto ret = F(0);
    for (ssize_t i = 0; i < lhs.dim(); ++i)
    {
        if constexpr (complex<F>)
        {
            ret += std::conj(lhs[i] * rhs[i]);
        }
        else
        {
            ret += lhs[i] * rhs[i];
        }
    }
    return ret;
}

/**
 * \brief Component-wise Euclidean dot product of two vector arrays
*/
template <floating_point_or_complex F>
std::vector<F> dot_product(const VectorArrayInterface<F>& lhs, const VectorArrayInterface<F>& rhs,
                           const std::optional<Indices>& lhs_indices = std::nullopt,
                           const std::optional<Indices>& rhs_indices = std::nullopt)
{
    if (lhs_indices || rhs_indices)
    {
        return dot_product(lhs[lhs_indices], rhs[rhs_indices], std::nullopt, std::nullopt);
    }
    if (lhs.size() != rhs.size() || lhs.dim() != rhs.dim())
    {
        throw std::invalid_argument("lhs and rhs must have the same size and dimension");
    }
    std::vector<F> ret(as_size_t(lhs.size()), F(0.));
    for (ssize_t i = 0; i < lhs.size(); ++i)
    {
        for (ssize_t k = 0; k < lhs.dim(); ++k)
        {
            if constexpr (complex<F>)
            {
                ret[as_size_t(i)] += std::conj(lhs.get(i, k)) * rhs.get(i, k);
            }
            else
            {
                ret[as_size_t(i)] += lhs.get(i, k) * rhs.get(i, k);
            }
        }
    }
    return ret;
}


}  // namespace nias

#endif  // NIASC_CPP_ALGORITHMS_DOT_PRODUCT_H
