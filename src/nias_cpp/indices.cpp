#include "indices.h"

#include <array>
#include <functional>
#include <initializer_list>
#include <set>
#include <variant>
#include <vector>

#include <nias_cpp/checked_integer_cast.h>
#include <nias_cpp/exceptions.h>
#include <nias_cpp/type_traits.h>
#include <pybind11/pytypes.h>

namespace nias
{

Indices::Indices()
    : indices_(new ValueType())
{
}

Indices::Indices(ssize_t index)
    : indices_(new ValueType(std::in_place_type_t<std::vector<ssize_t>>{}, 1, index))
{
}

Indices::Indices(const std::vector<ssize_t>& indices)
    : indices_(new ValueType(indices))
{
}

Indices::Indices(const std::set<ssize_t>& indices)
    : indices_(new ValueType(std::in_place_type_t<std::vector<ssize_t>>{}, indices.begin(), indices.end()))
{
}

Indices::Indices(const pybind11::slice& slice)
    : indices_(new ValueType(slice))
{
}

Indices::Indices(std::initializer_list<ssize_t> indices)
    : indices_(new ValueType(std::in_place_type_t<std::vector<ssize_t>>{}, indices))
{
}

Indices::~Indices()
{
    if (indices_)
    {
        delete indices_;
    }
}

Indices::Indices(const Indices& other)
    : indices_(other.indices_ ? new ValueType(*other.indices_) : nullptr)
{
}

Indices::Indices(Indices&& other)
    : indices_(nullptr)
{
    std::swap(indices_, other.indices_);
}

Indices& Indices::operator=(const Indices& other)
{
    Indices tmp(other);
    std::swap(indices_, tmp.indices_);
    return *this;
}

Indices& Indices::operator=(Indices&& other)
{
    Indices tmp(std::move(other));
    std::swap(indices_, tmp.indices_);
    return *this;
}

ssize_t Indices::size(ssize_t length) const
{
    if (std::holds_alternative<std::vector<ssize_t>>(*indices_))
    {
        return std::ssize(std::get<std::vector<ssize_t>>(*indices_));
    }
    auto [start, stop, step, slicelength] = compute(length);
    return slicelength;
}

ssize_t Indices::get(ssize_t i, ssize_t length) const
{
    if (std::holds_alternative<std::vector<ssize_t>>(*indices_))
    {
        return positive_index(std::get<std::vector<ssize_t>>(*indices_)[as_size_t(i)], length);
    }
    const auto [start, stop, step, slicelength] = compute(length);
    if (i < 0 || i >= slicelength)
    {
        throw InvalidIndexError("Index out of range");
    }
    return start + (i * step);
}

void Indices::check_valid(ssize_t length) const
{
    if (std::holds_alternative<std::vector<ssize_t>>(*indices_))
    {
        for (auto&& index : std::get<std::vector<ssize_t>>(*indices_))
        {
            if (index < -length || index >= length)
            {
                throw InvalidIndexError("Index must be between -length and length - 1");
            }
        }
    }
}

void Indices::for_each(const std::function<void(ssize_t)>& func, ssize_t length) const
{
    if (std::holds_alternative<std::vector<ssize_t>>(*indices_))
    {
        for (auto index : std::get<std::vector<ssize_t>>(*indices_))
        {
            func(positive_index(index, length));
        }
    }
    else
    {
        auto [start, stop, step, slicelength] = compute(length);
        if (step > 0)
        {
            for (ssize_t i = start; i < stop; i += step)
            {
                func(i);
            }
        }
        else
        {
            // TODO: check if this is correct
            for (ssize_t i = start; i >= stop; i += step)
            {
                func(i);
            }
        }
    }
}

std::vector<ssize_t> Indices::as_vec(ssize_t length) const
{
    if (std::holds_alternative<std::vector<ssize_t>>(*indices_))
    {
        auto indices = std::get<std::vector<ssize_t>>(*indices_);
        for (auto& index : indices)
        {
            index = positive_index(index, length);
        }
        return indices;
    }
    const auto [start, stop, step, slicelength] = compute(length);
    std::vector<ssize_t> indices;
    indices.reserve(as_size_t(slicelength));
    if (step > 0)
    {
        for (ssize_t i = start; i < stop; i += step)
        {
            indices.push_back(i);
        }
    }
    else
    {
        // TODO: check if this is correct
        for (ssize_t i = start; i >= stop; i += step)
        {
            indices.push_back(i);
        }
    }
    return indices;
}

std::set<ssize_t> Indices::unique_indices(ssize_t length) const
{
    const auto indices_vec = as_vec(length);
    return {indices_vec.begin(), indices_vec.end()};
}

std::array<ssize_t, 4> Indices::compute(ssize_t length) const
{
    if (!std::holds_alternative<pybind11::slice>(*indices_))
    {
        throw InvalidStateError("compute can only be called if indices_ holds a slice");
    }
    ssize_t start = 0;
    ssize_t stop = 0;
    ssize_t step = 0;
    ssize_t slicelength = 0;
    // slice.compute calls PySlice_GetIndicesEx (from the Python C-API) which in turn calls PySlice_Unpack and then PySlice_AdjustIndices, see
    // https://github.com/python/cpython/blob/main/Objects/sliceobject.c
    // length is the length of the sequence which the slice is applied to, and slicelength is the length of the resulting slice (number of indices in the slice)
    // PySlice_AdjustIndices adjust start and stop indices automatically to fit within the bounds of the sequence (depending on the sign of step)
    std::get<pybind11::slice>(*indices_).compute(length, &start, &stop, &step, &slicelength);
    return {start, stop, step, slicelength};
}

ssize_t Indices::positive_index(ssize_t index, ssize_t length)
{
    if (index < 0)
    {
        index += length;
    }
    if (index < 0 || index >= length)
    {
        throw InvalidIndexError("Index must be between -length and length - 1");
    }
    return index;
}

}  // namespace nias
