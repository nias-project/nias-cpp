#ifndef NIAS_CPP_VECTORARRAY_NUMPY_H
#define NIAS_CPP_VECTORARRAY_NUMPY_H

#include <concepts>
#include <cstddef>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include <nias_cpp/interfaces/vector.h>
#include <nias_cpp/interfaces/vectorarray.h>
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
    NumpyVectorArray(pybind11::array_t<F> array)
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
        for (size_t i = 0; i < size(); ++i)
        {
            for (size_t j = 0; j < dim(); ++j)
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

    size_t size() const override
    {
        return array_.shape(0);
    }

    size_t dim() const override
    {
        return array_.shape(1);
    }

    bool is_compatible_array(const InterfaceType& other) const override
    {
        return dim() == other.dim();
    }

    F get(size_t i, size_t j) const override
    {
        return array_.at(i, j);
    }

    std::shared_ptr<InterfaceType> copy(const std::vector<size_t>& indices = {}) const override
    {
        if (indices.empty())
        {
            return std::make_shared<ThisType>(pybind11::array_t<F>(array_.request()));
        }
        else
        {
            pybind11::array_t<F> sub_array({indices.size(), dim()});
            for (size_t i = 0 /*subarray_index*/; auto j : indices)
            {
                for (size_t k = 0; k < dim(); ++k)
                {
                    sub_array.mutable_at(i, k) = array_.at(j, k);
                }
                ++i;
            }
            return std::make_shared<ThisType>(sub_array);
        }
    }

    void append(InterfaceType& other, bool remove_from_other = false,
                const std::vector<size_t>& other_indices = {}) override
    {
        check(is_numpy_vector_array(other), "append is not (yet) implemented if x is not a NumpyVectorArray");
        const size_t other_size = other_indices.empty() ? other.size() : other_indices.size();
        pybind11::array_t<F> new_array({size() + other_size, dim()});
        // copy old data
        for (size_t i = 0; i < size(); ++i)
        {
            for (size_t j = 0; j < dim(); ++j)
            {
                new_array.mutable_at(i, j) = array_.at(i, j);
            }
        }
        // copy new data
        for (size_t i = 0; i < other_size; ++i)
        {
            const size_t other_index = other_indices.empty() ? i : other_indices[i];
            for (size_t j = 0; j < dim(); ++j)
            {
                new_array.mutable_at(size() + i, j) =
                    dynamic_cast<const ThisType&>(other).array_.at(other_index, j);
            }
        }
        array_ = new_array;
        if (remove_from_other)
        {
            auto& old_array_other = dynamic_cast<ThisType&>(other).array_;
            if (other_indices.empty())
            {
                old_array_other = pybind11::array_t<F>(std::vector<size_t> {0, dim()});
            }
            else
            {
                // deduplicate other_indices
                std::set<size_t> unique_indices(other_indices.begin(), other_indices.end());
                // now remove entries corresponding to unique_indices from other
                check(unique_indices.size() <= other.size(), "unique_indices contains invalid indices");
                const auto new_size = other.size() - unique_indices.size();
                pybind11::array_t<F> new_array_other({new_size, dim()});
                size_t j = 0;  // index for new_array_other
                for (size_t i = 0; i < other.size(); ++i)
                {
                    if (!unique_indices.contains(i))
                    {
                        check(j < new_size, "j is out of bounds");
                        for (size_t k = 0; k < dim(); ++k)
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

    void delete_vectors(const std::vector<size_t>& indices) override
    {
        // We first sort and deduplicate indices by converting to a std::set
        std::set<size_t> unique_indices(indices.begin(), indices.end());
        assert(unique_indices.size() <= size());
        const size_t new_size = size() - unique_indices.size();
        std::vector<size_t> indices_to_keep;
        for (size_t i = 0; i < size(); ++i)
        {
            if (!unique_indices.contains(i))
            {
                indices_to_keep.push_back(i);
            }
        }
        auto new_array = pybind11::array_t<F>({new_size, dim()});
        for (const auto& i : indices_to_keep)
        {
            for (size_t j = 0; j < dim(); ++j)
            {
                new_array.mutable_at(indices_to_keep[i], j) = array_.at(i, j);
            }
        }
        array_ = new_array;
    }

    void scal(F alpha, const std::vector<size_t>& indices = {}) override
    {
        if (indices.empty())
        {
            for (size_t i = 0; i < size(); ++i)
            {
                for (size_t j = 0; j < dim(); ++j)
                {
                    array_.mutable_at(i, j) *= alpha;
                }
            }
        }
        else
        {
            for (auto i : indices)
            {
                for (size_t j = 0; j < dim(); ++j)
                {
                    array_.mutable_at(i, j) *= alpha;
                }
            }
        }
    }

    void scal(const std::vector<F>& alpha, const std::vector<size_t>& indices = {}) override
    {
        const auto this_size = indices.empty() ? size() : indices.size();
        check(alpha.size() == this_size || alpha.size() == 1,
              "alpha must be scalar or have the same length as the array.");
        for (size_t i = 0; i < this_size; ++i)
        {
            const auto index = indices.empty() ? i : indices[i];
            const auto alpha_index = alpha.size() == 1 ? 0 : i;
            for (size_t j = 0; j < dim(); ++j)
            {
                array_.mutable_at(index, j) *= alpha[alpha_index];
            }
        }
    }

    void axpy(const std::vector<F>& alpha, const InterfaceType& x, const std::vector<size_t>& indices = {},
              const std::vector<size_t>& x_indices = {}) override
    {
        check(this->is_compatible_array(x), "incompatible dimensions.");
        const auto this_size = indices.empty() ? size() : indices.size();
        const auto x_size = x_indices.empty() ? x.size() : x_indices.size();
        check(x_size == this_size || x_size == 1, "x must have length 1 or the same length as this");
        check(alpha.size() == this_size || alpha.size() == 1,
              "alpha must be scalar or have the same length as x");
        check(is_numpy_vector_array(x), "axpy is not implemented if x is not a NumpyVectorArray");

        for (size_t i = 0; i < this_size; ++i)
        {
            const auto this_index = indices.empty() ? i : indices[i];
            // x can either have the same length as this or length 1
            size_t x_index;
            if (x_size == this_size)
            {
                x_index = x_indices.empty() ? i : x_indices[i];
            }
            else
            {
                // x has length 1
                x_index = x_indices.empty() ? 0 : x_indices[0];
            }
            const auto alpha_index = alpha.size() == 1 ? 0 : i;
            for (size_t j = 0; j < dim(); ++j)
            {
                array_.mutable_at(this_index, j) +=
                    alpha[alpha_index] * dynamic_cast<const ThisType&>(x).array_.at(x_index, j);
            }
        }
    }

    void axpy(F alpha, const InterfaceType& x, const std::vector<size_t>& indices = {},
              const std::vector<size_t>& x_indices = {})
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
