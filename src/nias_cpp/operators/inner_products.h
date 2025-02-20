#ifndef NIAS_CPP_OPERATORS_INNER_PRODUCTS_H
#define NIAS_CPP_OPERATORS_INNER_PRODUCTS_H

#include <functional>
#include <optional>

#include <nias_cpp/algorithms/dot_product.h>
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
        : inner_product_function_(inner_product_function)
    {
    }

    std::vector<F> apply(const VectorArrayInterface<ScalarType>& left,
                         const VectorArrayInterface<ScalarType>& right, bool pairwise = false,
                         const std::optional<Indices>& left_indices = std::nullopt,
                         const std::optional<Indices>& right_indices = std::nullopt) const override
    {
        if (left_indices || right_indices)
        {
            return apply(left[left_indices], right[right_indices], pairwise, std::nullopt, std::nullopt);
        }
        if (pairwise)
        {
            if (left.size() != right.size())
            {
                throw InvalidArgumentError("Vector arrays must have the same size for pairwise application.");
            }
            std::vector<F> ret(left.size());
            for (ssize_t i = 0; i < left.size(); ++i)
            {
                ret[i] = inner_product_function_(left, right, i, i);
            }
            return ret;
        }
        std::vector<F> ret(left.size() * right.size());
        for (ssize_t i = 0; i < left.size(); ++i)
        {
            for (ssize_t j = 0; j < right.size(); ++j)
            {
                ret[i * right.size() + j] = inner_product_function_(left, right, i, j);
            }
        }
        return ret;
    }

   private:
    std::function<ScalarType(const VectorArrayInterface<ScalarType>&, const VectorArrayInterface<ScalarType>&,
                             ssize_t, ssize_t)>
        inner_product_function_;
};

template <floating_point_or_complex F>
class EuclideanInnerProduct : public FunctionBasedInnerProduct<F>
{
   public:
    EuclideanInnerProduct()
        : FunctionBasedInnerProduct<F>(
              [](const VectorArrayInterface<F>& left, const VectorArrayInterface<F>& right, const ssize_t i,
                 const ssize_t j)
              {
                  return dot_product(left, right, {i}, {j}).at(0);
              })
    {
    }
};


}  // namespace nias

#endif  // NIAS_CPP_OPERATORS_INNER_PRODUCTS_H
