#ifndef NIAS_CPP_INTERFACES_VECTORARRAY_H
#define NIAS_CPP_INTERFACES_VECTORARRAY_H

#include <cstddef>
#include <memory>
#include <vector>

#include <nias_cpp/concepts.h>

namespace nias
{


template <floating_point_or_complex F>
class VectorArrayInterface
{
    using ThisType = VectorArrayInterface;

   public:
    // return the number of vectors in the array
    virtual size_t size() const = 0;

    // return the dimension (length) of the vectors in the array
    virtual size_t dim() const = 0;

    virtual bool is_compatible_array(const ThisType& other) const
    {
        return dim() == other.dim();
    }

    // copy (a subset of) the VectorArray to a new VectorArray
    virtual std::shared_ptr<ThisType> copy(const std::vector<size_t>& indices = {}) const = 0;

    virtual void append(ThisType& other, bool remove_from_other = false,
                        const std::vector<size_t>& other_indices = {}) = 0;

    virtual void scal(const std::vector<F>& alpha, const std::vector<size_t>& indices = {}) = 0;

    virtual void scal(F alpha, const std::vector<size_t>& indices = {})
    {
        scal(std::vector<F> {alpha}, indices);
    }

    virtual void axpy(const std::vector<F>& alpha, const ThisType& x, const std::vector<size_t>& indices = {},
                      const std::vector<size_t>& x_indices = {}) = 0;

    virtual void axpy(F alpha, const ThisType& x, const std::vector<size_t>& indices = {},
                      const std::vector<size_t>& x_indices = {})
    {
        axpy(std::vector<F> {alpha}, x, indices, x_indices);
    }

    virtual F get(size_t i, size_t j) const = 0;

    virtual void delete_vectors(const std::vector<size_t>& indices) = 0;
    // virtual const std::vector<std::shared_ptr<VectorInterface<F>>>& vectors() const = 0;
};


}  // namespace nias

#endif  // NIAS_CPP_INTERFACES_VECTORARRAY_H
