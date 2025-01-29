#ifndef NIAS_CPP_TEST_VECTORARRAY_COMMON_H
#define NIAS_CPP_TEST_VECTORARRAY_COMMON_H

#include <algorithm>
#include <limits>
#include <set>
#include <string>
#include <string_view>
#include <vector>

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

#include "../boost_ut_no_module.h"
#include "../test_module.h"
#include "boost/ut.hpp"

namespace
{
using namespace nias;
using namespace boost::ut;

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
                array->set(i, j, start + i * dim + j);
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

template <floating_point_or_complex F>
bool exactly_equal(const F& lhs, const F& rhs)
{
    return lhs == rhs;
}

template <floating_point_or_complex F>
bool exactly_equal(const VectorArrayInterface<F>& lhs, const VectorArrayInterface<F>& rhs)
{
    if (lhs.size() != rhs.size() || lhs.dim() != rhs.dim())
    {
        return false;
    }
    for (ssize_t i = 0; i < lhs.size(); ++i)
    {
        for (ssize_t j = 0; j < lhs.dim(); ++j)
        {
            if (lhs.get(i, j) != rhs.get(i, j))
            {
                return false;
            }
        }
    }
    return true;
}

template <floating_point_or_complex F>
bool approx_equal(const F& lhs, const F& rhs)
{
    constexpr F abs_tol = std::numeric_limits<F>::epsilon() * 10;
    constexpr F rel_tol = abs_tol;
    return std::abs(lhs - rhs) < abs_tol + std::max(std::abs(lhs), std::abs(lhs)) * rel_tol;
}

template <floating_point_or_complex F>
bool approx_equal(const VectorArrayInterface<F>& lhs, const VectorArrayInterface<F>& rhs)
{
    if (lhs.size() != rhs.size() || lhs.dim() != rhs.dim())
    {
        return false;
    }
    for (ssize_t i = 0; i < lhs.size(); ++i)
    {
        for (ssize_t j = 0; j < lhs.dim(); ++j)
        {
            if (!approx_equal((lhs.get(i, j), rhs.get(i, j))))
            {
                return false;
            }
        }
    }
    return true;
}

template <floating_point_or_complex F>
void check_copy(const VectorArrayInterface<F>& v, ssize_t size, ssize_t dim)
{
    using namespace boost::ut::bdd;

    given("A vectorarray v of size size and dimension dim") = [&]()
    {
        when("Calling v_copy = v.copy() without arguments") = [&]()
        {
            then("no exception is thrown") = [&]()
            {
                expect(nothrow(
                    [&]()
                    {
                        return v.copy();
                    }));
            };
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
                    expect(exactly_equal(v.get(0, 0), first_v_entry)) << "copy is deep";
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

        given("Non-empty indices") = [&]()
        {
            for (auto indices_vec :
                 {std::vector<ssize_t>({0, 2}), std::vector<ssize_t>({0, 0}), std::vector<ssize_t>{-1, 0, 1}})
            {
                const auto indices = Indices(indices_vec);
                const auto max_index = *std::ranges::max_element(indices_vec);
                const auto min_index = *std::ranges::min_element(indices_vec);

                if (max_index >= size || min_index < -size)
                {
                    when("indices contains an index i with i >= size or i < -size") = [&]()
                    {
                        then("v.copy(indices) throws an exception") = [&]()
                        {
                            expect(throws<InvalidIndexError>(
                                [&]()
                                {
                                    return v.copy(indices);
                                }));
                        };
                    };
                    continue;
                }
                when("All indices i fulfill -size <= i < size") = [&]()
                {
                    then("v.copy(indices) does not throw an exception") = [&]()
                    {
                        expect(nothrow(
                            [&]()
                            {
                                return v.copy(indices);
                            }));
                    };

                    auto v_copy = v.copy(indices);
                    then("v_copy = v.copy(indices) has correct dimensions and size") = [&]()
                    {
                        expect(v_copy->dim() == dim) << "dim should be unchanged";
                        expect(v_copy->size() == std::ssize(indices_vec))
                            << "size should be the number of indices";
                    };

                    then(
                        "v_copy = v.copy(indices) contains the correct vectors of v in the correct "
                        "order") = [&]()
                    {
                        ssize_t vec_index = 0;
                        for (auto i : indices_vec)
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
                        if (size > 0 && dim > 0)
                        {
                            const auto first_v_entry = v.get(0, 0);
                            v_copy->set(0, 0, F(42));
                            expect(exactly_equal(v.get(0, 0), first_v_entry)) << "copy is deep";
                        }
                    };
                };
            };
        };
    };
}

template <class VectorArray>
void check_append(const VectorArrayInterface<typename VectorArray::ScalarType>& v, ssize_t size, ssize_t dim)
{
    using namespace boost::ut::bdd;

    given("A vectorarray v of size size and dimension dim") = [&]()
    {
        for (ssize_t size2 : {0, 1, 2, 5})
        {
            given("Another vectorarray v2 with size size2 and same dimension dim") = [&]()
            {
                const auto v2_ptr = TestVectorArrayFactory<VectorArray>::iota(size2, dim);
                const auto& v2 = *v2_ptr;

                when("Calling v.append(v2) without indices") = [&]()
                {
                    // we need mutable versions of v and v2 to be able to modify them
                    auto v_mut = v.copy();
                    auto v2_mut = v2.copy();
                    then("No exception is thrown") = [&]()
                    {
                        expect(fatal(nothrow(
                            [&]()
                            {
                                v_mut->append(*v2_mut);
                            })));
                    };

                    then("v2 has been appended to the end of v") = [&]()
                    {
                        expect(exactly_equal((*v_mut)[Slice(0, v.size())], v))
                            << "v[0:size] should be the old v";
                        expect(exactly_equal((*v_mut)[Slice(v.size(), v.size() + size2)], v2))
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
                        expect(exactly_equal(*v2_mut, v2));
                    };
                };

                when("Calling v.append(v2, remove_from_other=true) without indices") = [&]()
                {
                    auto v_mut = v.copy();
                    auto v2_mut = v2.copy();
                    then("No exception is thrown") = [&]()
                    {
                        expect(fatal(nothrow(
                            [&]()
                            {
                                v_mut->append(*v2_mut, true);
                            })));
                    };

                    then("v has the same state as in the remove_from_other=false case") = [&]()
                    {
                        auto v_ref = v.copy();
                        v_ref->append(*v2.copy());
                        expect(exactly_equal(*v_mut, *v_ref));
                    };

                    then("v2 is now empty") = [&]()
                    {
                        expect(v2_mut->size() == 0);
                    };
                };

                when("Calling v.append(v2, remove_from_other, Indices{}) with empty indices") = [&]()
                {
                    auto v_mut1 = v.copy();
                    auto v_mut2 = v.copy();
                    auto v2_mut = v2.copy();
                    then("No exception is thrown") = [&]()
                    {
                        expect(fatal(nothrow(
                            [&]()
                            {
                                v_mut1->append(*v2.copy(), false, Indices{});
                                v_mut2->append(*v2_mut, true, Indices{});
                            })));
                    };
                    then("v and v2 are unchanged") = [&]()
                    {
                        expect(exactly_equal(*v_mut1, v));
                        expect(exactly_equal(*v_mut2, v));
                        expect(exactly_equal(*v2_mut, v2));
                    };
                };

                given("Non-empty indices") = [&]()
                {
                    for (auto indices_vec : {std::vector<ssize_t>({0, 2}), std::vector<ssize_t>({0, 0}),
                                             std::vector<ssize_t>{-1, 0, 1}})
                    {
                        const auto indices = Indices(indices_vec);
                        const auto max_index = *std::ranges::max_element(indices_vec);
                        const auto min_index = *std::ranges::min_element(indices_vec);

                        if (max_index >= size2 || min_index < -size2)
                        {
                            when("indices contains an index i with i >= size2 or i < -size2") = [&]()
                            {
                                then("v.append(v2, .., indices) throws an exception") = [&]()
                                {
                                    expect(throws<InvalidIndexError>(
                                        [&]()
                                        {
                                            return v.copy()->append(*v2.copy(), false, indices);
                                        }));
                                    expect(throws<InvalidIndexError>(
                                        [&]()
                                        {
                                            return v.copy()->append(*v2.copy(), true, indices);
                                        }));
                                };
                            };
                            continue;
                        }
                        when("Calling v.append(v2, remove_from_other=false, indices)") = [&]()
                        {
                            auto v_mut = v.copy();
                            auto v2_mut = v2.copy();
                            then("No exception is thrown") = [&]()
                            {
                                expect(fatal(nothrow(
                                    [&]()
                                    {
                                        v_mut->append(*v2_mut, false, indices);
                                    })));
                            };
                            const auto num_indices = std::ssize(indices_vec);
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
                                                     v2[indices]))
                                    << "v[size:size+num_indices] should be v2[indices]";
                            };
                            then("v2 has not been modified") = [&]()
                            {
                                expect(exactly_equal(v2, *v2_mut));
                            };
                        };
                        when("Calling v.append(v2, remove_from_other=true, indices)") = [&]()
                        {
                            auto v_mut = v.copy();
                            auto v2_mut = v2.copy();
                            then("No exception is thrown") = [&]()
                            {
                                expect(fatal(nothrow(
                                    [&]()
                                    {
                                        v_mut->append(*v2_mut, true, indices);
                                    })));
                            };
                            then("v has the same state as in the remove_from_other=false case") = [&]()
                            {
                                auto v_ref = v.copy();
                                v_ref->append(*v2.copy(), false, indices);
                                expect(exactly_equal(*v_mut, *v_ref));
                            };
                            then("the vectors corresponding to indices have been removed from v2") = [&]()
                            {
                                // We want to count unique indices.
                                // For that purpose, we first convert negative indices to positive ones...
                                auto positive_indices = indices_vec;
                                for (auto& i : positive_indices)
                                {
                                    if (i < 0)
                                    {
                                        i += size2;
                                    }
                                }
                                // and then use a set to remove duplicates
                                const auto num_unique_indices =
                                    std::ssize(std::set(positive_indices.begin(), positive_indices.end()));
                                expect(v2_mut->size() == size2 - num_unique_indices)
                                    << "vectors from v2_mut should have been moved";
                                // TODO: actually check vectors, not only the vectorarray size
                            };
                        };
                    }
                };
            };
        }
    };
}

template <floating_point_or_complex F>
void check_scal(const VectorArrayInterface<F>& v, ssize_t size, ssize_t dim)
{
    using namespace boost::ut::bdd;

    given("A vectorarray v of size size and dimension dim") = [&]()
    {
        given("A std::vector<F> alpha with size neither 1 nor size") = [&]()
        {
            const auto alpha = std::vector<F>(size + 2, F(42));
            when("Calling v.scal(alpha)") = [&]()
            {
                auto v_scaled_by_vec = v.copy();
                then("An exception is thrown") = [&]()
                {
                    expect(fatal(throws(
                        [&]()
                        {
                            v_scaled_by_vec->scal(alpha);
                        })));
                };
                then("v remains unchanged") = [&]()
                {
                    expect(exactly_equal(*v_scaled_by_vec, v));
                };
            };
        };

        given("A std::vector<F> alpha of size size") = [&]()
        {
            std::vector<F> alpha(size);
            for (ssize_t i = 0; i < size; ++i)
            {
                alpha[checked_integer_cast<size_t>(i)] = F(-1 + i);
            }
            when("Calling v.scal(alpha) or v.scal(alpha[0])") = [&]()
            {
                auto v_scaled_by_vec = v.copy();
                auto v_scaled_by_scalar = v.copy();

                then("No exception is thrown") = [&]()
                {
                    expect(fatal(nothrow(
                        [&]()
                        {
                            v_scaled_by_vec->scal(alpha);
                            v_scaled_by_scalar->scal(size > 0 ? alpha[0] : F(-1));
                        })));
                };
                then("dimensions of v remain unchanged") = [&]()
                {
                    for (const auto& v_new : std::vector({v_scaled_by_vec, v_scaled_by_scalar}))
                    {
                        expect(v_new->size() == size) << "size should be unchanged";
                        expect(v_new->dim() == dim) << "dim should be unchanged";
                    }
                };
                then("v has been scaled correctly") = [&]()
                {
                    for (ssize_t i = 0; i < size; ++i)
                    {
                        for (ssize_t j = 0; j < dim; ++j)
                        {
                            expect(v_scaled_by_vec->get(i, j) == alpha[i] * v.get(i, j));
                            expect(v_scaled_by_scalar->get(i, j) ==
                                   (size > 0 ? alpha[0] : F(-1)) * v.get(i, j));
                        }
                    }
                };
            };
        };

        given("Multiple indices") = [&]()
        {
            for (auto indices_vec : {std::vector<ssize_t>{}, std::vector<ssize_t>({0, 2}),
                                     std::vector<ssize_t>({0, 0}), std::vector<ssize_t>{-1, 0, 1}})
            {
                const auto indices = Indices(indices_vec);

                given("A std::vector<F> alpha of size != 1 and of a different size than the indices") = [&]()
                {
                    const auto alpha = std::vector<F>(indices_vec.size() + 2, F(42));
                    when("Calling v.scal(alpha, indices)") = [&]()
                    {
                        auto v_scaled_by_vec = v.copy();
                        then("An exception is thrown") = [&]()
                        {
                            expect(fatal(throws(
                                [&]()
                                {
                                    v_scaled_by_vec->scal(alpha, indices);
                                })));
                        };
                        then("v remains unchanged") = [&]()
                        {
                            expect(exactly_equal(*v_scaled_by_vec, v));
                        };
                    };
                };

                given("A std::vector<F> alpha of the same size as the indices") = [&]()
                {
                    const auto alpha = std::vector<F>(indices_vec.size(), F(42));

                    if (!indices_vec.empty() && (*std::ranges::max_element(indices_vec) >= size ||
                                                 *std::ranges::min_element(indices_vec) < -size))
                    {
                        when("indices contains an index i with i >= size or i < -size") = [&]()
                        {
                            then("v.scal(alpha, indices) throws an exception") = [&]()
                            {
                                expect(throws<InvalidIndexError>(
                                    [&]()
                                    {
                                        return v.copy()->scal(alpha, indices);
                                    }));
                                expect(throws<InvalidIndexError>(
                                    [&]()
                                    {
                                        return v.copy()->scal(F(-1), indices);
                                    }));
                            };
                        };
                    }
                    else
                    {
                        when("Calling v.scal(alpha, indices) or v.scal(alpha[0], indices)") = [&]()
                        {
                            auto v_scaled_by_vec = v.copy();
                            auto v_scaled_by_scalar = v.copy();

                            then("No exception is thrown") = [&]()
                            {
                                expect(fatal(nothrow(
                                    [&]()
                                    {
                                        v_scaled_by_vec->scal(alpha, indices);
                                        v_scaled_by_scalar->scal(indices_vec.size() > 0 ? alpha[0] : F(42),
                                                                 indices);
                                    })));
                            };
                            then("v has been scaled correctly") = [&]()
                            {
                                for (const auto& v_scaled : {v_scaled_by_vec, v_scaled_by_scalar})
                                {
                                    expect(v_scaled->size() == size) << "size should be unchanged";
                                    expect(v_scaled->dim() == dim) << "dim should be unchanged";
                                    for (ssize_t i = 0; i < size; ++i)
                                    {
                                        // alpha is a vector filled with 42s, so the entries should be scaled by 42^n
                                        // where n is the number of times the entry's index appears in indices_vec
                                        const auto n =
                                            std::ranges::count(indices_vec, i) +
                                            (size != 0) * std::ranges::count(indices_vec, i - size);
                                        for (ssize_t j = 0; j < dim; ++j)
                                        {
                                            expect(approx_equal(
                                                v_scaled->get(i, j),
                                                (n > 0 ? std::pow(F(42), n) : F(1.)) * v.get(i, j)))
                                                << "entries should be correctly scaled, n =" << n
                                                << ", i = " << i << ", j = " << j
                                                << ", v[i, j] = " << v.get(i, j)
                                                << ", v_scaled[i, j] = " << v_scaled->get(i, j);
                                        }
                                    }
                                }
                            };
                        };
                    }
                };
            }
        };
    };
}

template <class VectorArray>
void check_axpy(const VectorArrayInterface<typename VectorArray::ScalarType>& v, ssize_t size, ssize_t dim)
{
    //     using F = typename VectorArray::ScalarType;
    //     using namespace boost::ut::bdd;
    //
    //     given("A vectorarray v of size size and dimension dim") =
    //         [&]()
    //     {
    //         given("A std::vector<F> alpha of size size") = [&]()
    //         {
    //             std::vector<F> alpha(size);
    //             for (ssize_t i = 0; i < size; ++i)
    //             {
    //                 alpha[checked_integer_cast<size_t>(i)] = F(-1 + i);
    //             }
    //             given("Another vectorarray x of size size") = [&]()
    //             {
    //                 auto x_ptr = TestVectorArrayFactory<VectorArray>::iota(size, dim, F(-10));
    //                 const auto& x = *x_ptr;
    //
    //                 when("Performing axpy without indices") = [&]()
    //                 {
    //                     auto v_axpy_vec = v.copy();
    //                     auto v_axpy_scalar = v.copy();
    //
    //                     then("No exception is thrown") = [&]()
    //                     {
    //                         expect(fatal(nothrow(
    //                             [&]()
    //                             {
    //                                 v_axpy_vec->axpy(alpha, x);
    //                                 v_axpy_scalar->axpy(size > 0 ? alpha[0] : F(42), x);
    //                             })));
    //                     };
    //
    //                     then("The result has correct dimensions and size") = [&]()
    //                     {
    //                         for (const auto& v_axpy : {v_axpy_vec, v_axpy_scalar})
    //                         {
    //                             expect(v_axpy->size() == size) << "size should be unchanged";
    //                             expect(v_axpy->dim() == dim) << "dim should be unchanged";
    //                         }
    //                     };
    //                     then("The result is correct") = [&]()
    //                     {
    //                         for (ssize_t i = 0; i < size; ++i)
    //                         {
    //                             for (ssize_t j = 0; j < dim; ++j)
    //                             {
    //                                 expect(v_axpy_vec->get(i, j) == v.get(i, j) + (alpha[i] * x.get(i, j)))
    //                                     << "result of axpy should be correct";
    //                                 expect(v_axpy_scalar->get(i, j) == v.get(i, j) + (alpha[0] * x.get(i, j)))
    //                                     << "result of axpy should be correct";
    //                             }
    //                         }
    //                     };
    //                     };
    //
    //             when("Calling v.axpy(alpha, x, Indices{}) with empty indices and no x indices") = [&]()
    //             {
    //                 auto v_axpy_vec = v.copy();
    //                 auto v_axpy_scalar = v.copy();
    //                 if (size != 0)
    //                 {
    //                     then("An exception is thrown since the size of alpha does not match the indices") = [&]()
    //                     {
    //                         expect(throws<InvalidArgumentError>(
    //                             [&]()
    //                             {
    //                                 v_axpy_vec->scal(alpha, Indices{});
    //                             }));
    //                         expect(throws<InvalidArgumentError>(
    //                             [&]()
    //                             {
    //                                 v_axpy_scalar->scal(alpha[0], Indices{});
    //                             }));
    //                     };
    //                 }
    //                 else
    //                 {
    //                     then("No exception is thrown") = [&]()
    //                     {
    //                         expect(nothrow(
    //                             [&]()
    //                             {
    //                                 v_axpy_vec->scal(alpha, Indices{});
    //                             }));
    //                         expect(nothrow(
    //                             [&]()
    //                             {
    //                                 v_axpy_scalar->scal(alpha[0], Indices{});
    //                             }));
    //                     };
    //                 }
    //                 then("v remains unchanged") = [&]()
    //                 {
    //                     expect(exactly_equal(*v_scaled_by_vec, v));
    //                     expect(exactly_equal(*v_scaled_by_scalar, v));
    //                 };
    //             };
    //                 when("Performing axpy with empty indices") = [&]()
    //                 {
    //                     then("An exception is thrown ") = [&]()
    //                     {
    //                         auto v_axpy_vec = v.copy();
    //                         auto v_axpy_scalar = v.copy();
    //                         expect(fatal(nothrow(
    //                             [&]()
    //                             {
    //                                 v_axpy_vec->axpy(alpha, x, Indices{}, Indices{});
    //                                 v_axpy_scalar->axpy(size > 0 ? alpha[0] : F(42), x, Indices{}, Indices{});
    //                             })));
    //                         expect(exactly_equal(*v_axpy_vec, v));
    //                         expect(exactly_equal(*v_axpy_scalar, v));
    //                     };
    //
    //                     // non-empty indices
    //                     for (auto indices_set :
    //                          {std::set<ssize_t>({0, std::min(static_cast<ssize_t>(2), size - 1)}),
    //                           std::set<ssize_t>({0}),
    //                           std::set<ssize_t>{-1, 0, std::min(static_cast<ssize_t>(1), size - 1)}})
    //                     {
    //                         for (const auto& x_indices_set :
    //                              {std::set<ssize_t>({0, std::min(static_cast<ssize_t>(2), size - 1)}),
    //                               std::set<ssize_t>({0}),
    //                               std::set<ssize_t>{-1, 0, std::min(static_cast<ssize_t>(1), size - 1)}})
    //                         {
    //                             then("axpy with non-empty indices does not throw an exception") = [&]()
    //                             {
    //                                 auto v_copy1 = v.copy();
    //                                 auto v_copy2 = v.copy();
    //                                 const auto same_size = indices_set.size() == x_indices_set.size();
    //                                 auto x_copy = same_size ? x.copy()
    //                                                         : TestVectorArrayFactory<VectorArray>::iota(
    //                                                               indices_set.size(), dim, F(-10));
    //                                 alpha = std::vector<F>(indices_set.size(), F(42));
    //                                 const auto indices = Indices(indices_set);
    //                                 const auto x_indices =
    //                                     same_size ? std::optional<Indices>(Indices(x_indices_set)) : std::nullopt;
    //                                 const bool indices_unique =
    //                                     std::ranges::none_of(indices_set,
    //                                                          [&indices_set, size](ssize_t i)
    //                                                          {
    //                                                              return i < 0 && indices_set.contains(i + size);
    //                                                          });
    //
    //                                 if (size == 0 || !indices_unique)
    //                                 {
    //                                     expect(fatal(throws(
    //                                         [&]()
    //                                         {
    //                                             v_copy1->axpy(alpha, *x_copy, indices, x_indices);
    //                                             v_copy2->axpy(F(42), *x_copy, indices, x_indices);
    //                                         })));
    //                                 }
    //                                 else
    //                                 {
    //                                     expect(fatal(nothrow(
    //                                         [&]()
    //                                         {
    //                                             v_copy1->axpy(alpha, *x_copy, indices, x_indices);
    //                                             v_copy2->axpy(alpha[0], *x_copy, indices, x_indices);
    //                                         })));
    //                                     auto v_copies = std::vector({v_copy1, v_copy2});
    //                                     for (const auto& v_copy : v_copies)
    //                                     {
    //                                         expect(v_copy->size() == size) << "size should be unchanged";
    //                                         expect(v_copy->dim() == dim) << "dim should be unchanged";
    //                                         ssize_t alpha_index = 0;
    //                                         for (auto x_index_it = x_indices_set.begin();
    //                                              auto v_index : indices_set)
    //                                         {
    //                                             if (v_index < 0)
    //                                             {
    //                                                 v_index += size;
    //                                             }
    //                                             auto x_index = same_size ? *x_index_it : alpha_index;
    //                                             if (x_index < 0)
    //                                             {
    //                                                 x_index += size;
    //                                             }
    //                                             for (ssize_t j = 0; j < dim; ++j)
    //                                             {
    //                                                 expect(approx_equal(
    //                                                     v_copy->get(v_index, j),
    //                                                     v.get(v_index, j) + (F(42) * x.get(x_index, j))))
    //                                                     << "entries should have the correct entries";
    //                                             }
    //                                             if (same_size)
    //                                             {
    //                                                 ++x_index_it;
    //                                             }
    //                                             ++alpha_index;
    //                                         }
    //
    //                                         // check indices that are not in the indices set
    //                                         for (ssize_t i = 0; i < size; ++i)
    //                                         {
    //                                             if (indices_set.contains(i) || indices_set.contains(i - size))
    //                                             {
    //                                                 continue;
    //                                             }
    //                                             for (ssize_t j = 0; j < dim; ++j)
    //                                             {
    //                                                 expect(approx_equal(v_copy->get(i, j), v.get(i, j)))
    //                                                     << "entries should be correctly scaled";
    //                                             }
    //                                         }
    //                                     }
    //                                 }
    //                             };
    //                         }
    //                     }
    //                 };
    //             };
    // }
    //     }
}

}  // namespace

#endif  // NIAS_CPP_TEST_VECTORARRAY_COMMON_H
