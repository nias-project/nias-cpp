#ifndef NIAS_CPP_CONCEPTS_H
#define NIAS_CPP_CONCEPTS_H

#include <complex>
#include <concepts>
#include <type_traits>

namespace nias
{


template <class T, class... Types>
concept any_of = (std::is_same_v<T, Types> || ...);

template <class T, class... Types>
concept none_of = !any_of<T, Types...>;

/**
 * \brief Standard character types
 * \see https://en.cppreference.com/w/cpp/language/types#Character_types
 */
template <class T>
concept character = any_of<T, char, signed char, unsigned char, char8_t, char16_t, char32_t, wchar_t>;

/**
 * \brief Integral types without \c bool and without character types
 * \see https://en.cppreference.com/w/cpp/language/types#Integral_types
*/
template <class T>
concept integer = std::integral<T> && !character<T> && !std::same_as<T, bool>;

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

// Concept for printing
template <class T>
concept ostreamable = requires(T t, std::ostream& os) { os << t; };


}  // namespace nias

#endif  // NIAS_CPP_CONCEPTS_H
