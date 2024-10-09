#ifndef NIAS_CPP_INTERFACES_VECTOR_H
#define NIAS_CPP_INTERFACES_VECTOR_H

#include <memory>

#include <nias_cpp/concepts.h>
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
    virtual const F& get(ssize_t i) const = 0;

    // return the dimension (length) of the vector
    [[nodiscard]] virtual ssize_t dim() const = 0;

    // copy the Vector to a new Vector
    virtual std::shared_ptr<VectorInterface> copy() const = 0;

    // dot product
    virtual F dot(const VectorInterface& x) const = 0;

    // scale with a scalar
    virtual void scal(F alpha) = 0;

    // axpy
    virtual void axpy(F alpha, const VectorInterface& x) = 0;
};


}  // namespace nias

#endif  // NIAS_CPP_INTERFACES_VECTOR_H
