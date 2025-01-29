#ifndef NIAS_CPP_VECTORARRAY_NUMPY_H
#define NIAS_CPP_VECTORARRAY_NUMPY_H

#include <concepts>
#include <memory>
#include <optional>
#include <set>
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
    // TODO: We should be able to pass a numpy array from Python without copy
    explicit NumpyVectorArray(const pybind11::array_t<F>& array)
        : array_(array)
    {
        // Check if the array has the correct data type
        if (array.dtype() != pybind11::dtype::of<F>())
        {
            throw InvalidArgumentError("NumpyVectorArray: array must have F as data type");
        }

        // Check if the array is one-dimensional
        if (array.ndim() != 2)
        {
            throw InvalidArgumentError("NumpyVectorArray: array must be two-dimensional");
        }
    }

    explicit NumpyVectorArray(ssize_t size, ssize_t dim)
        : array_({size, dim})
    {
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
        this->check_indices(i, j);
        return array_.at(i, j);
    }

    void set(ssize_t i, ssize_t j, F value) override
    {
        this->check_indices(i, j);
        array_.mutable_at(i, j) = value;
    }

    std::shared_ptr<InterfaceType> copy(const std::optional<Indices>& indices = std::nullopt) const override
    {
        if (!indices)
        {
            return std::make_shared<ThisType>(pybind11::array_t<F>(array_.request()));
        }
        indices->check_valid(this->size());
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
                const std::optional<Indices>& other_indices = std::nullopt) override
    {
        check(is_numpy_vector_array(other), "append is not (yet) implemented if x is not a NumpyVectorArray");
        if (other_indices)
        {
            other_indices->check_valid(other.size());
        }
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
                old_array_other = pybind11::array_t<F>(std::vector<ssize_t>{0, dim()});
            }
            else
            {
                // deduplicate other_indices
                const auto other_indices_vec = other_indices->as_vec(other.size());
                std::set<ssize_t> unique_indices(other_indices_vec.begin(), other_indices_vec.end());
                // now remove entries corresponding to unique_indices from other
                check(unique_indices.size() <= other.size(), "unique_indices contains invalid indices");
                const ssize_t new_size = other.size() - std::ssize(unique_indices);
                pybind11::array_t<F> new_array_other(std::vector<ssize_t>{new_size, dim()});
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
            throw InvalidArgumentError("Indices contain invalid indices");
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

    void scal(const std::vector<F>& alpha, const std::optional<Indices>& indices = std::nullopt) override
    {
        if (!indices)
        {
            check(alpha.size() == 1 || alpha.size() == this->size(),
                  "alpha must have size 1 or the same size as the array.");
            for (ssize_t i = 0; i < size(); ++i)
            {
                const auto alpha_index = alpha.size() == 1 ? 0 : i;
                for (ssize_t j = 0; j < dim(); ++j)
                {
                    array_.mutable_at(i, j) *= alpha[alpha_index];
                }
            }
        }
        else
        {
            indices->check_valid(this->size());
            check(alpha.size() == 1 || alpha.size() == indices->size(this->size()),
                  "alpha must have size 1 or the same size as indices");
            ssize_t alpha_index = 0;
            indices->for_each(
                [this, &alpha, &alpha_index](ssize_t i)
                {
                    for (ssize_t j = 0; j < dim(); ++j)
                    {
                        array_.mutable_at(i, j) *= alpha[alpha_index];
                    }
                    if (alpha.size() > 1)
                    {
                        ++alpha_index;
                    }
                },
                this->size());
        }
    }

    void axpy(const std::vector<F>& alpha, const InterfaceType& x,
              const std::optional<Indices>& indices = std::nullopt,
              const std::optional<Indices>& x_indices = std::nullopt) override
    {
        check(this->is_compatible_array(x), "incompatible dimensions.");
        if (indices)
        {
            indices->check_valid(size());
            check(size() == 0 || indices->size(size()) > 0, "indices must not be empty");
        }
        if (x_indices)
        {
            x_indices->check_valid(x.size());
        }
        const auto this_size = indices ? indices->size(size()) : size();
        const auto x_size = x_indices ? x_indices->size(x.size()) : x.size();
        check(x_size == this_size || x_size == 1, "x must have length 1 or the same length as this");
        check(alpha.size() == this_size || alpha.size() == 1,
              "alpha must be scalar or have the same length as this");
        for (ssize_t i = 0; i < this_size; ++i)
        {
            const auto this_index = indices ? indices->get(i, size()) : i;
            // x and alpha can either have the same length as this or length 1
            ssize_t x_index = x_size == 1 ? i : 0;
            x_index = x_indices ? x_indices->get(x_index, x.size()) : x_index;
            const auto alpha_index = alpha.size() == 1 ? 0 : i;
            for (ssize_t j = 0; j < dim(); ++j)
            {
                array_.mutable_at(this_index, j) += alpha[alpha_index] * x.get(x_index, j);
            }
        }
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
            throw InvalidArgumentError("NumpyVectorArray: " + message);
        }
    }

    pybind11::array_t<F> array_;
};


}  // namespace nias

#endif  // NIAS_CPP_VECTORARRAY_NUMPY_H
