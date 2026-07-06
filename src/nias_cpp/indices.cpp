#include "indices.h"

#include <array>
#include <functional>
#include <initializer_list>
#include <set>
#include <utility>
#include <variant>
#include <vector>

#include <nias_cpp/checked_integer_cast.h>
#include <nias_cpp/exceptions.h>
#include <nias_cpp/type_traits.h>
#include <pybind11/pytypes.h>

namespace nias
{

// Implementation struct definition
struct Indices::Impl
{
    using ValueType = std::variant<std::vector<ssize_t>, pybind11::slice>;
    ValueType data;

    Impl() = default;

    explicit Impl(ssize_t index)
        : data(std::in_place_type_t<std::vector<ssize_t>>{}, 1, index)
    {
    }

    explicit Impl(const std::vector<ssize_t>& indices)
        : data(indices)
    {
    }

    explicit Impl(const std::set<ssize_t>& indices)
        : data(std::in_place_type_t<std::vector<ssize_t>>{}, indices.begin(), indices.end())
    {
    }

    explicit Impl(const pybind11::slice& slice)
        : data(slice)
    {
    }

    Impl(std::initializer_list<ssize_t> indices)
        : data(std::in_place_type_t<std::vector<ssize_t>>{}, indices)
    {
    }
};

Indices::Indices()
    : pimpl_(new Impl())
{
}

Indices::Indices(ssize_t index)
    : pimpl_(new Impl(index))
{
}

Indices::Indices(const std::vector<ssize_t>& indices)
    : pimpl_(new Impl(indices))
{
}

Indices::Indices(const std::set<ssize_t>& indices)
    : pimpl_(new Impl(indices))
{
}

Indices::Indices(const pybind11::slice& slice)
    : pimpl_(new Impl(slice))
{
}

Indices::Indices(std::initializer_list<ssize_t> indices)
    : pimpl_(new Impl(indices))
{
}

Indices::~Indices()
{
    delete pimpl_;
}

Indices::Indices(const Indices& other)
    : pimpl_((other.pimpl_ != nullptr) ? new Impl(*other.pimpl_) : nullptr)
{
}

Indices::Indices(Indices&& other) noexcept
    : pimpl_(nullptr)
{
    std::swap(pimpl_, other.pimpl_);
}

Indices& Indices::operator=(const Indices& other)
{
    Indices tmp(other);
    std::swap(pimpl_, tmp.pimpl_);
    return *this;
}

Indices& Indices::operator=(Indices&& other) noexcept
{
    Indices tmp(std::move(other));
    std::swap(pimpl_, tmp.pimpl_);
    return *this;
}

ssize_t Indices::size(ssize_t length) const
{
    if (holds_vector())
    {
        return std::ssize(stored_vector());
    }
    auto [start, stop, step, slicelength] = compute(length);
    return slicelength;
}

ssize_t Indices::get(ssize_t i, ssize_t length) const
{
    if (holds_vector())
    {
        return positive_index(stored_vector().at(as_size_t(i)), length);
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
    if (holds_vector())
    {
        for (auto&& index : stored_vector())
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
    if (holds_vector())
    {
        for (auto index : stored_vector())
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
    if (holds_vector())
    {
        auto indices = stored_vector();
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
    if (holds_vector())
    {
        throw InvalidStateError("compute can only be called if pimpl_ holds a slice");
    }
    ssize_t start = 0;
    ssize_t stop = 0;
    ssize_t step = 0;
    ssize_t slicelength = 0;
    // slice.compute calls PySlice_GetIndicesEx (from the Python C-API) which in turn calls PySlice_Unpack and then PySlice_AdjustIndices, see
    // https://github.com/python/cpython/blob/main/Objects/sliceobject.c
    // length is the length of the sequence which the slice is applied to, and slicelength is the length of the resulting slice (number of indices in the slice)
    // PySlice_AdjustIndices adjust start and stop indices automatically to fit within the bounds of the sequence (depending on the sign of step)
    std::get<pybind11::slice>(pimpl_->data).compute(length, &start, &stop, &step, &slicelength);
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

bool Indices::holds_vector() const
{
    return std::holds_alternative<std::vector<ssize_t>>(pimpl_->data);
}

const std::vector<ssize_t>& Indices::stored_vector() const
{
    return std::get<std::vector<ssize_t>>(pimpl_->data);
}

}  // namespace nias
