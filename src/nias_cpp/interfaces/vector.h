#ifndef NIAS_CPP_INTERFACES_VECTOR_H
#define NIAS_CPP_INTERFACES_VECTOR_H

#include <cstddef>
#include <memory>

#include <nias_cpp/concepts.h>
#include <nias_cpp/interfaces/vector.h>

namespace nias
{


template <floating_point_or_complex F>
class VectorInterface
{
   public:
    virtual ~VectorInterface() {
        // std::cout << "VectorInterface destructor for " << this << std::endl;
    };

    // accessors
    virtual F& get(size_t i) = 0;
    virtual const F& get(size_t i) const = 0;

    // return the dimension (length) of the vector
    virtual size_t dim() const = 0;

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
