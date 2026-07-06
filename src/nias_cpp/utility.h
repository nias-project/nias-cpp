#ifndef NIAS_CPP_UTILITY_H
#define NIAS_CPP_UTILITY_H

#include <type_traits>
#include <utility>

namespace nias
{


// C++23's std::forward_like, taken from https://en.cppreference.com/w/cpp/utility/forward_like.html
// Once all supported compiler implement std::forward_like we can use that instead.
template <class T, class U>
constexpr auto&& forward_like(U&& x) noexcept  // NOLINT(cppcoreguidelines-missing-std-forward)
{
    constexpr bool is_adding_const = std::is_const_v<std::remove_reference_t<T>>;
    if constexpr (std::is_lvalue_reference_v<T&&>)
    {
        if constexpr (is_adding_const)
        {
            return std::as_const(x);
        }
        else
        {
            return static_cast<U&>(x);  // NOLINT(readability-redundant-casting)
        }
    }
    else
    {
        if constexpr (is_adding_const)
        {
            return std::move(std::as_const(x));
        }
        else
        {
            return std::move(x);  // NOLINT(bugprone-move-forwarding-reference)
        }
    }
}


}  // namespace nias

#endif  // NIAS_CPP_UTILITY_H
