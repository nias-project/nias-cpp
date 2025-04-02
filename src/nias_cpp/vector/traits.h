#ifndef NIAS_CPP_VECTOR_TRAITS_H
#define NIAS_CPP_VECTOR_TRAITS_H

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
 * See the \c has_vector_traits concept below for the exact list of requirements that the specialization has to fulfill.
 * See also the existing specializations for \c std::vector and vectors derived from \c VectorInterface in
 * nias_cpp/vector/stl.h and nias_cpp/interfaces/vector.h, respectively.
 */
template <class V>
struct VectorTraits;

template <class VectorType>
concept has_vector_traits = requires(VectorType vec) {
    typename VectorTraits<VectorType>::VectorType;
    typename VectorTraits<VectorType>::ScalarType;
    { VectorTraits<VectorType>::dim_(vec) } -> std::convertible_to<ssize_t>;
    { VectorTraits<VectorType>::copy_(vec) } -> std::same_as<VectorType>;
    {
        VectorTraits<VectorType>::get_(vec, 0)
    } -> std::same_as<typename VectorTraits<VectorType>::ScalarType&>;
    {
        VectorTraits<VectorType>::const_get_(vec, 0)
    } -> std::same_as<const typename VectorTraits<VectorType>::ScalarType&>;
};


}  // namespace nias

#endif  // NIAS_CPP_VECTOR_TRAITS_H
