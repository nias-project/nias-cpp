#ifndef NIAS_CPP_VECTOR_WRAPPER_H
#define NIAS_CPP_VECTOR_WRAPPER_H

#include <memory>

#include <nias_cpp/checked_integer_cast.h>
#include <nias_cpp/concepts.h>
#include <nias_cpp/exceptions.h>
#include <nias_cpp/indices.h>
#include <nias_cpp/interfaces/vector.h>
#include <nias_cpp/interfaces/vectorarray.h>
#include <nias_cpp/type_traits.h>
#include <nias_cpp/vector/traits.h>

namespace nias
{

template <class VectorType>
    requires has_vector_traits<VectorType>
class VectorWrapper : public VectorInterface<typename VectorTraits<VectorType>::ScalarType>
{
   public:
    using F = typename VectorTraits<VectorType>::ScalarType;

    explicit VectorWrapper(const VectorType& vector)
        : vector_(VectorTraits<VectorType>::copy_(vector))
    {
    }

    [[nodiscard]] ssize_t dim() const override
    {
        return VectorTraits<VectorType>::dim_(vector_);
    }

    [[nodiscard]] F& operator[](ssize_t i) override
    {
        return VectorTraits<VectorType>::get_(vector_, i);
    }

    [[nodiscard]] const F& operator[](ssize_t i) const override
    {
        return VectorTraits<VectorType>::const_get_(vector_, i);
    }

    [[nodiscard]] VectorType& backend()
    {
        return vector_;
    }

    [[nodiscard]] const VectorType& backend() const
    {
        return vector_;
    }

    [[nodiscard]] std::shared_ptr<VectorInterface<F>> copy() const override
    {
        return std::make_shared<VectorWrapper>(VectorTraits<VectorType>::copy_(vector_));
    }

   private:
    VectorType vector_;
};


}  // namespace nias

#endif  // NIAS_CPP_VECTOR_WRAPPER_H
