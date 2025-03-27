#ifndef NIAS_CPP_INTERFACES_VECTORARRAY_H
#define NIAS_CPP_INTERFACES_VECTORARRAY_H

#include <format>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <nias_cpp/checked_integer_cast.h>
#include <nias_cpp/concepts.h>
#include <nias_cpp/exceptions.h>
#include <nias_cpp/indices.h>
#include <nias_cpp/interfaces/vector.h>
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

    [[nodiscard]] std::shared_ptr<InterfaceType> copy(
        const std::optional<Indices>& view_indices = std::nullopt) const override
    {
        const auto indices = new_indices(view_indices);
        return vec_array_.copy(indices);
    }

    void append(InterfaceType& /*other*/, bool /*remove_from_other*/ = false,
                const std::optional<Indices>& /*other_indices*/ = std::nullopt) override
    {
        // TODO: Add custom Nias exception
        throw NotImplementedError("ConstVectorArrayView: append is not implemented (call copy() first).");
    }

    void scal(const std::vector<F>& /*alpha*/,
              const std::optional<Indices>& /*view_indices*/ = std::nullopt) override
    {
        throw NotImplementedError("ConstVectorArrayView: scal is not implemented, use VectorArrayView.");
    }

    void axpy(const std::vector<F>& /*alpha*/, const InterfaceType& /*x*/,
              const std::optional<Indices>& /*view_indices*/ = std::nullopt,
              const std::optional<Indices>& /*x_indices*/ = std::nullopt) override
    {
        throw NotImplementedError("ConstVectorArrayView: axpy is not implemented, use VectorArrayView.");
    }

    [[nodiscard]] const VectorInterface<F>& vector(ssize_t i) const override
    {
        return vec_array_.vector(indices_ ? indices_->get(i, vec_array_.size()) : i);
    }

    [[nodiscard]] VectorInterface<F>& vector(ssize_t /*i*/) override
    {
        throw NotImplementedError("ConstVectorArrayView: no mutable access to vectors, use VectorArrayView.");
    }

    [[nodiscard]] F get(ssize_t i, ssize_t j) const override
    {
        return vec_array_.get(indices_ ? indices_->get(i, vec_array_.size()) : i, j);
    }

    void set(ssize_t /*i*/, ssize_t /*j*/, F /*value*/) override
    {
        throw NotImplementedError(
            "ConstVectorArrayView: cannot modify vector array entries through a const view.");
    }

    void delete_vectors(const std::optional<Indices>& /*indices*/) override
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
        new_indices_vec.reserve(as_size_t(view_indices->size(this->size())));
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
                new_indices_vec.push_back(old_indices_vec[as_size_t(i)]);
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

    void scal(const std::vector<F>& alpha, const std::optional<Indices>& view_indices = std::nullopt) override
    {
        const auto indices = this->new_indices(view_indices);
        vec_array_.scal(alpha, indices);
    }

    void axpy(const std::vector<F>& alpha, const InterfaceType& x,
              const std::optional<Indices>& view_indices = std::nullopt,
              const std::optional<Indices>& x_indices = std::nullopt) override
    {
        const auto indices = this->new_indices(view_indices);
        vec_array_.axpy(alpha, x, indices, x_indices);
    }

    void set(ssize_t i, ssize_t j, F value) override
    {
        vec_array_.set(this->indices_ ? this->indices_->get(i, vec_array_.size()) : i, j, value);
    }

    [[nodiscard]] VectorInterface<F>& vector(ssize_t i) override
    {
        return vec_array_.vector(this->indices_ ? this->indices_->get(i, vec_array_.size()) : i);
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

    /// \brief return the number of vectors in the array
    [[nodiscard]] virtual ssize_t size() const = 0;

    /// \brief return the dimension (length) of the vectors in the array
    [[nodiscard]] virtual ssize_t dim() const = 0;

    // Hack to get the scalar type on the Python side by calling type(impl.scalar_zero())
    // TODO: Find a better way to do this
    [[nodiscard]] F scalar_zero() const
    {
        return F(0);
    }

    /**
     * \brief Checks that other is compatible with this VectorArray
     * \todo What does that mean exactly? Currently only checks dimensions.
     */
    [[nodiscard]] virtual bool is_compatible_array(const ThisType& other) const
    {
        return dim() == other.dim();
    }

    /**
     * \brief Copies (a subset of) the VectorArray to a new VectorArray
     *
     * Copies the vectors corresponding to \c indices to a new VectorArray (in the order given by <tt>indices</tt>).
     * If no indices are given, all vectors are copied.
     *
     * \param indices: Indices of the vectors to copy. If std::nullopt (the default), all vectors are copied.
     * \note Indices do not have to be unique. If indices are repeated, the returned VectorArray
     * will contain multiple copies of the corresponding vector.
     * \todo: Should this be a unique ptr?
     * \todo: Return a view?
     */
    [[nodiscard]] virtual std::shared_ptr<ThisType> copy(
        const std::optional<Indices>& indices = std::nullopt) const = 0;

    /**
     * \brief Appends (a subset of) another VectorArray to this VectorArray
     *
     * Appends the vectors of \c other corresponding to \c other_indices to this VectorArray (in the order
     * given by \c other_indices). If no indices are given, all vectors are appended.
     *
     * \param other: The VectorArray to append.
     * \param remove_from_other: If true, the vectors are removed from \c other after appending.
     * \param other_indices: Indices of the vectors to append. If std::nullopt (the default), all vectors are appended.
     * \note Indices in \c other_indices do not have to be unique. If indices are repeated, the corresponding vector
     * is simply appended multiple times.
     */
    virtual void append(ThisType& other, bool remove_from_other = false,
                        const std::optional<Indices>& other_indices = std::nullopt) = 0;

    /**
     * \brief Scales (a subset of) the vectors by entries of alpha
     *
     * If no indices are given, alpha has to have the same size as the VectorArray, and each vector is scaled by the
     * respective entry of alpha. If indices are given, there must be as many indices as there are entries in alpha.
     *
     * \param alpha: The scaling factors.
     * \param indices: Indices of the vectors to scale. If std::nullopt (the default), all vectors are scaled.
     */
    virtual void scal(const std::vector<F>& alpha, const std::optional<Indices>& indices = std::nullopt)
    {
        if (!indices)
        {
            check(alpha.size() == 1 || std::ssize(alpha) == this->size(),
                  "alpha must have size 1 or the same size as the array.");
            for (ssize_t i = 0; i < size(); ++i)
            {
                const auto alpha_index = as_size_t(alpha.size() == 1 ? 0 : i);
                for (ssize_t j = 0; j < dim(); ++j)
                {
                    this->set(i, j, this->get(i, j) * alpha[alpha_index]);
                }
            }
        }
        else
        {
            indices->check_valid(this->size());
            check(alpha.size() == 1 || std::ssize(alpha) == indices->size(this->size()),
                  "alpha must have size 1 or the same size as indices");
            size_t alpha_index = 0;
            indices->for_each(
                [this, &alpha, &alpha_index](ssize_t i)
                {
                    for (ssize_t j = 0; j < dim(); ++j)
                    {
                        this->set(i, j, this->get(i, j) * alpha[alpha_index]);
                    }
                    if (alpha.size() > 1)
                    {
                        ++alpha_index;
                    }
                },
                this->size());
        }
    }

    /**
     * \brief Scales (a subset of) the vectors by a scalar alpha
     *
     * Behaves like the vector-valued version of \c scal called with an alpha vector of size 1
     */
    virtual void scal(F alpha, const std::optional<Indices>& indices = std::nullopt)
    {
        scal(std::vector<F>{alpha}, indices);
    }

    /**
     * \brief axpy operation on (a subset of) the VectorArray
     *
     * Computes <tt>y = y + alpha * x</tt>, where y is this VectorArray. If \c indices are given,
     * the operation is performed on the respective vectors only. If \c x_indices are given, only
     * the respective vectors of \c x are used.
     * Both \c alpha and \c x can either have size 1 or the same size as the VectorArray (after
     * applying both \c indices and <tt>x_indices</tt>). In the former case, the value of
     * \c alpha or \c x is used for all vectors.
     *
     * \param alpha: The scaling factors.
     * \param x: The VectorArray to add.
     * \param indices: Indices for this VectorArray. If std::nullopt (the default), all vectors are used.
     * \param x_indices: Indices for x. If std::nullopt (the default), all vectors from x are used.
     */
    virtual void axpy(const std::vector<F>& alpha, const ThisType& x,
                      const std::optional<Indices>& indices = std::nullopt,
                      const std::optional<Indices>& x_indices = std::nullopt)
    {
        check(this->is_compatible_array(x), "incompatible dimensions.");
        if (indices)
        {
            indices->check_valid(size());
        }
        if (x_indices)
        {
            x_indices->check_valid(x.size());
        }
        const auto this_size = indices ? indices->size(size()) : size();
        const auto x_size = x_indices ? x_indices->size(x.size()) : x.size();
        check(x_size == this_size || x_size == 1, "x must have length 1 or the same length as this");
        check(std::ssize(alpha) == this_size || alpha.size() == 1,
              "alpha must be scalar or have the same length as this");
        for (ssize_t i = 0; i < this_size; ++i)
        {
            const auto this_index = indices ? indices->get(i, size()) : i;
            // x and alpha can either have the same length as this or length 1
            ssize_t x_index = x_size == 1 ? 0 : i;
            x_index = x_indices ? x_indices->get(x_index, x.size()) : x_index;
            const auto alpha_index = as_size_t(alpha.size() == 1 ? 0 : i);
            for (ssize_t j = 0; j < dim(); ++j)
            {
                this->set(this_index, j, this->get(this_index, j) + (alpha[alpha_index] * x.get(x_index, j)));
            }
        }
    }

    /**
     * \brief axpy operation with fixed alpha on (a subset of) the VectorArray
     *
     * Behaves like the vector-valued version of \c axpy called with an alpha vector of size 1
     */
    virtual void axpy(F alpha, const ThisType& x, const std::optional<Indices>& indices = std::nullopt,
                      const std::optional<Indices>& x_indices = std::nullopt)
    {
        axpy(std::vector<F>{alpha}, x, indices, x_indices);
    }

    /**
     * \brief Returns a const reference to the i-th vector
     *
     * \note Not all vector array implementations give access to the underlying vectors,
     * e.g., the NumpyVectorArray does not. In this case, the method will throw a NotImplementedError.
     */
    [[nodiscard]] virtual const VectorInterface<F>& vector(ssize_t /*i*/) const
    {
        throw nias::NotImplementedError("No access to underlying vectors.");
    }

    /**
     * \brief Returns a mutable reference to the i-th vector
     *
     * \note Not all vector array implementations give access to the underlying vectors,
     * e.g., the NumpyVectorArray does not. In this case, the method will throw a NotImplementedError.
     */
    [[nodiscard]] virtual VectorInterface<F>& vector(ssize_t /*i*/)
    {
        throw nias::NotImplementedError("No access to underlying vectors.");
    }

    /**
     * \brief Returns a const reference to the vector at index \c i cast to the specified type \c VectorType.
     *
     * \tparam VectorType The type to which the vector should be cast. Must derive from VectorInterface<F>.
     * \param i The index of the vector to retrieve.
     * \throws InvalidArgumentError if the vector cannot be cast to \c VectorType.
     * \throws NotImplementedError if the vector array implementation does not provide random vector access
     * \sa vector(ssize_t)
     */
    template <class VectorType>
        requires std::derived_from<VectorType, VectorInterface<F>>
    [[nodiscard]] const VectorType& vector_as(ssize_t i) const
    {
        try
        {
            return dynamic_cast<const VectorType&>(vector(i));
        }
        catch (std::bad_cast&)
        {
            throw InvalidArgumentError("vector at index " + std::to_string(i) + " is not of type " +
                                       typeid(VectorType).name());
        }
    }

    /**
     * \brief Returns a mutable reference to the vector at index \c i cast to the specified type \c VectorType.
     * \sa vector_as(ssize_t) const
     */
    template <class VectorType>
        requires std::derived_from<VectorType, VectorInterface<F>>
    [[nodiscard]] VectorType& vector_as(ssize_t i)
    {
        try
        {
            return dynamic_cast<VectorType&>(vector(i));
        }
        catch (std::bad_cast&)
        {
            throw InvalidArgumentError("vector at index " + std::to_string(i) + " is not of type " +
                                       typeid(VectorType).name());
        }
    }

    /**
     * \brief Returns the j-th entry of the i-th vector
     */
    [[nodiscard]] virtual F get(ssize_t i, ssize_t j) const = 0;

    /**
     * \brief Sets the j-th entry of the i-th vector to \c value
     */
    virtual void set(ssize_t i, ssize_t j, F value) = 0;

    virtual void print() const
    {
        std::cerr << *this;
    }

    friend std::ostream& operator<<(std::ostream& os, const ThisType& vec_array)
    {
        os << "VectorArray with " << vec_array.size() << " vectors of dimension " << vec_array.dim()
           << std::endl;
        for (ssize_t i = 0; i < vec_array.size(); ++i)
        {
            for (ssize_t j = 0; j < vec_array.dim(); ++j)
            {
                os << vec_array.get(i, j) << " ";
            }
            os << '\n';
        }
        return os;
    }

    /**
     * \brief Returns a view on the vectors corresponding to the given indices
     */
    virtual VectorArrayView<F> operator[](const std::optional<Indices>& indices)
    {
        return VectorArrayView<F>(*this, indices);
    }

    /**
     * \brief Returns a const view on the vectors corresponding to the given indices
     */
    virtual ConstVectorArrayView<F> operator[](const std::optional<Indices>& indices) const
    {
        return ConstVectorArrayView<F>(*this, indices);
    }

    /**
     * \brief Deletes the vectors corresponding to the given indices
     */
    virtual void delete_vectors(const std::optional<Indices>& indices) = 0;

   protected:
    /**
     * \brief Checks that the first index i is in the range [0, size())
     */
    void check_first_index(ssize_t i) const
    {
        if (i < 0 || i >= size())
        {
            throw InvalidIndexError(
                std::format("ListVectorArray: index i={} out of range for {}x{} array", i, size(), dim()));
        }
    }

    /**
     * \brief Checks that the second index j is in the range [0, dim())
     */
    void check_second_index(ssize_t j) const
    {
        if (j < 0 || j >= dim())
        {
            throw InvalidIndexError(
                std::format("ListVectorArray: index j={} out of range for {}x{} array", j, size(), dim()));
        }
    }

    /**
     * \brief Checks that both indices are in range
     */
    void check_indices(ssize_t i, ssize_t j) const
    {
        check_first_index(i);
        check_second_index(j);
    }

    void check(const bool condition, const std::string& message) const
    {
        if (!condition)
        {
            throw InvalidArgumentError(message);
        }
    }
};

template <floating_point_or_complex F>
std::ostream& operator<<(std::ostream& os, const VectorArrayInterface<F>& vec_array)
{
    os << "VectorArray with " << vec_array.size() << " vectors of dimension " << vec_array.dim() << std::endl;
    for (ssize_t i = 0; i < vec_array.size(); ++i)
    {
        for (ssize_t j = 0; j < vec_array.dim(); ++j)
        {
            os << vec_array.get(i, j) << " ";
        }
        os << '\n';
    }
    return os;
}


}  // namespace nias

#endif  // NIAS_CPP_INTERFACES_VECTORARRAY_H
