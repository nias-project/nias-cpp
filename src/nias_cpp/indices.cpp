#include "indices.h"

#include <functional>
#include <initializer_list>
#include <set>
#include <stdexcept>
#include <utility>
#include <variant>
#include <vector>

#include <nias_cpp/type_traits.h>
#include <pybind11/pytypes.h>

namespace nias
{


Indices::Indices(ssize_t index)
    : indices_(std::vector<ssize_t> {index})
{
}

Indices::Indices(const std::vector<ssize_t>& indices)
    : indices_(indices)
{
}

Indices::Indices(const std::set<ssize_t>& indices)
    : indices_(std::vector<ssize_t>(indices.begin(), indices.end()))
{
}

Indices::Indices(const pybind11::slice& slice)
    : indices_(slice)
{
}

Indices::Indices(std::initializer_list<ssize_t> indices)
    : indices_(std::vector<ssize_t>(indices))
{
}

ssize_t Indices::size(ssize_t length) const
{
    if (std::holds_alternative<std::vector<ssize_t>>(indices_))
    {
        const auto ret = std::get<std::vector<ssize_t>>(indices_).size();
        if (!std::in_range<ssize_t>(ret))
        {
            throw std::out_of_range("size() result is out of range");
        }
        return static_cast<ssize_t>(ret);
    }
    auto [start, stop, step, slicelength] = compute(length);
    return slicelength;
}

ssize_t Indices::get(ssize_t i, ssize_t length) const
{
    if (std::holds_alternative<std::vector<ssize_t>>(indices_))
    {
        return positive_index(std::get<std::vector<ssize_t>>(indices_)[i], length);
    }
    const auto [start, stop, step, slicelength] = compute(length);
    if (i < 0 || i >= slicelength)
    {
        throw std::out_of_range("Index out of range");
    }
    return start + (i * step);
}

void Indices::for_each(const std::function<void(ssize_t)>& func, ssize_t length) const
{
    if (std::holds_alternative<std::vector<ssize_t>>(indices_))
    {
        for (auto index : std::get<std::vector<ssize_t>>(indices_))
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
    if (std::holds_alternative<std::vector<ssize_t>>(indices_))
    {
        auto indices = std::get<std::vector<ssize_t>>(indices_);
        for (auto& index : indices)
        {
            index = positive_index(index, length);
        }
        return indices;
    }
    const auto [start, stop, step, slicelength] = compute(length);
    std::vector<ssize_t> indices;
    indices.reserve(slicelength);
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

}  // namespace nias
