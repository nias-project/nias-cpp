#ifndef NIAS_CPP_INDICES_H
#define NIAS_CPP_INDICES_H

#include <initializer_list>
#include <variant>
#include <vector>

#include <pybind11/detail/common.h>
#include <pybind11/pybind11.h>
#include <pybind11/pytypes.h>
#include <sys/types.h>

namespace nias
{


class Indices
{
    using ssize_t = pybind11::ssize_t;

   public:
    /// The default constructor initializes indices_ to an empty vector
    Indices() = default;

    /// Construct from a single index
    Indices(ssize_t index);

    /// Construct from a vector of indices
    Indices(const std::vector<ssize_t>& indices);

    /// Construct from a Python slice
    Indices(const pybind11::slice& slice);

    /// Construct from a list of indices
    Indices(std::initializer_list<ssize_t>&& indices);

    /**
      * \brief Get number of indices for sequence of given length
      *
      * Returns the number of indices in the Indices object. Since the indices can
      * be a slice, the length of the sequence the indices are applied to is needed.
      */
    ssize_t size(ssize_t length) const;

    /**
      * \brief Get i-th index for sequence of given length
      *
      * Since the stored indices can be a slice, the i-th index depends on the length
      * of the sequence the indices are applied.
      */
    ssize_t get(ssize_t i, ssize_t length) const;

    /**
      * \brief Apply a function for each index
      */
    void for_each(const std::function<void(ssize_t)>& func, ssize_t length) const;

    /**
      * \brief Get the indices as a vector
      *
      * Takes the length of the sequence the indices are applied to as an argument and
      * returns a vector of indices with 0 <= index < length.
      */
    std::vector<ssize_t> as_vec(ssize_t length) const;

   private:
    // compute start, stop, step, and slicelength for a slice
    std::array<ssize_t, 4> compute(ssize_t length) const
    {
        if (!std::holds_alternative<pybind11::slice>(indices_))
            throw std::runtime_error("compute can only be called if indices_ holds a slice");
        ssize_t start, stop, step, slicelength;
        // slice.compute calls PySlice_GetIndicesEx (from the Python C-API) which in turn calls PySlice_Unpack and then PySlice_AdjustIndices, see
        // https://github.com/python/cpython/blob/main/Objects/sliceobject.c
        // length is the length of the sequence which the slice is applied to, and slicelength is the length of the resulting slice (number of indices in the slice)
        // PySlice_AdjustIndices adjust start and stop indices automatically to fit within the bounds of the sequence (depending on the sign of step)
        std::get<pybind11::slice>(indices_).compute(length, &start, &stop, &step, &slicelength);
        return {start, stop, step, slicelength};
    }

    // a valid index i for a sequence of length n in Python fulfills  -n <= i <= n-1
    // we cannot check this in the constructor because the length of the sequence is not known at that point,
    // so we have to check it here. Since we want a list of valid C++ indices, we also have to convert negative indices to positive ones.
    ssize_t positive_index(ssize_t index, ssize_t length) const
    {
        if (index < 0)
        {
            index += length;
        }
        if (index < 0 || index >= length)
        {
            throw std::out_of_range("Index must be between -length and length - 1");
        }
        return index;
    }

    std::variant<std::vector<ssize_t>, pybind11::slice> indices_;
};


}  // namespace nias

#endif  // NIAS_CPP_INDICES_H
