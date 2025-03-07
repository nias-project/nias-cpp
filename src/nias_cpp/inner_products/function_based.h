#ifndef NIAS_CPP_INNER_PRODUCTS_FUNCTION_BASED_H
#define NIAS_CPP_INNER_PRODUCTS_FUNCTION_BASED_H

#include <functional>
#include <optional>
#include <utility>

#include <nias_cpp/algorithms/dot_product.h>
#include <nias_cpp/checked_integer_cast.h>
#include <nias_cpp/concepts.h>
#include <nias_cpp/exceptions.h>
#include <nias_cpp/indices.h>
#include <nias_cpp/interfaces/inner_products.h>
#include <nias_cpp/interfaces/vectorarray.h>
#include <nias_cpp/type_traits.h>

namespace nias
{


template <floating_point_or_complex F>
class FunctionBasedInnerProduct : public InnerProductInterface<F>
{
   public:
    using InterfaceType = InnerProductInterface<F>;
    using ThisType = FunctionBasedInnerProduct<F>;
    using typename InterfaceType::ScalarType;

    explicit FunctionBasedInnerProduct(
        std::function<ScalarType(const VectorArrayInterface<ScalarType>&,
                                 const VectorArrayInterface<ScalarType>&, ssize_t i, ssize_t j)>
            inner_product_function)
        : inner_product_function_(std::move(inner_product_function))
    {
    }

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
                ret[as_size_t(i)][as_size_t(j)] = inner_product_function_(left, right, i, j);
            }
        }
        return ret;
    }

    [[nodiscard]] std::vector<F> apply_pairwise(
        const VectorArrayInterface<ScalarType>& left, const VectorArrayInterface<ScalarType>& right,
        const std::optional<Indices>& left_indices = std::nullopt,
        const std::optional<Indices>& right_indices = std::nullopt) const override
    {
        if (left_indices || right_indices)
        {
            return apply_pairwise(left[left_indices], right[right_indices], std::nullopt, std::nullopt);
        }
        if (left.size() != right.size())
        {
            throw InvalidArgumentError("Vector arrays must have the same size for pairwise application.");
        }
        std::vector<F> ret(as_size_t(left.size()));
        for (ssize_t i = 0; i < left.size(); ++i)
        {
            ret[as_size_t(i)] = inner_product_function_(left, right, i, i);
        }
        return ret;
    }

   private:
    std::function<ScalarType(const VectorArrayInterface<ScalarType>&, const VectorArrayInterface<ScalarType>&,
                             ssize_t, ssize_t)>
        inner_product_function_;
};


}  // namespace nias

#endif  // NIAS_CPP_INNER_PRODUCTS_FUNCTION_BASED_H
