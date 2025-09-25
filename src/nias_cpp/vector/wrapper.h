#ifndef NIAS_CPP_VECTOR_WRAPPER_H
#define NIAS_CPP_VECTOR_WRAPPER_H

#include <memory>
#include <variant>

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

namespace internal
{

// helper type for the variant visitor
template <class... Ts>
struct overloads : Ts...
{
    using Ts::operator()...;
};

}  // namespace internal

template <class VectorType>
    requires has_vector_traits<VectorType>
class VectorWrapper : public VectorInterface<typename VectorTraits<VectorType>::ScalarType>
{
   public:
    using F = typename VectorTraits<VectorType>::ScalarType;

    explicit VectorWrapper(const VectorType& vector, bool copy)
        : vector_(&vector)
    {
        if (copy)
        {
            vector_ = VectorTraits<VectorType>::copy(vector);
        }
    }

    explicit VectorWrapper(VectorType& vector)
        : vector_(&vector)
    {
    }

    explicit VectorWrapper(VectorType&& vector)
        : vector_(std::move(vector))
    {
    }

    VectorWrapper(const VectorWrapper& other, bool copy = true)
        : vector_(&other.get_vector())
    {
        if (copy)
        {
            vector_ = VectorTraits<VectorType>::copy(other.get_vector());
        }
    }

    VectorWrapper& operator=(const VectorWrapper& other)
    {
        if (this != &other)
        {
            vector_ = VectorTraits<VectorType>::copy(other.get_vector());
        }
        return *this;
    }

    VectorWrapper(VectorWrapper&& other) noexcept = default;
    VectorWrapper& operator=(VectorWrapper&& other) noexcept = default;
    ~VectorWrapper() override = default;

    [[nodiscard]] ssize_t dim() const override
    {
        return VectorTraits<VectorType>::dim(get_vector());
    }

    [[nodiscard]] F& operator[](ssize_t i) override
    {
        return VectorTraits<VectorType>::get(get_vector(), i);
    }

    [[nodiscard]] const F& operator[](ssize_t i) const override
    {
        return VectorTraits<VectorType>::const_get(get_vector(), i);
    }

    [[nodiscard]] VectorType& backend()
    {
        return get_vector();
    }

    [[nodiscard]] const VectorType& backend() const
    {
        return get_vector();
    }

    [[nodiscard]] std::shared_ptr<VectorInterface<F>> copy() const override
    {
        return std::make_shared<VectorWrapper>(VectorTraits<VectorType>::copy(get_vector()));
    }

   private:
    const VectorType& get_vector() const
    {
        return std::visit(
            internal::overloads{
                [](const VectorType& vec) -> const VectorType&
                {
                    // NOLINTNEXTLINE(bugprone-return-const-ref-from-parameter)
                    return vec;
                },
                [](const VectorType* vec) -> const VectorType&
                {
                    return *vec;
                },
            },
            vector_);
    }

    VectorType& get_vector()
    {
        return std::visit(
            internal::overloads{
                [](VectorType& vec) -> VectorType&
                {
                    return vec;
                },
                [](VectorType* vec) -> VectorType&
                {
                    return *vec;
                },
                [](const VectorType* /*vec*/) -> VectorType&
                {
                    throw InvalidStateError("Cannot return non-const reference to const VectorType");
                },
            },
            vector_);
    }

    // uses pointers instead of references to allow for copy and move assignment operators
    std::variant<VectorType, VectorType*, const VectorType*> vector_;
};


}  // namespace nias

#endif  // NIAS_CPP_VECTOR_WRAPPER_H
