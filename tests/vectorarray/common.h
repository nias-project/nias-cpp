#ifndef NIAS_CPP_TEST_VECTORARRAY_COMMON_H
#define NIAS_CPP_TEST_VECTORARRAY_COMMON_H

#include <algorithm>
#include <cstddef>
#include <format>
#include <memory>
#include <ostream>
#include <set>
#include <string>
#include <string_view>
#include <vector>

#include <nias_cpp/checked_integer_cast.h>
#include <nias_cpp/concepts.h>
#include <nias_cpp/exceptions.h>
#include <nias_cpp/indices.h>
#include <nias_cpp/interfaces/vector.h>
#include <nias_cpp/interfaces/vectorarray.h>
#include <nias_cpp/type_traits.h>
#include <nias_cpp/vectorarray/list.h>
#include <nias_cpp/vectorarray/numpy.h>
#include <pybind11/numpy.h>

#include "../boost_ext_ut_no_module.h"
#include "../common.h"
#include "../test_module.h"

// NOLINTBEGIN(google-global-names-in-headers)
using namespace nias;
using namespace boost::ut;

// NOLINTEND(google-global-names-in-headers)

// Comparison operators for VectorArrays
template <floating_point_or_complex F>
struct ExactlyEqualOpVecArray
{
    ExactlyEqualOpVecArray(const VectorArrayInterface<F>& lhs, const VectorArrayInterface<F>& rhs)
        : lhs_(lhs)
        , rhs_(rhs)
    {
    }

    [[nodiscard]] explicit operator bool() const
    {
        if (lhs_.size() != rhs_.size() || lhs_.dim() != rhs_.dim())
        {
            return false;
        }
        for (ssize_t i = 0; i < lhs_.size(); ++i)
        {
            for (ssize_t j = 0; j < lhs_.dim(); ++j)
            {
                if (lhs_.get(i, j) != rhs_.get(i, j))
                {
                    return false;
                }
            }
        }
        return true;
    }

    friend std::ostream& operator<<(std::ostream& os, const ExactlyEqualOpVecArray& eq)
    {
        return (os << "\n" << eq.lhs_ << " == " << "\n" << eq.rhs_);
    }

    const VectorArrayInterface<F>& lhs_;
    const VectorArrayInterface<F>& rhs_;
};

template <floating_point_or_complex F>
auto exactly_equal(const VectorArrayInterface<F>& lhs, const VectorArrayInterface<F>& rhs)
{
    return ExactlyEqualOpVecArray<F>(lhs, rhs);
}

template <floating_point_or_complex F>
struct ApproxEqualOpVecArray
{
    ApproxEqualOpVecArray(const VectorArrayInterface<F>& lhs, const VectorArrayInterface<F>& rhs)
        : lhs_(lhs)
        , rhs_(rhs)
    {
    }

    [[nodiscard]] explicit operator bool() const
    {
        if (lhs_.size() != rhs_.size() || lhs_.dim() != rhs_.dim())
        {
            return false;
        }
        for (ssize_t i = 0; i < lhs_.size(); ++i)
        {
            for (ssize_t j = 0; j < lhs_.dim(); ++j)
            {
                if (!approx_equal((lhs_.get(i, j), rhs_.get(i, j))))
                {
                    return false;
                }
            }
        }
        return true;
    }

    friend std::ostream& operator<<(std::ostream& os, const ApproxEqualOpVecArray& eq)
    {
        return (os << "\n" << eq.lhs_ << " == " << "\n" << eq.rhs_);
    }

    const VectorArrayInterface<F>& lhs_;
    const VectorArrayInterface<F>& rhs_;
};

template <floating_point_or_complex F>
auto approx_equal(const VectorArrayInterface<F>& lhs, const VectorArrayInterface<F>& rhs)
{
    return ApproxEqualOpVecArray<F>(lhs, rhs);
}

// Factories to create test values
template <class VectorArray>
struct TestVectorArrayFactory
{
    using F = typename VectorArray::ScalarType;

    static_assert(always_false<VectorArray>::value,
                  "Not implemented, specialize this template for the specific VectorArray type");

    static std::shared_ptr<VectorArrayInterface<F>> iota(ssize_t /*size*/, ssize_t /*dim*/, F /*start*/) {}
};

template <floating_point_or_complex F>
struct TestVectorArrayFactory<ListVectorArray<F>>
{
    static std::shared_ptr<VectorArrayInterface<F>> iota(ssize_t size, ssize_t dim, F start = F(1))
    {
        auto vec_array = std::make_shared<ListVectorArray<F>>(dim);
        auto current_number = start;
        for (ssize_t i = 0; i < size; ++i)
        {
            std::shared_ptr<VectorInterface<F>> const new_vec = std::make_shared<DynamicVector<F>>(dim);
            for (ssize_t j = 0; j < dim; ++j)
            {
                new_vec->get(j) = current_number;
                current_number += F(1);
            }
            vec_array->append(new_vec);
        }
        return vec_array;
    }
};

template <floating_point_or_complex F>
struct TestVectorArrayFactory<NumpyVectorArray<F>>
{
    static std::shared_ptr<VectorArrayInterface<F>> iota(ssize_t size, ssize_t dim, F start = F(1))
    {
        auto array = std::make_shared<NumpyVectorArray<F>>(size, dim);
        for (ssize_t i = 0; i < size; ++i)
        {
            for (ssize_t j = 0; j < dim; ++j)
            {
                array->set(i, j, start + (i * dim) + j);
            }
        }
        return array;
    }
};

template <floating_point_or_complex F>
struct TestVectorArrayFactory<pybind11::array_t<F>>
{
    static std::shared_ptr<pybind11::array_t<F>> iota(ssize_t size, ssize_t dim, F start = F(1))
    {
        auto array = std::make_shared<pybind11::array_t<F>>(std::vector{size, dim});
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        std::iota(array->mutable_data(), array->mutable_data() + (size * dim), start);
        return array;
    }
};

template <class F>
auto create_test_alphas(ssize_t size)
{
    // We test an empty vector, a vector of size 1, a vector of size size, and a vector thas has neither size 1 nor size (size + 2)
    auto ret = std::vector{std::vector<F>{}, std::vector<F>(1, F(42)), std::vector<F>(size, F(42)),
                           std::vector<F>(size + 2, F(42))};
    // and we add a vector of size size filled with the numbers -1, 0, 1, 2, ..., size - 1
    std::vector<F> iota_alpha(size);
    for (ssize_t i = 0; i < size; ++i)
    {
        iota_alpha[checked_integer_cast<size_t>(i)] = F(-1 + i);
    }
    ret.push_back(iota_alpha);
    return ret;
}

inline auto create_test_index_vectors(ssize_t size)
{
    return std::vector{std::vector<ssize_t>{}, std::vector<ssize_t>({0, 2}), std::vector<ssize_t>(size, 0),
                       std::vector<ssize_t>{-1, 0, 1}};
}

template <class VectorArray>
auto create_test_vectorarrays(ssize_t size, ssize_t dim)
{
    using VecArrayFactory = TestVectorArrayFactory<VectorArray>;
    using F = typename VectorArray::ScalarType;
    std::vector<std::shared_ptr<VectorArrayInterface<F>>> ret;
    for (const ssize_t sz : std::vector<ssize_t>{0, 1, size, size + 2})
    {
        ret.emplace_back(VecArrayFactory::iota(sz, dim, F(-1)));
    }
    return ret;
}

// some helper functions for testing
inline bool contains_invalid_index(const std::vector<ssize_t>& indices, ssize_t size)
{
    return std::ranges::any_of(indices,
                               [size](ssize_t i)
                               {
                                   return i >= size || i < -size;
                               });
}

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define VECTORARRAY_TEST_CHECK_NOTHROW(expression) \
    then("No exception is thrown") = [&]()         \
    {                                              \
        expect(fatal(nothrow(                      \
            [&]()                                  \
            {                                      \
                static_cast<void>(expression);     \
            })));                                  \
    }

#define VECTORARRAY_TEST_CHECK_THROWS(method_call, error_type) \
    when("Calling v." #method_call) = [&]()                    \
    {                                                          \
        auto v_tmp = v.copy();                                 \
        then("An exception is thrown") = [&]()                 \
        {                                                      \
            expect(fatal(throws<error_type>(                   \
                [&]()                                          \
                {                                              \
                    static_cast<void>(v_tmp->method_call);     \
                })));                                          \
        };                                                     \
        then("v remains unchanged") = [&]()                    \
        {                                                      \
            expect(exactly_equal(*v_tmp, v));                  \
        };                                                     \
    }

template <floating_point_or_complex F>
void assert_dimensions_are_unchanged(const VectorArrayInterface<F>& v, const VectorArrayInterface<F>& v_mod)
{
    using namespace boost::ut::bdd;
    then("dimensions of v remain unchanged") = [&]()
    {
        expect(v_mod.size() == v.size()) << "size should be unchanged";
        expect(v_mod.dim() == v.dim()) << "dim should be unchanged";
    };
}

template <floating_point_or_complex F>
void check_copy(const VectorArrayInterface<F>& v, ssize_t size, ssize_t dim)
{
    using namespace boost::ut::bdd;

    given(std::format("A vectorarray v of size {} and dimension {}", size, dim)) = [&]()
    {
        when("Calling v_copy = v.copy() without arguments") = [&]()
        {
            VECTORARRAY_TEST_CHECK_NOTHROW(v.copy());
            auto v_copy = v.copy();
            then("*v_copy equals v") = [&]()
            {
                expect(exactly_equal(*v_copy, v));
            };
            then("v_copy is a deep copy") = [&]()
            {
                if (size > 0 && dim > 0)
                {
                    const auto first_v_entry = v.get(0, 0);
                    v_copy->set(0, 0, F(42));
                    expect(exactly_equal(v.get(0, 0), first_v_entry));
                }
            };
        };

        when("Calling v_copy = v.copy(Indices{}) with empty indices") = [&]()
        {
            then("*v_copy is a vectorarray with size 0 and dimension dim") = [&]()
            {
                auto v_copy = v.copy(Indices{});
                expect(v_copy->dim() == dim) << "dim should be unchanged";
                expect(v_copy->size() == 0) << "size should be 0";
            };
        };

        given("Non-empty indices") = [&](const std::vector<ssize_t>& index_vec)
        {
            const auto indices = Indices(index_vec);

            if (contains_invalid_index(index_vec, size))
            {
                VECTORARRAY_TEST_CHECK_THROWS(copy(indices), InvalidIndexError);
            }
            else
            {
                when("All indices i fulfill -size <= i < size") = [&]()
                {
                    VECTORARRAY_TEST_CHECK_NOTHROW(v.copy(indices));

                    auto v_copy = v.copy(indices);
                    then("v_copy = v.copy(indices) has correct dimensions and size") = [&]()
                    {
                        expect(v_copy->dim() == dim) << "dim should be unchanged";
                        expect(v_copy->size() == std::ssize(index_vec))
                            << "size should be the number of indices";
                    };

                    then(
                        "v_copy = v.copy(indices) contains the correct vectors of v in the correct "
                        "order") = [&]()
                    {
                        ssize_t vec_index = 0;
                        for (auto i : index_vec)
                        {
                            // negative indices should be interpreted as counting from the end as in Python
                            if (i < 0)
                            {
                                i += size;
                            }
                            for (ssize_t j = 0; j < dim; ++j)
                            {
                                expect(exactly_equal(v_copy->get(vec_index, j), v.get(i, j)));
                            }
                            vec_index++;
                        }
                    };

                    then("v does not change if v_copy is modified (copy is deep)") = [&]()
                    {
                        if (size > 0 && dim > 0 && v_copy->size() > 0)
                        {
                            const auto first_v_entry = v.get(0, 0);
                            v_copy->set(0, 0, F(42));
                            expect(exactly_equal(v.get(0, 0), first_v_entry));
                        }
                    };
                };
            }
        } | create_test_index_vectors(size);
    };
}

template <class VectorArray>
void check_append(const VectorArrayInterface<typename VectorArray::ScalarType>& v, ssize_t size, ssize_t dim)
{
    using namespace boost::ut::bdd;

    given("A vectorarray v of size size and dimension dim") = [&]()
    {
        given("Another vectorarray v2 with size size2 and same dimension dim") = [&](const auto& v2)
        {
            const auto size2 = v2->size();

            when("Calling v.append(v2) without indices") = [&]()
            {
                // we need mutable versions of v and v2 to be able to modify them
                auto v_mut = v.copy();
                auto v2_mut = v2->copy();
                VECTORARRAY_TEST_CHECK_NOTHROW(v_mut->append(*v2_mut));

                then("v2 has been appended to the end of v") = [&]()
                {
                    expect(exactly_equal((*v_mut)[Slice(0, v.size())], v)) << "v[0:size] should be the old v";
                    expect(exactly_equal((*v_mut)[Slice(v.size(), v.size() + size2)], *v2))
                        << "v[size:size+v2_size] should be v2";
                };
                then("v has size size + size2 and unchanged dimension") = [&]()
                {
                    expect(v_mut->size() == size + size2) << "size should be the sum of the sizes";
                    expect(v_mut->dim() == dim) << "dim should be unchanged";
                };

                then("v2 has not been modified") = [&]()
                {
                    expect(v2_mut->size() == size2);
                    expect(exactly_equal(*v2_mut, *v2));
                };
            };

            when("Calling v.append(v2, remove_from_other=true) without indices") = [&]()
            {
                auto v_mut = v.copy();
                auto v2_mut = v2->copy();

                VECTORARRAY_TEST_CHECK_NOTHROW(v_mut->append(*v2_mut, true));

                then("v has the same state as in the remove_from_other=false case") = [&]()
                {
                    auto v_ref = v.copy();
                    v_ref->append(*v2->copy());
                    expect(exactly_equal(*v_mut, *v_ref));
                };

                then("v2 is now empty") = [&]()
                {
                    expect(v2_mut->size() == 0);
                };
            };

            given("Multiple indices") = [&](const std::vector<ssize_t>& index_vec)
            {
                const auto indices = Indices(index_vec);

                if (contains_invalid_index(index_vec, size2))
                {
                    VECTORARRAY_TEST_CHECK_THROWS(append(*v2->copy(), false, indices), InvalidIndexError);
                    VECTORARRAY_TEST_CHECK_THROWS(append(*v2->copy(), true, indices), InvalidIndexError);
                }
                else
                {
                    when("Calling v.append(v2, remove_from_other=false, indices)") = [&]()
                    {
                        auto v_mut = v.copy();
                        auto v2_mut = v2->copy();

                        VECTORARRAY_TEST_CHECK_NOTHROW(v_mut->append(*v2_mut, false, indices));

                        const auto num_indices = std::ssize(index_vec);
                        then("v.size() is original size + number of indices") = [&]()
                        {
                            expect(v_mut->size() == size + num_indices);
                        };
                        then("v.dim() is unchanged") = [&]()
                        {
                            expect(v_mut->dim() == dim);
                        };
                        then("The correct vectors of v2 have been appended to v") = [&]()
                        {
                            expect(exactly_equal((*v_mut)[Slice(0, v.size())], v))
                                << "v[0:size] should be the original v";
                            expect(exactly_equal((*v_mut)[Slice(v.size(), v.size() + num_indices)],
                                                 (*v2)[indices]))
                                << "v[size:size+num_indices] should be v2[indices]";
                        };
                        then("v2 has not been modified") = [&]()
                        {
                            expect(exactly_equal(*v2, *v2_mut));
                        };
                    };
                    when("Calling v.append(v2, remove_from_other=true, indices)") = [&]()
                    {
                        auto v_mut = v.copy();
                        auto v2_mut = v2->copy();

                        VECTORARRAY_TEST_CHECK_NOTHROW(v_mut->append(*v2_mut, true, indices));

                        then("v has the same state as in the remove_from_other=false case") = [&]()
                        {
                            auto v_ref = v.copy();
                            v_ref->append(*v2->copy(), false, indices);
                            expect(exactly_equal(*v_mut, *v_ref));
                        };
                        then("the vectors corresponding to indices have been removed from v2") = [&]()
                        {
                            // We want to count unique indices.
                            // For that purpose, we first convert negative indices to positive ones...
                            auto positive_indices = index_vec;
                            for (auto& i : positive_indices)
                            {
                                if (i < 0)
                                {
                                    i += size2;
                                }
                            }
                            // ... and then use a set to remove duplicates
                            const auto num_unique_indices =
                                std::ssize(std::set(positive_indices.begin(), positive_indices.end()));
                            expect(v2_mut->size() == size2 - num_unique_indices)
                                << "vectors should have been removed from v2";
                        };
                    };
                }
            } | create_test_index_vectors(size2);
        } | create_test_vectorarrays<VectorArray>(size, dim);
    };
}

template <floating_point_or_complex F>
void check_scal(const VectorArrayInterface<F>& v, ssize_t size, ssize_t dim)
{
    using namespace boost::ut::bdd;

    given("A vectorarray v of size size and dimension dim") = [&]()
    {
        given("Multiple std::vector<F> alpha") = [&](const auto& alpha)
        {
            if (std::ssize(alpha) != 1 && std::ssize(alpha) != size)
            {
                when("alpha's size is neither 1 nor size") = [&]()
                {
                    VECTORARRAY_TEST_CHECK_THROWS(scal(alpha), InvalidArgumentError);
                };
            }
            else
            {
                when("alpha's size is 1 or size") = [&]()
                {
                    when("Calling v.scal(alpha)") = [&]()
                    {
                        auto v_scaled = v.copy();

                        VECTORARRAY_TEST_CHECK_NOTHROW(v_scaled->scal(alpha));

                        assert_dimensions_are_unchanged(v, *v_scaled);

                        then("v has been scaled correctly") = [&]()
                        {
                            for (ssize_t i = 0; i < size; ++i)
                            {
                                const auto scaling_factor = alpha.size() == 1 ? alpha[0] : alpha[i];
                                for (ssize_t j = 0; j < dim; ++j)
                                {
                                    expect(v_scaled->get(i, j) == scaling_factor * v.get(i, j));
                                }
                            }
                        };
                    };
                };
            }

            given("Multiple indices") = [&](const std::vector<ssize_t>& index_vec)
            {
                const auto indices = Indices(index_vec);

                if (contains_invalid_index(index_vec, size))
                {
                    VECTORARRAY_TEST_CHECK_THROWS(scal(alpha, indices), InvalidIndexError);
                    VECTORARRAY_TEST_CHECK_THROWS(scal(F(-1), indices), InvalidIndexError);
                }
                else
                {
                    if (alpha.size() != 1 && alpha.size() != index_vec.size())
                    {
                        when("alpha's size is neither 1 nor equal the number of indices") = [&]()
                        {
                            VECTORARRAY_TEST_CHECK_THROWS(scal(alpha, indices), InvalidArgumentError);
                        };
                    }
                    else
                    {
                        when("alpha's size is 1 or equals the number of indices") = [&]()
                        {
                            when("Calling v.scal(alpha, indices)") = [&]()
                            {
                                auto v_scaled = v.copy();

                                VECTORARRAY_TEST_CHECK_NOTHROW(v_scaled->scal(alpha, indices));

                                assert_dimensions_are_unchanged(v, *v_scaled);

                                then("v has been scaled correctly") = [&]()
                                {
                                    for (ssize_t i = 0; i < size; ++i)
                                    {
                                        F scaling_factor = F(1.);
                                        for (size_t k = 0; k < index_vec.size(); ++k)
                                        {
                                            if (index_vec[k] == i || index_vec[k] == i - size)
                                            {
                                                scaling_factor *= alpha.size() == 1 ? alpha[0] : alpha[k];
                                            }
                                        }
                                        for (ssize_t j = 0; j < dim; ++j)
                                        {
                                            expect(approx_equal(v_scaled->get(i, j),
                                                                scaling_factor * v.get(i, j)))
                                                << ", i = " << i << ", j = " << j
                                                << ", v[i, j] = " << v.get(i, j)
                                                << ", v_scaled[i, j] = " << v_scaled->get(i, j);
                                        }
                                    }
                                };
                            };
                        };
                    }
                }
            } | create_test_index_vectors(std::ssize(alpha));
        } | create_test_alphas<F>(size);
    };
}

template <class VectorArray>
void check_axpy(const VectorArrayInterface<typename VectorArray::ScalarType>& v, ssize_t size, ssize_t dim)
{
    using namespace boost::ut::bdd;
    using F = typename VectorArray::ScalarType;
    using VecArrayFactory = TestVectorArrayFactory<VectorArray>;

    given("A vectorarray v of size size and dimension dim") = [&]()
    {
        given("Multiple std::vector<F> alpha") = [&](const std::vector<F>& alpha)
        {
            if (std::ssize(alpha) != 1 && std::ssize(alpha) != size)
            {
                when("alpha's size is neither 1 nor size") = [&]()
                {
                    given("Another vectorarray x of size size") = [&]()
                    {
                        const auto x = VecArrayFactory::iota(size, dim, F(-1));
                        VECTORARRAY_TEST_CHECK_THROWS(axpy(alpha, *x), InvalidArgumentError);
                    };
                };
            }
            else
            {
                when("alpha's size is 1 or size") = [&]()
                {
                    given("A vectorarray x with a dimension different from dim") = [&]()
                    {
                        const auto x = VecArrayFactory::iota(size, dim + 1, F(-1));
                        VECTORARRAY_TEST_CHECK_THROWS(axpy(alpha, *x), InvalidArgumentError);
                    };
                    given("Multiple other vectorarrays x with dimension dim") = [&](const auto& x)
                    {
                        if (x->size() != 1 && x->size() != v.size())
                        {
                            when("x's size is neither 1 nor size") = [&]()
                            {
                                VECTORARRAY_TEST_CHECK_THROWS(axpy(alpha, *x), InvalidArgumentError);
                            };
                        }
                        else
                        {
                            when("x's size is 1 or size") = [&]()
                            {
                                when("Calling v.axpy(alpha, x)") = [&]()
                                {
                                    auto v_axpy = v.copy();

                                    VECTORARRAY_TEST_CHECK_NOTHROW(v_axpy->axpy(alpha, *x));

                                    assert_dimensions_are_unchanged(v, *v_axpy);

                                    then("v contains the correct values") = [&]()
                                    {
                                        for (ssize_t i = 0; i < size; ++i)
                                        {
                                            const auto alpha_index = alpha.size() == 1 ? 0 : i;
                                            const auto x_index = x->size() == 1 ? 0 : i;
                                            for (ssize_t j = 0; j < dim; ++j)
                                            {
                                                expect(v_axpy->get(i, j) ==
                                                       v.get(i, j) +
                                                           (alpha[alpha_index] * x->get(x_index, j)));
                                            }
                                        }
                                    };
                                };
                            };
                        }
                    } | create_test_vectorarrays<VectorArray>(size, dim);
                };
            }

            given("Multiple indices") = [&](const std::vector<ssize_t>& index_vec)
            {
                const auto indices = Indices(index_vec);

                if (contains_invalid_index(index_vec, size))
                {
                    given("Another vectorarray x of size indices.size()") = [&]()
                    {
                        const auto x = VecArrayFactory::iota(std::ssize(index_vec), dim, F(-1));
                        VECTORARRAY_TEST_CHECK_THROWS(axpy(alpha, *x, indices), InvalidIndexError);
                        VECTORARRAY_TEST_CHECK_THROWS(axpy(F(-1), *x, indices), InvalidIndexError);
                    };
                }
                else
                {
                    if (alpha.size() != 1 && alpha.size() != index_vec.size())
                    {
                        when("alpha's size is neither 1 nor equal the number of indices") = [&]()
                        {
                            given("Another vectorarray x of size indices.size()") = [&]()
                            {
                                const auto x = VecArrayFactory::iota(size, dim, F(-1));
                                VECTORARRAY_TEST_CHECK_THROWS(axpy(alpha, *x, indices), InvalidArgumentError);
                            };
                        };
                    }
                    else
                    {
                        when("alpha's size is 1 or equals the number of indices") = [&]()
                        {
                            given("A vectorarray x with a dimension different from dim") = [&]()
                            {
                                const auto x = VecArrayFactory::iota(size, dim + 1, F(-1));
                                VECTORARRAY_TEST_CHECK_THROWS(axpy(alpha, *x, indices), InvalidArgumentError);
                            };
                            given("Multiple other vectorarrays x with dimension dim") = [&](const auto& x)
                            {
                                if (x->size() != 1 && x->size() != std::ssize(index_vec))
                                {
                                    when("x's size is neither 1 nor indices.size()") = [&]()
                                    {
                                        VECTORARRAY_TEST_CHECK_THROWS(axpy(alpha, *x, indices),
                                                                      InvalidArgumentError);
                                    };
                                }
                                else
                                {
                                    when("x's size is 1 or indices.size()") = [&]()
                                    {
                                        when("Calling v.axpy(alpha, x, indices)") = [&]()
                                        {
                                            auto v_axpy = v.copy();

                                            VECTORARRAY_TEST_CHECK_NOTHROW(v_axpy->axpy(alpha, *x, indices));

                                            assert_dimensions_are_unchanged(v, *v_axpy);

                                            for (ssize_t i = 0; i < size; ++i)
                                            {
                                                const auto i_in_indices =
                                                    std::ranges::find_if(index_vec,
                                                                         [i, size](ssize_t index)
                                                                         {
                                                                             return index == i ||
                                                                                    index == i - size;
                                                                         }) != index_vec.end();
                                                if (i_in_indices)
                                                {
                                                    when(std::format("i={} is in index_vec", i)) = [&]()
                                                    {
                                                        then(std::format("v[{}] has been updated correctly",
                                                                         i)) = [&]()
                                                        {
                                                            for (ssize_t j = 0; j < dim; ++j)
                                                            {
                                                                F alpha_x_ij = F(0.);
                                                                for (size_t k = 0; k < index_vec.size(); ++k)
                                                                {
                                                                    if (index_vec[k] == i ||
                                                                        index_vec[k] == i - size)
                                                                    {
                                                                        alpha_x_ij +=
                                                                            (alpha.size() == 1 ? alpha[0]
                                                                                               : alpha[k]) *
                                                                            (x->size() == 1 ? x->get(0, j)
                                                                                            : x->get(k, j));
                                                                    }
                                                                }
                                                                expect(approx_equal(v_axpy->get(i, j),
                                                                                    v.get(i, j) + alpha_x_ij))
                                                                    << ", i = " << i << ", j = " << j
                                                                    << ", v[i, j] = " << v.get(i, j)
                                                                    << ", v_axpy[i, j] = "
                                                                    << v_axpy->get(i, j);
                                                            }
                                                        };
                                                    };
                                                }
                                                else
                                                {
                                                    when(std::format("i={} is not in index_vec", i)) = [&]()
                                                    {
                                                        then(std::format("v[{}] has not been modified", i)) =
                                                            [&]()
                                                        {
                                                            for (ssize_t j = 0; j < dim; ++j)
                                                            {
                                                                expect(v_axpy->get(i, j) == v.get(i, j));
                                                            }
                                                        };
                                                    };
                                                }
                                            }
                                        };
                                    };
                                }
                            } | create_test_vectorarrays<VectorArray>(std::ssize(index_vec), dim);
                        };
                    }
                }
            } | create_test_index_vectors(std::ssize(alpha));
        } | create_test_alphas<F>(size);

        // TODO: tests including x_indices
    };
}

#endif  // NIAS_CPP_TEST_VECTORARRAY_COMMON_H
