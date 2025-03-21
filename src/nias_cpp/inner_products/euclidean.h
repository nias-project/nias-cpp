#ifndef NIAS_CPP_INNER_PRODUCTS_EUCLIDEAN_H
#define NIAS_CPP_INNER_PRODUCTS_EUCLIDEAN_H

#include <nias_cpp/algorithms/dot_product.h>
#include <nias_cpp/concepts.h>
#include <nias_cpp/exceptions.h>
#include <nias_cpp/indices.h>
#include <nias_cpp/inner_products/function_based.h>
#include <nias_cpp/interfaces/inner_products.h>
#include <nias_cpp/interfaces/vectorarray.h>

namespace nias
{


template <floating_point_or_complex F>
class EuclideanInnerProduct : public InnerProductInterface<F>
{
   public:
    using InterfaceType = InnerProductInterface<F>;
    using typename InterfaceType::ScalarType;

    [[nodiscard]] std::vector<std::vector<F>> apply(
        const VectorArrayInterface<ScalarType>& left, const VectorArrayInterface<ScalarType>& right,
        const std::optional<Indices>& left_indices = std::nullopt,
        const std::optional<Indices>& right_indices = std::nullopt) const override
    {
        if (left_indices || right_indices)
        {
            return apply(left[left_indices], right[right_indices], std::nullopt, std::nullopt);
        }
        std::vector<std::vector<F>> ret(as_size_t(left.size()), std::vector<F>(as_size_t(right.size())));
        for (ssize_t i = 0; i < left.size(); ++i)
        {
            for (ssize_t j = 0; j < right.size(); ++j)
            {
                ret[as_size_t(i)][as_size_t(j)] = dot_product(left, right, {i}, {j})[0];
            }
        }
        return ret;
    }

    [[nodiscard]] std::vector<F> apply_pairwise(
        const VectorArrayInterface<ScalarType>& left, const VectorArrayInterface<ScalarType>& right,
        const std::optional<Indices>& left_indices = std::nullopt,
        const std::optional<Indices>& right_indices = std::nullopt) const override
    {
        return dot_product(left, right, left_indices, right_indices);
    }
};


}  // namespace nias

#endif  // NIAS_CPP_INNER_PRODUCTS_EUCLIDEAN_H
