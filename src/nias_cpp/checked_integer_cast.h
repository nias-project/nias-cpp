#ifndef NIAS_CPP_CHECKED_INTEGER_CAST_H
#define NIAS_CPP_CHECKED_INTEGER_CAST_H

#include <stdexcept>
#include <utility>

#include <nias_cpp/concepts.h>

namespace nias
{


template <integer T, integer S>
constexpr T checked_integer_cast(S source)
{
    std::in_range<T>(source) ? (void)0 : throw std::overflow_error("checked_integer_cast: overflow");
    return static_cast<T>(source);
}


}  // namespace nias

#endif  // NIAS_CPP_CHECKED_INTEGER_CAST_H
