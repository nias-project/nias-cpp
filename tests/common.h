#ifndef NIAS_CPP_TEST_COMMON_H
#define NIAS_CPP_TEST_COMMON_H

#include <algorithm>
#include <concepts>

#include <nias_cpp/checked_integer_cast.h>
#include <nias_cpp/concepts.h>
#include <nias_cpp/exceptions.h>
#include <nias_cpp/indices.h>
#include <nias_cpp/interfaces/vectorarray.h>
#include <nias_cpp/interpreter.h>
#include <nias_cpp/type_traits.h>
#include <nias_cpp/vectorarray/list.h>
#include <nias_cpp/vectorarray/numpy.h>
#include <pybind11/numpy.h>

namespace
{
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

    [[nodiscard]] constexpr operator bool() const
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
    FloatingPointApproxEqualOp(F lhs, F rhs)
        : lhs_(lhs)
        , rhs_(rhs)
    {
    }

    [[nodiscard]] constexpr operator bool() const
    {
        constexpr F abs_tol = std::numeric_limits<F>::epsilon() * 10;
        constexpr F rel_tol = abs_tol;
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


}  // namespace

#endif  // NIAS_CPP_TEST_COMMON_H
