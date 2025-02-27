#ifndef NIAS_CPP_INDICES_H
#define NIAS_CPP_INDICES_H

#include <array>
#include <functional>
#include <initializer_list>
#include <optional>
#include <set>
#include <variant>
#include <vector>

#include <nias_cpp/type_traits.h>
#include <pybind11/pytypes.h>

#include "nias_cpp_export.h"

namespace nias
{


class NIAS_CPP_EXPORT Indices
{
    using ValueType = std::variant<std::vector<ssize_t>, pybind11::slice>;

   public:
    /// The default constructor initializes indices_ to an empty vector
    Indices();

    /// Construct from a single index
    explicit(false) Indices(ssize_t index);

    /// Construct from a vector of indices
    explicit(false) Indices(const std::vector<ssize_t>& indices);

    /// Construct from a set of indices
    explicit(false) Indices(const std::set<ssize_t>& indices);

    /// Construct from a Python slice
    explicit(false) Indices(const pybind11::slice& slice);

    /// Construct from a list of indices
    Indices(std::initializer_list<ssize_t> indices);

    /// Destructor
    ~Indices();

    // copy and move constructor
    Indices(const Indices& other);
    Indices(Indices&& other) noexcept;
    // copy and move assignment operators
    Indices& operator=(const Indices& other);
    Indices& operator=(Indices&& other) noexcept;

    /**
      * \brief Get number of indices for a sequence of given length
      *
      * Returns the number of indices in the Indices object. Since the indices can
      * be a slice, the length of the sequence the indices are applied to is needed.
      */
    [[nodiscard]] ssize_t size(ssize_t length) const;

    /**
      * \brief Get i-th index for a sequence of given length
      *
      * Since the stored indices can be a slice, the i-th index depends on the length
      * of the sequence the indices are applied.
      */
    [[nodiscard]] ssize_t get(ssize_t i, ssize_t length) const;

    /**
      * \brief Check that all indices are valid for a sequence of given length
      *
      * If we hold a list of indices, we check that all indices i fulfill -length <= i <= length-1.
      * If we hold a slice, we do not have to check anything as slices will just be empty if they
      * do not fit the sequence (e.g., for a sequence vec of length 10 in Python, vec[20:30] will just be empty)
      */
    void check_valid(ssize_t length) const;

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
    [[nodiscard]] std::vector<ssize_t> as_vec(ssize_t length) const;

    [[nodiscard]] std::set<ssize_t> unique_indices(ssize_t length) const;

   private:
    // compute start, stop, step, and slicelength for a slice
    [[nodiscard]] std::array<ssize_t, 4> compute(ssize_t length) const;

    // a valid index i for a sequence of length n in Python fulfills  -n <= i <= n-1
    // we cannot check this in the constructor because the length of the sequence is not known at that point,
    // so we have to check it here. Since we want a list of valid C++ indices, we also have to convert negative indices to positive ones.
    [[nodiscard]] static ssize_t positive_index(ssize_t index, ssize_t length);

    // check if indices_ holds a vector (and not a slice)
    [[nodiscard]] bool holds_vector() const;

    // get the stored vector (check with holds_vector() first)
    [[nodiscard]] const std::vector<ssize_t>& stored_vector() const;

    // See https://stackoverflow.com/questions/4145605/stdvector-needs-to-have-dll-interface-to-be-used-by-clients-of-class-xt-war on why this is a pointer
    ValueType* indices_;
};

// Convenience class to not have to write Indices(pybind11::slice(start, stop, step)) when creating a slice
// TODO: Drop this class?
class NIAS_CPP_EXPORT Slice : public Indices
{
   public:
    Slice(std::optional<ssize_t> start, std::optional<ssize_t> stop,
          std::optional<ssize_t> step = std::nullopt)
        : Indices(pybind11::slice(start, stop, step))
    {
    }
};


}  // namespace nias

#endif  // NIAS_CPP_INDICES_H
