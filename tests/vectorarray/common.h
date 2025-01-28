#ifndef NIAS_CPP_TEST_VECTORARRAY_COMMON_H
#define NIAS_CPP_TEST_VECTORARRAY_COMMON_H

#include <algorithm>
#include <limits>
#include <set>
#include <string>
#include <string_view>
#include <vector>

#include <nias_cpp/concepts.h>
#include <nias_cpp/indices.h>
#include <nias_cpp/interfaces/vectorarray.h>
#include <nias_cpp/interpreter.h>
#include <nias_cpp/type_traits.h>
#include <nias_cpp/vectorarray/list.h>
#include <nias_cpp/vectorarray/numpy.h>
#include <pybind11/numpy.h>

#include "../boost_ut_no_module.h"
#include "../test_module.h"

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
    // check copy without arguments
    auto v_copy = v.copy();
    expect(exactly_equal(*v_copy, v));
    if (size > 0 && dim > 0)
    {
        const auto first_v_entry = v.get(0, 0);
        v_copy->set(0, 0, F(42));
        expect(exactly_equal(v.get(0, 0), first_v_entry)) << "copy is deep";
    }

    // check copy with indices
    // empty indices
    auto v_copy_with_empty_indices = v.copy(Indices{});
    expect(v_copy_with_empty_indices->dim() == dim) << "dim should be unchanged";
    expect(v_copy_with_empty_indices->size() == 0) << "size should be 0";
    // non-empty indices
    for (auto indices_vec :
         {std::vector<ssize_t>({0, 2}), std::vector<ssize_t>({0, 0}), std::vector<ssize_t>{-1, 0, 1}})
    {
        const auto indices = Indices(indices_vec);
        const auto max_index = *std::ranges::max_element(indices_vec);
        const auto min_index = *std::ranges::min_element(indices_vec);
        if (max_index >= size || min_index < -size)
        {
            expect(throws(
                [&]()
                {
                    return v.copy(indices);
                }));
            break;
        }
        expect(nothrow(
            [&]()
            {
                return v.copy(indices);
            }));
        auto v_copy_with_indices = v.copy(indices);
        expect(v_copy_with_indices->dim() == dim) << "dim should be unchanged";
        expect(v_copy_with_indices->size() == std::ssize(indices_vec))
            << "size should be the number of indices";
        // check that copy contains the correct vectors in the correct order
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
                expect(exactly_equal(v_copy_with_indices->get(vec_index, j), v.get(i, j)));
            }
            vec_index++;
        }
        // check that v does not change if copy is modified
        if (size > 0 && dim > 0)
        {
            const auto first_v_entry = v.get(0, 0);
            v_copy_with_indices->set(0, 0, F(42));
            expect(exactly_equal(v.get(0, 0), first_v_entry)) << "copy is deep";
        }
    }
}

template <class VectorArray>
void check_append(const VectorArrayInterface<typename VectorArray::ScalarType>& v, ssize_t size, ssize_t dim)
{
    // create vectorarray with known size to append
    constexpr ssize_t v2_size = 5;
    auto v2_ptr = TestVectorArrayFactory<VectorArray>::iota(v2_size, dim);
    auto& v2 = *v2_ptr;
    // check append without indices (both with remove_from_other = false and with remove_from_other = true)
    auto v2_copy = v2.copy();
    auto v_copy1 = v.copy();
    auto v_copy2 = v.copy();
    expect(fatal(nothrow(
        [&]()
        {
            v_copy1->append(v2);
            v_copy2->append(*v2_copy, true);
        })));
    const auto v_copies = std::vector({v_copy1, v_copy2});
    for (const auto& v_copy : v_copies)
    {
        expect(v_copy->size() == size + v2_size) << "size should be the sum of the sizes";
        expect(v_copy->dim() == dim) << "dim should be unchanged";
        expect(exactly_equal((*v_copy)[Slice(0, v.size())], v)) << "v_copy[0:size] should be v";
        expect(exactly_equal((*v_copy)[Slice(v.size(), v.size() + v2_size)], v2))
            << "v_copy[size:size+v2_size] should be v2";
    }
    expect(v2.size() == v2_size) << "v2 should not be modified";
    expect(v2_copy->size() == 0) << "vectors from v2_copy should have been moved";

    // check_append with indices
    // empty indices
    v_copy1 = v.copy();
    v_copy2 = v.copy();
    v2_copy = v2.copy();
    expect(fatal(nothrow(
        [&]()
        {
            v_copy1->append(v2, false, Indices{});
            v_copy2->append(*v2_copy, true, Indices{});
        })));
    expect(exactly_equal(*v_copy1, v));
    expect(exactly_equal(*v_copy2, v));
    expect(exactly_equal(*v2_copy, v2)) << "v2 should not be modified";
    // non-empty indices
    for (auto indices_vec :
         {std::vector<ssize_t>({0, 2}), std::vector<ssize_t>({0, 0}), std::vector<ssize_t>{-1, 0, 1}})
    {
        // reset vectorarrays
        v_copy1 = v.copy();
        v_copy2 = v.copy();
        v2_copy = v2.copy();
        const auto indices = Indices(indices_vec);
        expect(fatal(nothrow(
            [&]()
            {
                v_copy1->append(v2, false, indices);
                v_copy2->append(*v2_copy, true, indices);
            })));
        const auto v_copies = std::vector({v_copy1, v_copy2});
        const auto num_indices = std::ssize(indices_vec);
        for (const auto& v_copy : v_copies)
        {
            expect(v_copy->size() == size + num_indices)
                << "size should be original size + number of indices";
            expect(v_copy->dim() == dim) << "dim should be unchanged";
            expect(exactly_equal((*v_copy)[Slice(0, v.size())], v)) << "v_copy[0:size] should be v";
            expect(exactly_equal((*v_copy)[Slice(v.size(), v.size() + num_indices)], v2[indices]))
                << "v_copy[size:size+num_indices] should be v2[indices]";
        }
        expect(v2.size() == v2_size) << "v2 should not be modified";
        const auto num_unique_indices = std::ssize(std::set(indices_vec.begin(), indices_vec.end()));
        expect(v2_copy->size() == v2_size - num_unique_indices)
            << "vectors from v2_copy should have been moved";
    }
}

template <floating_point_or_complex F>
void check_scal(const VectorArrayInterface<F>& v, ssize_t size, ssize_t dim)
{
    // check scal without indices
    auto v_copy1 = v.copy();
    auto v_copy2 = v.copy();
    std::vector<F> alpha(size, F(42));
    expect(fatal(nothrow(
        [&]()
        {
            v_copy1->scal(alpha);
            v_copy2->scal(size > 0 ? alpha[0] : F(42));
        })));
    auto v_copies = std::vector({v_copy1, v_copy2});
    for (const auto& v_copy : v_copies)
    {
        expect(v_copy->size() == size) << "size should be unchanged";
        expect(v_copy->dim() == dim) << "dim should be unchanged";
    }
    for (ssize_t i = 0; i < size; ++i)
    {
        for (ssize_t j = 0; j < dim; ++j)
        {
            expect(v_copy1->get(i, j) == alpha[i] * v.get(i, j)) << "entries should be correctly scaled";
            expect(v_copy2->get(i, j) == alpha[0] * v.get(i, j)) << "entries should be correctly scaled";
        }
    }

    // check scal with indices
    // empty indices
    v_copy1 = v.copy();
    v_copy2 = v.copy();
    expect(fatal(nothrow(
        [&]()
        {
            v_copy1->scal(std::vector<F>{}, Indices{});
            v_copy2->scal(size > 0 ? alpha[0] : F(42), Indices{});
        })));
    expect(exactly_equal(*v_copy1, v));
    expect(exactly_equal(*v_copy2, v));
    // non-empty indices
    for (auto indices_set :
         {std::set<ssize_t>({0, std::min(static_cast<ssize_t>(2), size - 1)}), std::set<ssize_t>({0}),
          std::set<ssize_t>{-1, 0, std::min(static_cast<ssize_t>(1), size - 1)}})
    {
        // reset vectorarrays
        v_copy1 = v.copy();
        v_copy2 = v.copy();
        // scal
        alpha = std::vector<F>(indices_set.size(), F(42));
        const auto indices = Indices(indices_set);
        // Although we use a set and already restricted the indices to be in the range [-size, size),
        // indices can still be non-unique if a negative index corresponds to an existing positive one.
        const bool indices_unique = std::ranges::none_of(indices_set,
                                                         [&indices_set, size](ssize_t i)
                                                         {
                                                             return i < 0 && indices_set.contains(i + size);
                                                         });
        // For a size 0 vector array, scaling with a non-empty set of indices does not make sense.
        // Independent of the size, non-unique indices are not allowed.
        if (size == 0 || !indices_unique)
        {
            expect(fatal(throws(
                [&]()
                {
                    v_copy1->scal(alpha, indices);
                    v_copy2->scal(F(42), indices);
                })));
        }
        else
        {
            expect(fatal(nothrow(
                [&]()
                {
                    v_copy1->scal(alpha, indices);
                    v_copy2->scal(alpha[0], indices);
                })));
            v_copies = std::vector({v_copy1, v_copy2});
            for (const auto& v_copy : v_copies)
            {
                expect(v_copy->size() == size) << "size should be unchanged";
                expect(v_copy->dim() == dim) << "dim should be unchanged";
                for (ssize_t i = 0; i < size; ++i)
                {
                    const bool is_in_indices = indices_set.contains(i) || indices_set.contains(i - size);
                    for (ssize_t j = 0; j < dim; ++j)
                    {
                        expect(approx_equal(v_copy->get(i, j), (is_in_indices ? F(42) : F(1.)) * v.get(i, j)))
                            << "entries should be correctly scaled";
                    }
                }
            }
        }
    }
}

template <class VectorArray>
void check_axpy(const VectorArrayInterface<typename VectorArray::ScalarType>& v, ssize_t size, ssize_t dim)
{
    using F = typename VectorArray::ScalarType;
    // check axpy without indices
    auto v_copy1 = v.copy();
    auto v_copy2 = v.copy();
    auto x_ptr = TestVectorArrayFactory<VectorArray>::iota(size, dim, F(-10));
    const auto& x = *x_ptr;
    std::vector<F> alpha(size, F(42));
    expect(fatal(nothrow(
        [&]()
        {
            v_copy1->axpy(alpha, x);
            v_copy2->axpy(size > 0 ? alpha[0] : F(42), x);
        })));
    auto v_copies = std::vector({v_copy1, v_copy2});
    for (const auto& v_copy : v_copies)
    {
        expect(v_copy->size() == size) << "size should be unchanged";
        expect(v_copy->dim() == dim) << "dim should be unchanged";
    }
    for (ssize_t i = 0; i < size; ++i)
    {
        for (ssize_t j = 0; j < dim; ++j)
        {
            expect(v_copy1->get(i, j) == v.get(i, j) + (alpha[i] * x.get(i, j)))
                << "result of axpy should be correct";
            expect(v_copy2->get(i, j) == v.get(i, j) + (alpha[0] * x.get(i, j)))
                << "result of axpy should be correct";
        }
    }

    // check axpy with indices
    // empty indices
    v_copy1 = v.copy();
    v_copy2 = v.copy();
    expect(fatal(nothrow(
        [&]()
        {
            v_copy1->axpy(alpha, x, Indices{}, Indices{});
            v_copy2->axpy(size > 0 ? alpha[0] : F(42), x, Indices{}, Indices{});
        })));
    expect(exactly_equal(*v_copy1, v));
    expect(exactly_equal(*v_copy2, v));

    // non-empty indices
    for (auto indices_set :
         {std::set<ssize_t>({0, std::min(static_cast<ssize_t>(2), size - 1)}), std::set<ssize_t>({0}),
          std::set<ssize_t>{-1, 0, std::min(static_cast<ssize_t>(1), size - 1)}})
    {
        for (const auto& x_indices_set :
             {std::set<ssize_t>({0, std::min(static_cast<ssize_t>(2), size - 1)}), std::set<ssize_t>({0}),
              std::set<ssize_t>{-1, 0, std::min(static_cast<ssize_t>(1), size - 1)}})
        {
            // reset vectorarrays
            v_copy1 = v.copy();
            v_copy2 = v.copy();
            const auto same_size = indices_set.size() == x_indices_set.size();
            auto x_copy = same_size
                              ? x.copy()
                              : TestVectorArrayFactory<VectorArray>::iota(indices_set.size(), dim, F(-10));
            // axpy
            alpha = std::vector<F>(indices_set.size(), F(42));
            const auto indices = Indices(indices_set);
            const auto x_indices = same_size ? std::optional<Indices>(Indices(x_indices_set)) : std::nullopt;
            // Although we use a set and already restricted the indices to be in the range [-size, size),
            // indices can still be non-unique if a negative index corresponds to an existing positive one.
            const bool indices_unique =
                std::ranges::none_of(indices_set,
                                     [&indices_set, size](ssize_t i)
                                     {
                                         return i < 0 && indices_set.contains(i + size);
                                     });
            // For a size 0 vector array, axpy with a non-empty set of indices does not make sense.
            // Independent of the size, non-unique indices are not allowed.
            if (size == 0 || !indices_unique)
            {
                expect(fatal(throws(
                    [&]()
                    {
                        v_copy1->axpy(alpha, *x_copy, indices, x_indices);
                        v_copy2->axpy(F(42), *x_copy, indices, x_indices);
                    })));
            }
            else
            {
                expect(fatal(nothrow(
                    [&]()
                    {
                        v_copy1->axpy(alpha, *x_copy, indices, x_indices);
                        v_copy2->axpy(alpha[0], *x_copy, indices, x_indices);
                    })));
                v_copies = std::vector({v_copy1, v_copy2});
                for (const auto& v_copy : v_copies)
                {
                    expect(v_copy->size() == size) << "size should be unchanged";
                    expect(v_copy->dim() == dim) << "dim should be unchanged";
                    ssize_t alpha_index = 0;
                    for (auto x_index_it = x_indices_set.begin(); auto v_index : indices_set)
                    {
                        if (v_index < 0)
                        {
                            v_index += size;
                        }
                        auto x_index = same_size ? *x_index_it : alpha_index;
                        if (x_index < 0)
                        {
                            x_index += size;
                        }
                        for (ssize_t j = 0; j < dim; ++j)
                        {
                            expect(approx_equal(v_copy->get(v_index, j),
                                                v.get(v_index, j) + (F(42) * x.get(x_index, j))))
                                << "entries should have the correct entries";
                        }
                        if (same_size)
                        {
                            ++x_index_it;
                        }
                        ++alpha_index;
                    }
                    // check indices that are not in the indices set
                    for (ssize_t i = 0; i < size; ++i)
                    {
                        // skip indices that are in the index set
                        if (indices_set.contains(i) || indices_set.contains(i - size))
                        {
                            continue;
                        }
                        for (ssize_t j = 0; j < dim; ++j)
                        {
                            expect(approx_equal(v_copy->get(i, j), v.get(i, j)))
                                << "entries should be correctly scaled";
                        }
                    }
                }
            }
        }
    }
}

}  // namespace

#endif  // NIAS_CPP_TEST_VECTORARRAY_COMMON_H
