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

    void delete_vectors(const std::vector<size_t>& indices)
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


}  // namespace nias

#endif  // NIAS_CPP_VECTOR_H
