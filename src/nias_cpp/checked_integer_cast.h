#ifndef NIAS_CPP_CHECKED_INTEGER_CAST_H
#define NIAS_CPP_CHECKED_INTEGER_CAST_H

#include <utility>

#include <nias_cpp/concepts.h>
#include <nias_cpp/exceptions.h>

namespace nias
{


template <integer T, integer S>
constexpr T checked_integer_cast(S source)
{
    std::in_range<T>(source) ? (void)0 : throw OverflowError("checked_integer_cast: overflow");
    return static_cast<T>(source);
}

template <integer S>
constexpr size_t as_size_t(S source)
{
    return checked_integer_cast<size_t>(source);
}

template <integer S>
constexpr ssize_t as_ssize_t(S source)
{
    return checked_integer_cast<ssize_t>(source);
}


}  // namespace nias

#endif  // NIAS_CPP_CHECKED_INTEGER_CAST_H
