#ifndef NIAS_CPP_OPERATORS_INNER_PRODUCTS_H
#define NIAS_CPP_OPERATORS_INNER_PRODUCTS_H

#include <optional>

#include <nias_cpp/algorithms/dot_product.h>
#include <nias_cpp/interfaces/inner_products.h>
#include <nias_cpp/interfaces/vector.h>
#include <nias_cpp/interfaces/vectorarray.h>
#include <nias_cpp/vectorarray/list.h>

namespace nias
{


template <floating_point_or_complex F>
class FunctionBasedInnerProduct : public InnerProductInterface<F>
{
   public:
    using InterfaceType = InnerProductInterface<F>;
    using ThisType = FunctionBasedInnerProduct<F>;
    using typename InterfaceType::ScalarType;

    FunctionBasedInnerProduct(
        std::function<ScalarType(const VectorArrayInterface<ScalarType>&,
                                 const VectorArrayInterface<ScalarType>&, ssize_t i, ssize_t j)>
            inner_product_function)
        : inner_product_function_(inner_product_function)
    {
    }

    pybind11::array_t<ScalarType> apply(
        const VectorArrayInterface<ScalarType>& left, const VectorArrayInterface<ScalarType>& right,
        bool pairwise = false, const std::optional<Indices>& left_indices = std::nullopt,
        const std::optional<Indices>& right_indices = std::nullopt) const override
    {
        using py_array = pybind11::array_t<ScalarType>;
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
            py_array ret({left.size()});
            for (ssize_t i = 0; i < left.size(); ++i)
            {
                ret.mutable_at(i) = inner_product_function_(left, right, i, i);
            }
            return ret;
        }
        else
        {
            py_array ret({left.size(), right.size()});
            for (ssize_t i = 0; i < left.size(); ++i)
            {
                for (ssize_t j = 0; j < right.size(); ++j)
                {
                    ret.mutable_at(i, j) = inner_product_function_(left, right, i, j);
                }
            }
            return ret;
        }
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
