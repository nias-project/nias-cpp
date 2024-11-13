#ifndef NIAS_CPP_VECTORARRAY_NUMPY_H
#define NIAS_CPP_VECTORARRAY_NUMPY_H

#include <concepts>
#include <memory>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <typeinfo>
#include <vector>

#include <nias_cpp/indices.h>
#include <nias_cpp/interfaces/vector.h>
#include <nias_cpp/interfaces/vectorarray.h>
#include <nias_cpp/type_traits.h>
#include <pybind11/numpy.h>

namespace nias
{


template <std::floating_point F>
class NumpyVectorArray : public VectorArrayInterface<F>
{
    using ThisType = NumpyVectorArray;
    using VectorInterfaceType = VectorInterface<F>;
    using InterfaceType = VectorArrayInterface<F>;

   public:
    // TODO: We shoudl be able to pass a numpy array from Python without copy
    explicit NumpyVectorArray(const pybind11::array_t<F>& array)
        : array_(array)
    {
        // Check if the array has the correct data type
        if (array.dtype() != pybind11::dtype::of<F>())
        {
            throw std::invalid_argument("NumpyVectorArray: array must have F data type");
        }

        // Check if the array is one-dimensional
        if (array.ndim() != 2)
        {
            throw std::invalid_argument("NumpyVectorArray: array must be two-dimensional");
        }
    }

    bool operator==(const NumpyVectorArray& other) const
    {
        if (dim() != other.dim() || size() != other.size())
        {
            return false;
        }
        for (ssize_t i = 0; i < size(); ++i)
        {
            for (ssize_t j = 0; j < dim(); ++j)
            {
                if (array_.at(i, j) != other.array_.at(i, j))
                {
                    return false;
                }
            }
        }
        return true;
    }

    auto& array()
    {
        return array_;
    }

    const auto& array() const
    {
        return array_;
    }

    [[nodiscard]] ssize_t size() const override
    {
        return array_.shape(0);
    }

    [[nodiscard]] ssize_t dim() const override
    {
        return array_.shape(1);
    }

    bool is_compatible_array(const InterfaceType& other) const override
    {
        return dim() == other.dim();
    }

    F get(ssize_t i, ssize_t j) const override
    {
        check(i >= 0 && i < size(), "index i out of range");
        check(j >= 0 && j < dim(), "index j out of range");
        return array_.at(i, j);
    }

    std::shared_ptr<InterfaceType> copy(const std::optional<Indices>& indices = {}) const override
    {
        if (!indices)
        {
            return std::make_shared<ThisType>(pybind11::array_t<F>(array_.request()));
        }
        pybind11::array_t<F> sub_array({indices->size(this->size()), dim()});
        ssize_t i = 0;  // index for sub_array
        indices->for_each(
            [this, &i, &sub_array](ssize_t j)
            {
                for (ssize_t k = 0; k < dim(); ++k)
                {
                    sub_array.mutable_at(i, k) = array_.at(j, k);
                }
                ++i;
            },
            this->size());
        return std::make_shared<ThisType>(sub_array);
    }

    void append(InterfaceType& other, bool remove_from_other = false,
                const std::optional<Indices>& other_indices = {}) override
    {
        check(is_numpy_vector_array(other), "append is not (yet) implemented if x is not a NumpyVectorArray");
        const ssize_t other_size = other_indices ? other_indices->size(other.size()) : other.size();
        pybind11::array_t<F> new_array({size() + other_size, dim()});
        // copy old data
        for (ssize_t i = 0; i < size(); ++i)
        {
            for (ssize_t j = 0; j < dim(); ++j)
            {
                new_array.mutable_at(i, j) = array_.at(i, j);
            }
        }
        // copy new data
        for (ssize_t i = 0; i < other_size; ++i)
        {
            const ssize_t other_index = other_indices ? other_indices->get(i, other.size()) : i;
            for (ssize_t j = 0; j < dim(); ++j)
            {
                new_array.mutable_at(size() + i, j) =
                    dynamic_cast<const ThisType&>(other).array_.at(other_index, j);
            }
        }
        array_ = new_array;
        if (remove_from_other)
        {
            auto& old_array_other = dynamic_cast<ThisType&>(other).array_;
            if (!other_indices)
            {
                old_array_other = pybind11::array_t<F>(std::vector<ssize_t> {0, dim()});
            }
            else
            {
                // deduplicate other_indices
                const auto other_indices_vec = other_indices->as_vec(other.size());
                std::set<ssize_t> unique_indices(other_indices_vec.begin(), other_indices_vec.end());
                // now remove entries corresponding to unique_indices from other
                check(unique_indices.size() <= other.size(), "unique_indices contains invalid indices");
                const ssize_t new_size = other.size() - std::ssize(unique_indices);
                pybind11::array_t<F> new_array_other(std::vector<ssize_t> {new_size, dim()});
                ssize_t j = 0;  // index for new_array_other
                for (ssize_t i = 0; i < other.size(); ++i)
                {
                    if (!unique_indices.contains(i))
                    {
                        check(j < new_size, "j is out of bounds");
                        for (ssize_t k = 0; k < dim(); ++k)
                        {
                            new_array_other.mutable_at(j, k) = old_array_other.at(i, k);
                        }
                        ++j;
                    }
                }
                old_array_other = new_array_other;
            }
        }
    }

    void delete_vectors(const std::optional<Indices>& indices) override
    {
        // We first sort and deduplicate indices by converting to a std::set
        const auto indices_vec = indices->as_vec(this->size());
        std::set<ssize_t> unique_indices(indices_vec.begin(), indices_vec.end());
        if (unique_indices.size() > size())
        {
            throw std::invalid_argument("Indices contain invalid indices");
        }
        const ssize_t new_size = size() - unique_indices.size();
        std::vector<ssize_t> indices_to_keep;
        for (ssize_t i = 0; i < size(); ++i)
        {
            if (!unique_indices.contains(i))
            {
                indices_to_keep.push_back(i);
            }
        }
        auto new_array = pybind11::array_t<F>({new_size, dim()});
        for (const auto& i : indices_to_keep)
        {
            for (ssize_t j = 0; j < dim(); ++j)
            {
                new_array.mutable_at(indices_to_keep[i], j) = array_.at(i, j);
            }
        }
        array_ = new_array;
    }

    void scal(F alpha, const std::optional<Indices>& indices = {}) override
    {
        if (!indices)
        {
            for (ssize_t i = 0; i < size(); ++i)
            {
                for (ssize_t j = 0; j < dim(); ++j)
                {
                    array_.mutable_at(i, j) *= alpha;
                }
            }
        }
        else
        {
            // scaling the same vector multiple times is most likely not intended,
            // so we require uniqueness for indices
            this->check_indices_unique(*indices);
            indices->for_each(
                [this, alpha](ssize_t i)
                {
                    for (ssize_t j = 0; j < dim(); ++j)
                    {
                        array_.mutable_at(i, j) *= alpha;
                    }
                },
                size());
        }
    }

    void scal(const std::vector<F>& alpha, const std::optional<Indices>& indices = {}) override
    {
        const auto this_size = indices ? indices->size(size()) : size();
        check(alpha.size() == this_size || alpha.size() == 1,
              "alpha must be scalar or have the same length as the array.");
        if (indices)
        {
            // scaling the same vector multiple times is most likely not intended
            // so we require uniqueness for indices
            this->check_indices_unique(*indices);
        }
        for (ssize_t i = 0; i < this_size; ++i)
        {
            const auto index = indices ? indices->get(i, size()) : i;
            const auto alpha_index = alpha.size() == 1 ? 0 : i;
            for (ssize_t j = 0; j < dim(); ++j)
            {
                array_.mutable_at(index, j) *= alpha[alpha_index];
            }
        }
    }

    void axpy(const std::vector<F>& alpha, const InterfaceType& x, const std::optional<Indices>& indices = {},
              const std::optional<Indices>& x_indices = {}) override
    {
        check(this->is_compatible_array(x), "incompatible dimensions.");
        if (indices)
        {
            // We do not want to write to the same index multiple times, so we require uniqueness for indices.
            // Note that we do not require uniqueness for x_indices, it is okay to use
            // the same vector (read-only) multiple times on the right-hand side.
            this->check_indices_unique(*indices);
        }
        const auto this_size = indices ? indices->size(size()) : size();
        const auto x_size = x_indices ? x_indices->size(x.size()) : x.size();
        check(x_size == this_size || x_size == 1, "x must have length 1 or the same length as this");
        if (this_size == 0)
        {
            return;
        }
        check(alpha.size() == this_size || alpha.size() == 1,
              "alpha must be scalar or have the same length as x");
        check(is_numpy_vector_array(x), "axpy is not implemented if x is not a NumpyVectorArray");
        for (ssize_t i = 0; i < this_size; ++i)
        {
            const auto this_index = indices ? indices->get(i, size()) : i;
            // x can either have the same length as this or length 1
            ssize_t x_index = 0;
            if (x_size == this_size)
            {
                x_index = x_indices ? x_indices->get(i, x.size()) : i;
            }
            else
            {
                // x has length 1
                x_index = x_indices ? x_indices->get(0, x.size()) : 0;
            }
            const auto alpha_index = alpha.size() == 1 ? 0 : i;
            for (ssize_t j = 0; j < dim(); ++j)
            {
                array_.mutable_at(this_index, j) +=
                    alpha[alpha_index] * dynamic_cast<const ThisType&>(x).array_.at(x_index, j);
            }
        }
    }

    void axpy(F alpha, const InterfaceType& x, const std::optional<Indices>& indices = {},
              const std::optional<Indices>& x_indices = {})
    {
        axpy(std::vector<F> {alpha}, x, indices, x_indices);
    }

   private:
    bool is_numpy_vector_array(const InterfaceType& other) const
    {
        try
        {
            dynamic_cast<const ThisType&>(other);
            return true;
        }
        catch (std::bad_cast&)
        {
            return false;
        }
    }

    void check(const bool condition, const std::string& message) const
    {
        if (!condition)
        {
            throw std::invalid_argument("NumpyVectorArray: " + message);
        }
    }

    pybind11::array_t<F> array_;
};


}  // namespace nias

#endif  // NIAS_CPP_VECTORARRAY_NUMPY_H
