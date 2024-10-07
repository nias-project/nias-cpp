#include "indices.h"

#include <initializer_list>
#include <variant>
#include <vector>

#include <pybind11/detail/common.h>
#include <pybind11/pybind11.h>
#include <pybind11/pytypes.h>
#include <sys/types.h>

namespace nias
{


using ssize_t = pybind11::ssize_t;

Indices::Indices(ssize_t index)
    : indices_(std::vector<ssize_t> {index})
{
}

Indices::Indices(const std::vector<ssize_t>& indices)
    : indices_(indices)
{
}

Indices::Indices(const pybind11::slice& slice)
    : indices_(slice)
{
}

Indices::Indices(std::initializer_list<ssize_t>&& indices)
    : indices_(std::vector<ssize_t>(std::move(indices)))
{
}

ssize_t Indices::size(ssize_t length) const
{
    if (std::holds_alternative<std::vector<ssize_t>>(indices_))
    {
        return std::get<std::vector<ssize_t>>(indices_).size();
    }
    else
    {
        ssize_t start, stop, step, slicelength;
        // slice.compute calls PySlice_GetIndicesEx (from the Python C-API) which in turn calls PySlice_Unpack and then PySlice_AdjustIndices, see
        // https://github.com/python/cpython/blob/main/Objects/sliceobject.c
        // length is the length of the sequence which the slice is applied to, and slicelength is the length of the resulting slice (number of indices in the slice)
        // PySlice_AdjustIndices adjust start and stop indices automatically to fit within the bounds of the sequence (depending on the sign of step)
        std::get<pybind11::slice>(indices_).compute(length, &start, &stop, &step, &slicelength);
        return slicelength;
    }
}

ssize_t Indices::get(ssize_t i, ssize_t length) const
{
    if (std::holds_alternative<std::vector<ssize_t>>(indices_))
    {
        return std::get<std::vector<ssize_t>>(indices_)[i];
    }
    else
    {
        const auto [start, stop, step, slicelength] = compute(length);
        if (i < 0 || i >= slicelength)
        {
            throw std::out_of_range("Index out of range");
        }
        return start + i * step;
    }
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
    else
    {
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
}


}  // namespace nias
