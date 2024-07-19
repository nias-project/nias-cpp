#ifndef NIAS_CPP_VECTOR_H
#define NIAS_CPP_VECTOR_H

#include <algorithm>
#include <complex>
#include <concepts>
#include <cstddef>
// #include <iostream>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include <pybind11/complex.h>
#include <pybind11/numpy.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace nias
{

template <class F>
concept floating_point_or_complex =
    std::floating_point<F> || std::is_same_v<F, std::complex<typename F::value_type>>;

template <floating_point_or_complex F>
class VectorInterface
{
   public:
    virtual ~VectorInterface() {
        // std::cout << "VectorInterface destructor for " << this << std::endl;
    };

    // accessors
    virtual F& get(size_t i) = 0;
    virtual const F& get(size_t i) const = 0;

    // return the dimension (length) of the vector
    virtual size_t dim() const = 0;

    // copy the Vector to a new Vector
    virtual std::shared_ptr<VectorInterface> copy() const = 0;

    // dot product
    virtual F dot(const VectorInterface& x) const = 0;

    // scale with a scalar
    virtual void scal(F alpha) = 0;

    // axpy
    virtual void axpy(F alpha, const VectorInterface& x) = 0;
};

template <floating_point_or_complex F>
class VectorArrayInterface
{
    using ThisType = VectorArrayInterface;

   public:
    // return the number of vectors in the array
    virtual size_t size() const = 0;

    // return the dimension (length) of the vectors in the array
    virtual size_t dim() const = 0;

    virtual bool is_compatible_array(const ThisType& other) const
    {
        return dim() == other.dim();
    }

    // copy (a subset of) the VectorArray to a new VectorArray
    virtual std::shared_ptr<ThisType> copy(const std::vector<size_t>& indices = {}) const = 0;

    virtual void append(ThisType& other, bool remove_from_other = false,
                        const std::vector<size_t>& other_indices = {}) = 0;

    virtual void scal(const std::vector<F>& alpha, const std::vector<size_t>& indices = {}) = 0;

    virtual void scal(F alpha, const std::vector<size_t>& indices = {})
    {
        scal(std::vector<F> {alpha}, indices);
    }

    virtual void axpy(const std::vector<F>& alpha, const ThisType& x, const std::vector<size_t>& indices = {},
                      const std::vector<size_t>& x_indices = {}) = 0;

    virtual void axpy(F alpha, const ThisType& x, const std::vector<size_t>& indices = {},
                      const std::vector<size_t>& x_indices = {})
    {
        axpy(std::vector<F> {alpha}, x, indices, x_indices);
    }

    virtual F get(size_t i, size_t j) const = 0;

    virtual void delete_vectors(const std::vector<size_t>& indices) = 0;
    // virtual const std::vector<std::shared_ptr<VectorInterface<F>>>& vectors() const = 0;
};

template <floating_point_or_complex F>
class ListVectorArray : public VectorArrayInterface<F>
{
    using ThisType = ListVectorArray;
    using VectorInterfaceType = VectorInterface<F>;
    using InterfaceType = VectorArrayInterface<F>;

   public:
    ListVectorArray(const std::vector<std::shared_ptr<VectorInterfaceType>>& vectors, size_t dim)
        : vectors_()
        , dim_(dim)
    {
        // std::cout << "VectorArray constructor" << std::endl;
        vectors_.reserve(vectors.size());
        for (const auto& vector : vectors)
        {
            vectors_.push_back(vector->copy());
        }
        check_vec_dimensions();
    }

    ListVectorArray(std::vector<std::shared_ptr<VectorInterfaceType>>&& vectors, size_t dim)
        : vectors_(std::move(vectors))
        , dim_(dim)
    {
        // std::cout << "VectorArray with vectors move constructor" << std::endl;
        check_vec_dimensions();
    }

    ~ListVectorArray()
    {
        // std::cout << "ListVectorArray destructor for " << this << std::endl;
    }

    ListVectorArray(const ListVectorArray& other) = delete;
    ListVectorArray(ListVectorArray&& other) = delete;
    ListVectorArray& operator=(const ListVectorArray& other) = delete;
    ListVectorArray& operator=(ListVectorArray&& other) = delete;

    size_t size() const override
    {
        return vectors_.size();
    }

    size_t dim() const override
    {
        return dim_;
    }

    bool is_compatible_array(const InterfaceType& other) const override
    {
        return dim() == other.dim();
    }

    F get(size_t i, size_t j) const
    {
        return vectors_[i]->get(j);
    }

    const VectorInterfaceType& get(size_t i) const
    {
        return *vectors_[i];
    }

    const std::vector<std::shared_ptr<VectorInterfaceType>>& vectors() const
    {
        return vectors_;
    }

    std::shared_ptr<InterfaceType> copy(const std::vector<size_t>& indices = {}) const override
    {
        // std::cout << "Copy called in VecArray!" << std::endl;
        std::vector<std::shared_ptr<VectorInterfaceType>> copied_vectors;
        if (indices.empty())
        {
            copied_vectors.reserve(vectors_.size());
            std::ranges::transform(vectors_, std::back_inserter(copied_vectors),
                                   [](const auto& vec)
                                   {
                                       return vec->copy();
                                   });
        }
        else
        {
            copied_vectors.reserve(indices.size());
            std::ranges::transform(indices, std::back_inserter(copied_vectors),
                                   [this](size_t i)
                                   {
                                       return vectors_[i]->copy();
                                   });
        }
        return std::make_shared<ThisType>(std::move(copied_vectors), dim_);
    }

    // void append(BaseType& other, bool remove_from_other = false,
    void append(InterfaceType& other, bool remove_from_other = false,
                const std::vector<size_t>& other_indices = {}) override
    {
        check(is_list_vector_array(other), "append is not (yet) implemented if x is not a ListVectorArray");
        const auto num_vecs_to_add = other_indices.empty() ? other.size() : other_indices.size();
        vectors_.reserve(vectors_.size() + num_vecs_to_add);
        remove_from_other ? append_with_removal(dynamic_cast<ThisType&>(other), other_indices)
                          : append_without_removal(dynamic_cast<ThisType&>(other), other_indices);
    }

    void delete_vectors(const std::vector<size_t>& indices) override
    {
        // We first sort and deduplicate indices by converting to a std::set
        // We sort in reverse order (by using std::greater as second template argument) to avoid
        // invalidating indices when removing elements from the vector
        std::set<size_t, std::greater<size_t>> sorted_indices(indices.begin(), indices.end());
        for (auto&& i : sorted_indices)
        {
            vectors_.erase(vectors_.begin() + i);
        }
    }

    void scal(F alpha, const std::vector<size_t>& indices = {}) override
    {
        if (indices.empty())
        {
            for (auto& vector : vectors_)
            {
                vector->scal(alpha);
            }
        }
        else
        {
            for (auto i : indices)
            {
                vectors_[i]->scal(alpha);
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
            vectors_[index]->scal(alpha[alpha_index]);
        }
    }

    // void axpy(const std::vector<F>& alpha, const BaseType& x, const std::vector<size_t>& indices = {},
    void axpy(const std::vector<F>& alpha, const InterfaceType& x, const std::vector<size_t>& indices = {},
              const std::vector<size_t>& x_indices = {}) override
    {
        check(this->is_compatible_array(x), "incompatible dimensions.");
        const auto this_size = indices.empty() ? size() : indices.size();
        const auto x_size = x_indices.empty() ? x.size() : x_indices.size();
        check(x_size == this_size || x_size == 1, "x must have length 1 or the same length as this");
        check(alpha.size() == this_size || alpha.size() == 1,
              "alpha must be scalar or have the same length as x");
        check(is_list_vector_array(x), "axpy is not implemented if x is not a ListVectorArray");

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
            vectors_[this_index]->axpy(alpha[alpha_index],
                                       *dynamic_cast<const ThisType&>(x).vectors_[x_index]);
        }
    }

    void axpy(F alpha, const InterfaceType& x, const std::vector<size_t>& indices = {},
              const std::vector<size_t>& x_indices = {})
    {
        axpy(std::vector<F> {alpha}, x, indices, x_indices);
    }

   private:
    bool is_list_vector_array(const InterfaceType& other) const
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
            throw std::invalid_argument("ListVectorArray: " + message);
        }
    }

    void check_vec_dimensions() const
    {
        check(std::ranges::all_of(vectors_,
                                  [dim = dim_](const auto& vector)
                                  {
                                      return vector->dim() == dim;
                                  }),
              "All vectors must have the same length.");
    }

    void append_without_removal(const ThisType& other, std::vector<size_t> other_indices = {})
    {
        if (other_indices.empty())
        {
            std::ranges::transform(other.vectors_, std::back_inserter(vectors_),
                                   [](const auto& vec)
                                   {
                                       return vec->copy();
                                   });
        }
        else
        {
            std::ranges::transform(other_indices, std::back_inserter(vectors_),
                                   [&other](size_t i)
                                   {
                                       return other.vectors_[i]->copy();
                                   });
        }
    }

    void append_with_removal(ThisType& other, const std::vector<size_t>& other_indices)
    {
        if (other_indices.empty())
        {
            // move all vectors from other to this and then clear other
            vectors_.insert(vectors_.end(), std::make_move_iterator(other.vectors_.begin()),
                            std::make_move_iterator(other.vectors_.end()));
            other.vectors_.clear();
        }
        else
        {
            // move selected entries of other to the end of this
            std::ranges::transform(other_indices, std::back_inserter(vectors_),
                                   [&other](size_t i)
                                   {
                                       return std::move(other.vectors_[i]);
                                   });
            other.delete_vectors(other_indices);
        }
    }

    std::vector<std::shared_ptr<VectorInterfaceType>> vectors_;
    size_t dim_;
};

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

#endif  // NIAS_CPP_VECTOR_H
