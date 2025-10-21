#ifndef NIAS_CPP_VECTOR_TRAITS_H
#define NIAS_CPP_VECTOR_TRAITS_H

#include <concepts>

#include <nias_cpp/type_traits.h>

namespace nias
{

/**
 * \brief Traits class for vector types.
 *
 * This class provides a uniform interface to wrap different vector types such that nias_cpp can
 * work with them. In order to wrap your own vector type, you need to specialize this struct for
 * your vector type. The specialization has to contain the the following typedefs and methods:
 * - \c VectorType: The type of the vector
 * - \c ScalarType: The type of the entries of the vector
 * - \c dim_: A callable that takes a \c VectorType object and that returns the dimension (length) of that object
 * - \c copy_: A callable that takes a \c VectorType object and returns a copy of that object
 * - \c get_: A callable that takes a \c VectorType object and an index and returns a reference to the entry at that index
 * - \c const_get_: A callable that takes a \c VectorType object and an index and returns a const reference to the entry at that index
 *
 * See the \c wrappable_vector concept below for the exact list of requirements that the specialization has to fulfill.
 * See also the existing specializations for \c std::vector and vectors derived from \c VectorInterface in
 * nias_cpp/vector/stl.h and nias_cpp/interfaces/vector.h, respectively.
 */
template <class V>
struct VectorWrapperTraits;

template <class VectorType>
concept wrappable_vector = requires(VectorType vec) {
    typename VectorWrapperTraits<VectorType>::VectorType;
    typename VectorWrapperTraits<VectorType>::ScalarType;
    { VectorWrapperTraits<VectorType>::dim(vec) } -> std::same_as<ssize_t>;
    { VectorWrapperTraits<VectorType>::copy(vec) } -> std::same_as<VectorType>;
    {
        VectorWrapperTraits<VectorType>::get(vec, 0)
    } -> any_of<typename VectorWrapperTraits<VectorType>::ScalarType&,
                typename VectorWrapperTraits<VectorType>::ScalarType>;
    {
        VectorWrapperTraits<VectorType>::const_get(vec, 0)
    } -> any_of<const typename VectorWrapperTraits<VectorType>::ScalarType&,
                typename VectorWrapperTraits<VectorType>::ScalarType>;
};


}  // namespace nias

#endif  // NIAS_CPP_VECTOR_TRAITS_H
