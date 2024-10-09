#ifndef NIAS_CPP_TYPE_TRAITS_H
#define NIAS_CPP_TYPE_TRAITS_H

#include <cstddef>
#include <type_traits>

using ssize_t = std::common_type_t<std::ptrdiff_t, std::make_signed_t<std::size_t>>;

#endif  // NIAS_CPP_TYPE_TRAITS_H
