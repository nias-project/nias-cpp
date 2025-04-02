#ifndef NIAS_CPP_INTERFACES_VECTOR_H
#define NIAS_CPP_INTERFACES_VECTOR_H

#include <format>
#include <memory>
#include <ostream>

#include <nias_cpp/concepts.h>
#include <nias_cpp/exceptions.h>
#include <nias_cpp/type_traits.h>
#include <nias_cpp/vector/traits.h>

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
    [[nodiscard]] virtual const F& operator[](ssize_t i) const = 0;
    [[nodiscard]] virtual F& operator[](ssize_t i) = 0;

    // return the dimension (length) of the vector
    [[nodiscard]] virtual ssize_t dim() const = 0;

    // copy the Vector to a new Vector
    [[nodiscard]] virtual std::shared_ptr<VectorInterface> copy() const = 0;

    // scale with a scalar
    virtual void scal(F alpha)
    {
        for (ssize_t i = 0; i < this->dim(); ++i)
        {
            (*this)[i] *= alpha;
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
            (*this)[i] += alpha * x[i];
        }
    }
};

template <floating_point_or_complex F>
std::ostream& operator<<(std::ostream& os, const VectorInterface<F>& vec)
{
    os << "[";
    for (ssize_t i = 0; i < vec.dim(); ++i)
    {
        os << vec[i];
        if (i < vec.dim() - 1)
        {
            os << ", ";
        }
    }
    os << "]";
    return os;
}

namespace internal
{
template <floating_point_or_complex F>
ssize_t derived_from_vector_interface_helper_function(const VectorInterface<F>& vec)
{
    return vec.dim();
}
}  // namespace internal

template <class V>
concept derived_from_vector_interface = requires(V vec) {
    { internal::derived_from_vector_interface_helper_function(vec) } -> std::same_as<ssize_t>;
};

template <class V>
    requires derived_from_vector_interface<V>
struct VectorTraits<V>
{
    using VectorType = V;
    using ScalarType = std::decay_t<decltype(std::declval<VectorType>()[0])>;
    static constexpr auto dim_ = [](const VectorType& vec) -> ssize_t
    {
        return vec.dim();
    };
    static constexpr auto copy_ = [](const VectorType& vec) -> VectorType
    {
        return *std::dynamic_pointer_cast<VectorType>(vec.copy());
    };
    static constexpr auto get_ = [](VectorType& vec, ssize_t i) -> ScalarType&
    {
        return vec[i];
    };
    static constexpr auto const_get_ = [](const VectorType& vec, ssize_t i) -> const ScalarType&
    {
        return vec[i];
    };
};


}  // namespace nias

#endif  // NIAS_CPP_INTERFACES_VECTOR_H
