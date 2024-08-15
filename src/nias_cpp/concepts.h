#ifndef NIAS_CPP_CONCEPTS_H
#define NIAS_CPP_CONCEPTS_H

#include <complex>
#include <concepts>
#include <type_traits>

namespace nias
{


// Helper trait to be able to define a concept for complex numbers
template <class T>
struct is_complex : public std::false_type
{
};

template <class F>
struct is_complex<std::complex<F>> : public std::true_type
{
};

template <class T>
inline constexpr bool is_complex_v = is_complex<T>::value;

// Concept for complex numbers
template <class T>
concept complex = is_complex_v<T>;

// Concept for floating point numbers or complex numbers
template <class T>
concept floating_point_or_complex = std::floating_point<T> || complex<T>;


}  // namespace nias

#endif  // NIAS_CPP_CONCEPTS_H
