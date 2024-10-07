#ifndef NIAS_CPP_INTERFACES_VECTORARRAY_H
#define NIAS_CPP_INTERFACES_VECTORARRAY_H

#include <iostream>
#include <memory>
#include <stdexcept>
#include <vector>

#include <nias_cpp/concepts.h>
#include <nias_cpp/indices.h>
#include <sys/types.h>

namespace nias
{


template <floating_point_or_complex F>
class VectorArrayInterface
{
    using ThisType = VectorArrayInterface;

   public:
    // return the number of vectors in the array
    virtual ssize_t size() const = 0;

    // return the dimension (length) of the vectors in the array
    virtual ssize_t dim() const = 0;

    virtual bool is_compatible_array(const ThisType& other) const
    {
        return dim() == other.dim();
    }

    // copy (a subset of) the VectorArray to a new VectorArray
    virtual std::shared_ptr<ThisType> copy(const std::optional<Indices>& indices = {}) const = 0;

    virtual void append(ThisType& other, bool remove_from_other = false,
                        const std::optional<Indices>& other_indices = {}) = 0;

    virtual void scal(const std::vector<F>& alpha, const std::optional<Indices>& indices = {}) = 0;

    virtual void scal(F alpha, const std::optional<Indices>& indices = {})
    {
        scal(std::vector<F> {alpha}, indices);
    }

    virtual void axpy(const std::vector<F>& alpha, const ThisType& x,
                      const std::optional<Indices>& indices = {},
                      const std::optional<Indices>& x_indices = {}) = 0;

    virtual void axpy(F alpha, const ThisType& x, const std::optional<Indices>& indices = {},
                      const std::optional<Indices>& x_indices = {})
    {
        axpy(std::vector<F> {alpha}, x, indices, x_indices);
    }

    virtual F get(ssize_t i, ssize_t j) const = 0;

    virtual void print()
    {
        std::cout << "VectorArray with " << size() << " vectors of dimension " << dim() << std::endl;
        for (size_t i = 0; i < size(); ++i)
        {
            for (size_t j = 0; j < dim(); ++j)
            {
                std::cout << get(i, j) << " ";
            }
            std::cout << std::endl;
        }
    }

    virtual void delete_vectors(const std::optional<Indices>& indices) = 0;
    // virtual const std::vector<std::shared_ptr<VectorInterface<F>>>& vectors() const = 0;
};

template <floating_point_or_complex F>
class ConstVectorArrayView : public VectorArrayInterface<F>
{
    using ThisType = ConstVectorArrayView<F>;
    using InterfaceType = VectorArrayInterface<F>;

   public:
    ConstVectorArrayView(const VectorArrayInterface<F>& vec_array, const std::optional<Indices>& indices)
        : vec_array_(vec_array)
        , indices_(indices)
    {
    }

    ssize_t size() const override
    {
        return indices_->size(vec_array_.size());
    }

    ssize_t dim() const override
    {
        return vec_array_.dim();
    }

    std::shared_ptr<InterfaceType> copy(const std::optional<Indices>& view_indices = {}) const
    {
        const auto indices = new_indices(view_indices);
        return vec_array_.copy(indices);
    }

    void append(ThisType& /*other*/, bool /*remove_from_other*/ = false,
                const std::optional<Indices>& /*other_indices*/ = {}) override
    {
        throw std::runtime_error("ConstVectorArrayView: append is not implemented (call copy() first).");
    }

    void scal(const std::vector<F>& /*alpha*/, const std::optional<Indices>& /*view_indices*/ = {}) override
    {
        throw std::runtime_error("ConstVectorArrayView: scal is not implemented, use VectorArrayView.");
    }

    void axpy(const std::vector<F>& /*alpha*/, const ThisType& /*x*/,
              const std::optional<Indices>& /*view_indices*/ = {},
              const std::optional<Indices>& /*x_indices*/ = {})
    {
        throw std::runtime_error("ConstVectorArrayView: axpy is not implemented, use VectorArrayView.");
    }

    F get(ssize_t i, ssize_t j) const override
    {
        return vec_array_.get(indices_->get(i, this->size()), j);
    }

    virtual void delete_vectors(const std::optional<Indices>& /*indices*/)
    {
        throw std::runtime_error("ConstVectorArrayView: delete_vectors is not implemented.");
    }

   protected:
    /// \brief Calculates new indices in the original VectorArray from the view indices
    std::optional<Indices> new_indices(const std::optional<Indices>& view_indices) const
    {
        if (!view_indices)
        {
            return indices_;
        }
        if (!indices_)
        {
            return view_indices;
        }
        // TODO: can we do this more efficiently?
        std::vector<ssize_t> new_indices_vec;
        new_indices_vec.reserve(view_indices->size(this->size()));
        const auto old_indices_vec = indices_->as_vec(vec_array_.size());
        view_indices->for_each(
            [&new_indices_vec, &old_indices_vec](ssize_t i)
            {
                new_indices_vec.push_back(old_indices_vec[i]);
            },
            this->size());
        return Indices(new_indices_vec);
    }

    const VectorArrayInterface<F>& vec_array_;
    std::optional<Indices> indices_;
};

template <floating_point_or_complex F>
class VectorArrayView : public ConstVectorArrayView<F>
{
    using ThisType = VectorArrayView<F>;
    using InterfaceType = VectorArrayInterface<F>;

   public:
    VectorArrayView(VectorArrayInterface<F>& vec_array, const std::optional<Indices>& indices)
        : ConstVectorArrayView<F>(vec_array, indices)
        , vec_array_(vec_array)
    {
    }

    void scal(const std::vector<F>& alpha, const std::optional<Indices>& view_indices = {}) override
    {
        const auto indices = this->new_indices(view_indices);
        vec_array_.scal(alpha, indices);
    }

    void axpy(const std::vector<F>& alpha, const InterfaceType& x,
              const std::optional<Indices>& view_indices = {},
              const std::optional<Indices>& x_indices = {}) override
    {
        const auto indices = this->new_indices(view_indices);
        vec_array_.axpy(alpha, x, indices, x_indices);
    }

   private:
    VectorArrayInterface<F>& vec_array_;
};


}  // namespace nias

#endif  // NIAS_CPP_INTERFACES_VECTORARRAY_H
