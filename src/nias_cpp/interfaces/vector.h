#ifndef NIAS_CPP_INTERFACES_VECTOR_H
#define NIAS_CPP_INTERFACES_VECTOR_H

#include <format>
#include <memory>
#include <ostream>

#include <nias_cpp/concepts.h>
#include <nias_cpp/exceptions.h>
#include <nias_cpp/type_traits.h>

namespace nias
{


template <floating_point_or_complex F>
class VectorInterface
{
   public:
    // constructors and destructor
    VectorInterface() = default;
    virtual ~VectorInterface() = default;

    // copy and move constructor and assignment operators
    VectorInterface(const VectorInterface&) = default;
    VectorInterface(VectorInterface&&) = default;
    VectorInterface& operator=(const VectorInterface&) = default;
    VectorInterface& operator=(VectorInterface&&) = default;

    // accessors
    virtual F& get(ssize_t i) = 0;
    [[nodiscard]] virtual const F& get(ssize_t i) const = 0;

    // return the dimension (length) of the vector
    [[nodiscard]] virtual ssize_t dim() const = 0;

    // copy the Vector to a new Vector
    [[nodiscard]] virtual std::shared_ptr<VectorInterface> copy() const = 0;

    // scale with a scalar
    virtual void scal(F alpha)
    {
        for (ssize_t i = 0; i < this->dim(); ++i)
        {
            this->get(i) *= alpha;
        }
    }

    // axpy
    virtual void axpy(F alpha, const VectorInterface& x)
    {
        if (this->dim() != x.dim())
        {
            throw InvalidArgumentError(std::format(
                "Cannot compute axpy of vectors of different sizes: {} and {}", this->dim(), x.dim()));
        }
        for (ssize_t i = 0; i < this->dim(); ++i)
        {
            this->get(i) += alpha * x.get(i);
        }
    }

    /**
     * \brief Returns a const reference to this cast to the specified type \c VectorType.
     *
     * \tparam VectorType The type to which the vector should be cast. Must derive from VectorInterface<F>.
     * \throws InvalidArgumentError if the vector cannot be cast to \c VectorType.
     */
    template <class DerivedType>
        requires std::derived_from<DerivedType, VectorInterface>
    [[nodiscard]] const DerivedType& _as() const
    {
        try
        {
            return dynamic_cast<const DerivedType&>(*this);
        }
        catch (std::bad_cast&)
        {
            throw InvalidArgumentError(std::format("vector is not of type {}", typeid(DerivedType).name()));
        }
    }

    /**
     * \brief Returns a mutable reference to this cast to the specified type \c VectorType.
     * \sa _as() const
     */
    template <class DerivedType>
        requires std::derived_from<DerivedType, VectorInterface>
    [[nodiscard]] DerivedType& _as()
    {
        try
        {
            return dynamic_cast<DerivedType&>(*this);
        }
        catch (std::bad_cast&)
        {
            throw InvalidArgumentError(std::format("vector is not of type {}", typeid(DerivedType).name()));
        }
    }
};

template <floating_point_or_complex F>
std::ostream& operator<<(std::ostream& os, const VectorInterface<F>& vec)
{
    os << "[";
    for (ssize_t i = 0; i < vec.dim(); ++i)
    {
        os << vec.get(i);
        if (i < vec.dim() - 1)
        {
            os << ", ";
        }
    }
    os << "]";
    return os;
}


}  // namespace nias

#endif  // NIAS_CPP_INTERFACES_VECTOR_H
