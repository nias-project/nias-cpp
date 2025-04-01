#ifndef NIAS_CPP_TEST_COMMON_H
#define NIAS_CPP_TEST_COMMON_H

#include <algorithm>
#include <concepts>
#include <limits>
#include <ostream>

#include <nias_cpp/concepts.h>
#include <nias_cpp/type_traits.h>
#include <pybind11/numpy.h>

// NOLINTNEXTLINE(google-global-names-in-headers)
using namespace nias;

template <class Lhs, class Rhs>
    requires std::equality_comparable_with<Lhs, Rhs> && ostreamable<Lhs> && ostreamable<Rhs>
struct DefaultExactlyEqualOp
{
    DefaultExactlyEqualOp(const Lhs& lhs, const Rhs& rhs)
        : lhs_(lhs)
        , rhs_(rhs)
    {
    }

    [[nodiscard]] constexpr explicit operator bool() const
    {
        return lhs_ == rhs_;
    }

    friend std::ostream& operator<<(std::ostream& os, const DefaultExactlyEqualOp& eq)
    {
        return (os << eq.lhs_ << " == " << eq.rhs_);
    }

    const Lhs& lhs_;
    const Rhs& rhs_;
};

template <class Lhs, class Rhs>
    requires std::equality_comparable_with<Lhs, Rhs> && ostreamable<Lhs> && ostreamable<Rhs>
auto exactly_equal(const Lhs& lhs, const Rhs& rhs)
{
    return DefaultExactlyEqualOp(lhs, rhs);
}

template <floating_point_or_complex F>
struct FloatingPointApproxEqualOp
{
    using RealType = real_t<F>;

    FloatingPointApproxEqualOp(F lhs, F rhs)
        : lhs_(lhs)
        , rhs_(rhs)
    {
    }

    [[nodiscard]] constexpr explicit operator bool() const
    {
        constexpr RealType abs_tol = std::numeric_limits<RealType>::epsilon() * 10;
        constexpr RealType rel_tol = abs_tol;
        return std::abs(lhs_ - rhs_) < abs_tol + std::max(std::abs(lhs_), std::abs(lhs_)) * rel_tol;
    }

    friend std::ostream& operator<<(std::ostream& os, const FloatingPointApproxEqualOp& eq)
    {
        return (os << eq.lhs_ << " == " << eq.rhs_);
    }

    const F lhs_;
    const F rhs_;
};

template <floating_point_or_complex F>
auto approx_equal(F lhs, F rhs)
{
    return FloatingPointApproxEqualOp<F>(lhs, rhs);
}

#endif  // NIAS_CPP_TEST_COMMON_H
