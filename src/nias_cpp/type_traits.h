#ifndef NIAS_CPP_TYPE_TRAITS_H
#define NIAS_CPP_TYPE_TRAITS_H

#include <cstddef>
#include <type_traits>

using ssize_t = std::common_type_t<std::ptrdiff_t, std::make_signed_t<std::size_t>>;

namespace nias
{

/**
 * \brief Simple template struct with member \c value that always evaluates to \c false.
 * \sa    https://artificial-mind.net/blog/2020/10/03/always-false
 */
template <typename T>
struct always_false : public std::false_type
{
};

}  // namespace nias

#endif  // NIAS_CPP_TYPE_TRAITS_H
