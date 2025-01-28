#ifndef NIAS_CPP_INTERFACES_VECTORARRAY_H
#define NIAS_CPP_INTERFACES_VECTORARRAY_H

#include <format>
#include <iostream>
#include <memory>
#include <optional>
#include <vector>

#include <nias_cpp/concepts.h>
#include <nias_cpp/exceptions.h>
#include <nias_cpp/indices.h>
#include <nias_cpp/type_traits.h>
#include <sys/types.h>

namespace nias
{


// forward
template <floating_point_or_complex F>
class VectorArrayInterface;

template <floating_point_or_complex F>
class ConstVectorArrayView : public VectorArrayInterface<F>
{
    using ThisType = ConstVectorArrayView<F>;
    using InterfaceType = VectorArrayInterface<F>;

   public:
    // TODO: Add some refcounting in the interface to throw if the underlying object is deleted and there is still a view
    // Consider to disallow views of views to not complicate refcounting
    ConstVectorArrayView(const VectorArrayInterface<F>& vec_array, const std::optional<Indices>& indices)
        : vec_array_(vec_array)
        , indices_(indices)
    {
    }

    [[nodiscard]] ssize_t size() const override
    {
        return indices_ ? indices_->size(vec_array_.size()) : vec_array_.size();
    }

    [[nodiscard]] ssize_t dim() const override
    {
        return vec_array_.dim();
    }

    std::shared_ptr<InterfaceType> copy(const std::optional<Indices>& view_indices = {}) const
    {
        const auto indices = new_indices(view_indices);
        return vec_array_.copy(indices);
    }

    void append(InterfaceType& /*other*/, bool /*remove_from_other*/ = false,
                const std::optional<Indices>& /*other_indices*/ = {}) override
    {
        // TODO: Add custom Nias exception
        throw NotImplementedError("ConstVectorArrayView: append is not implemented (call copy() first).");
    }

    void scal(const std::vector<F>& /*alpha*/, const std::optional<Indices>& /*view_indices*/ = {}) override
    {
        throw NotImplementedError("ConstVectorArrayView: scal is not implemented, use VectorArrayView.");
    }

    void axpy(const std::vector<F>& /*alpha*/, const InterfaceType& /*x*/,
              const std::optional<Indices>& /*view_indices*/ = {},
              const std::optional<Indices>& /*x_indices*/ = {})
    {
        throw NotImplementedError("ConstVectorArrayView: axpy is not implemented, use VectorArrayView.");
    }

    F get(ssize_t i, ssize_t j) const override
    {
        return vec_array_.get(indices_ ? indices_->get(i, vec_array_.size()) : i, j);
    }

    virtual void set(ssize_t /*i*/, ssize_t /*j*/, F /*value*/) override
    {
        throw NotImplementedError(
            "ConstVectorArrayView: cannot modify vector array entries through a const view.");
    }

    virtual void delete_vectors(const std::optional<Indices>& /*indices*/)
    {
        throw NotImplementedError(
            "ConstVectorArrayView: cannot delete vectors from vector array through a const view.");
    }

   protected:
    /// \brief Calculates new indices in the original VectorArray from the view indices
    [[nodiscard]] std::optional<Indices> new_indices(const std::optional<Indices>& view_indices) const
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
        std::cerr << "old_indices_vec: ";
        for (auto i : old_indices_vec)
        {
            std::cerr << i << " ";
        }
        std::cerr << '\n';
        view_indices->for_each(
            [&new_indices_vec, &old_indices_vec](ssize_t i)
            {
                new_indices_vec.push_back(old_indices_vec[i]);
            },
            this->size());
        std::cerr << "new_indices_vec: ";
        for (auto i : new_indices_vec)
        {
            std::cerr << i << " ";
        }
        std::cerr << '\n';
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

    virtual void set(ssize_t i, ssize_t j, F value) override
    {
        vec_array_.set(this->indices_ ? this->indices_->get(i, vec_array_.size()) : i, j, value);
    }

   private:
    VectorArrayInterface<F>& vec_array_;
};

template <floating_point_or_complex F>
class VectorArrayInterface
{
    using ThisType = VectorArrayInterface;

   public:
    using ScalarType = F;

    // constructors and destructor
    VectorArrayInterface() = default;
    virtual ~VectorArrayInterface() = default;

    // copy and move constructor and assignment operators
    VectorArrayInterface(const ThisType&) = default;
    VectorArrayInterface(ThisType&&) = default;
    VectorArrayInterface& operator=(const ThisType&) = default;
    VectorArrayInterface& operator=(ThisType&&) = default;

    // return the number of vectors in the array
    [[nodiscard]] virtual ssize_t size() const = 0;

    // return the dimension (length) of the vectors in the array
    [[nodiscard]] virtual ssize_t dim() const = 0;

    // Hack to get the scalar type on the Python side by calling type(impl.scalar_zero())
    // TODO: Find a better way to do this
    [[nodiscard]] F scalar_zero() const
    {
        return F(0);
    }

    virtual bool is_compatible_array(const ThisType& other) const
    {
        return dim() == other.dim();
    }

    // copy (a subset of) the VectorArray to a new VectorArray
    // TODO: Should this be a unique ptr?
    // TODO: Return a view?
    virtual std::shared_ptr<ThisType> copy(const std::optional<Indices>& indices = {}) const = 0;

    virtual void append(ThisType& other, bool remove_from_other = false,
                        const std::optional<Indices>& other_indices = {}) = 0;

    virtual void scal(const std::vector<F>& alpha, const std::optional<Indices>& indices = {}) = 0;

    virtual void scal(F alpha, const std::optional<Indices>& indices = {})
    {
        scal(std::vector<F>{alpha}, indices);
    }

    virtual void axpy(const std::vector<F>& alpha, const ThisType& x,
                      const std::optional<Indices>& indices = {},
                      const std::optional<Indices>& x_indices = {}) = 0;

    virtual void axpy(F alpha, const ThisType& x, const std::optional<Indices>& indices = {},
                      const std::optional<Indices>& x_indices = {})
    {
        axpy(std::vector<F>{alpha}, x, indices, x_indices);
    }

    virtual F get(ssize_t i, ssize_t j) const = 0;

    virtual void set(ssize_t /*i*/, ssize_t /*j*/, F /*value*/)
    {
        throw NotImplementedError("VectorArrayInterface: set is not implemented.");
    }

    virtual void print() const
    {
        std::cerr << "VectorArray with " << size() << " vectors of dimension " << dim() << std::endl;
        for (ssize_t i = 0; i < size(); ++i)
        {
            for (ssize_t j = 0; j < dim(); ++j)
            {
                std::cerr << get(i, j) << " ";
            }
            std::cerr << '\n';
        }
    }

    virtual VectorArrayView<F> operator[](const Indices& indices)
    {
        return VectorArrayView<F>(*this, indices);
    }

    virtual ConstVectorArrayView<F> operator[](const Indices& indices) const
    {
        return ConstVectorArrayView<F>(*this, indices);
    }

    virtual void delete_vectors(const std::optional<Indices>& indices) = 0;

   protected:
    void check_first_index(ssize_t i) const
    {
        if (i < 0 || i >= size())
        {
            throw InvalidIndexError(
                std::format("ListVectorArray: index i={} out of range for {}x{} array", i, size(), dim()));
        }
    }

    void check_second_index(ssize_t j) const
    {
        if (j < 0 || j >= dim())
        {
            throw InvalidIndexError(
                std::format("ListVectorArray: index j={} out of range for {}x{} array", j, size(), dim()));
        }
    }

    void check_indices(ssize_t i, ssize_t j) const
    {
        check_first_index(i);
        check_second_index(j);
    }
};


}  // namespace nias

#endif  // NIAS_CPP_INTERFACES_VECTORARRAY_H
