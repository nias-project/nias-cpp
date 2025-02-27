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

#endif  // NIAS_CPP_INNER_PRODUCTS_EUCLIDEAN_H
