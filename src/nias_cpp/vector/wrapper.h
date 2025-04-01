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

namespace nias
{


template <class V>
struct VectorTraits
{
    using VectorType = V;
    static constexpr auto dim_ = [](const VectorType& vec)
    {
        return vec.dim();
    }();
    static constexpr auto copy_ = [](const VectorType& vec) -> VectorType
    {
        return vec;
    }();
};

template <class VectorType, floating_point_or_complex F>
class VectorWrapper : public VectorInterface<F>
{
   public:
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
        return vector_[i];
    }

    [[nodiscard]] const F& operator[](ssize_t i) const override
    {
        return vector_[i];
    }

    VectorType& backend()
    {
        return vector_;
    }

    const VectorType& backend() const
    {
        return vector_;
    }

    [[nodiscard]] std::shared_ptr<VectorInterface<F>> copy() const override
    {
        return std::make_shared<VectorType>(VectorTraits<VectorType>::copy_(vector_));
    }

   private:
    VectorType vector_;
};


}  // namespace nias

#endif  // NIAS_CPP_VECTOR_WRAPPER_H
