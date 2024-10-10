#ifndef NIAS_CPP_VECTORARRAY_LIST_H
#define NIAS_CPP_VECTORARRAY_LIST_H

#include <algorithm>
#include <functional>
#include <memory>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <typeinfo>
#include <vector>

#include <nias_cpp/concepts.h>
#include <nias_cpp/indices.h>
#include <nias_cpp/interfaces/vector.h>
#include <nias_cpp/interfaces/vectorarray.h>
#include <nias_cpp/type_traits.h>

namespace nias
{


template <floating_point_or_complex F>
class ListVectorArray : public VectorArrayInterface<F>
{
    using ThisType = ListVectorArray;
    using VectorInterfaceType = VectorInterface<F>;
    using InterfaceType = VectorArrayInterface<F>;

   public:
    // TODO: Add default constructor and append method to add vectors (not vectorarrays)

    ListVectorArray(const std::vector<std::shared_ptr<VectorInterfaceType>>& vectors, ssize_t dim)
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

    ListVectorArray(std::vector<std::shared_ptr<VectorInterfaceType>>&& vectors, ssize_t dim)
        : vectors_(std::move(vectors))
        , dim_(dim)
    {
        // std::cout << "VectorArray with vectors move constructor" << std::endl;
        check_vec_dimensions();
    }

    // ~ListVectorArray()
    //      std::cout << "ListVectorArray destructor for " << this << std::endl;
    // }

    ~ListVectorArray() override = default;

    ListVectorArray(const ListVectorArray& other) = delete;
    ListVectorArray(ListVectorArray&& other) = delete;
    ListVectorArray& operator=(const ListVectorArray& other) = delete;
    ListVectorArray& operator=(ListVectorArray&& other) = delete;

    [[nodiscard]] ssize_t size() const override
    {
        return vectors_.size();
    }

    [[nodiscard]] ssize_t dim() const override
    {
        return dim_;
    }

    bool is_compatible_array(const InterfaceType& other) const override
    {
        return dim() == other.dim();
    }

    F get(ssize_t i, ssize_t j) const
    {
        if (i < 0 || i >= size())
        {
            throw std::out_of_range("ListVectorArray: index i out of range");
        }
        if (j < 0 || j >= dim())
        {
            throw std::out_of_range("ListVectorArray: index j out of range");
        }
        return vectors_[i]->get(j);
    }

    const VectorInterfaceType& get(ssize_t i) const
    {
        if (i < 0 || i >= size())
        {
            throw std::out_of_range("ListVectorArray: index i out of range");
        }
        return *vectors_[i];
    }

    const std::vector<std::shared_ptr<VectorInterfaceType>>& vectors() const
    {
        return vectors_;
    }

    std::shared_ptr<InterfaceType> copy(const std::optional<Indices>& indices = {}) const override
    {
        // std::cout << "Copy called in VecArray!" << std::endl;
        std::vector<std::shared_ptr<VectorInterfaceType>> copied_vectors;
        if (!indices)
        {
            copied_vectors.reserve(this->size());
            std::ranges::transform(vectors_, std::back_inserter(copied_vectors),
                                   [](const auto& vec)
                                   {
                                       return vec->copy();
                                   });
        }
        else
        {
            copied_vectors.reserve(indices->size(this->size()));
            indices->for_each(
                [this, &copied_vectors](ssize_t i)
                {
                    copied_vectors.push_back(vectors_[i]->copy());
                },
                this->size());
        }
        return std::make_shared<ThisType>(std::move(copied_vectors), dim_);
    }

    void append(InterfaceType& other, bool remove_from_other = false,
                const std::optional<Indices>& other_indices = {}) override
    {
        check(is_list_vector_array(other), "append is not (yet) implemented if x is not a ListVectorArray");
        remove_from_other ? append_with_removal(dynamic_cast<ThisType&>(other), other_indices)
                          : append_without_removal(dynamic_cast<ThisType&>(other), other_indices);
    }

    void delete_vectors(const std::optional<Indices>& indices) override
    {
        if (!indices)
        {
            vectors_.clear();
            return;
        }
        // We first sort and deduplicate indices by converting to a std::set
        // We sort in reverse order (by using std::greater as second template argument) to avoid
        // invalidating indices when removing elements from the vector
        const auto indices_vec = indices->as_vec(this->size());
        std::set<ssize_t, std::greater<>> sorted_indices(indices_vec.begin(), indices_vec.end());
        for (auto&& i : sorted_indices)
        {
            vectors_.erase(vectors_.begin() + i);
        }
    }

    void scal(F alpha, const std::optional<Indices>& indices = {}) override
    {
        if (!indices)
        {
            for (auto& vector : vectors_)
            {
                vector->scal(alpha);
            }
        }
        else
        {
            indices->for_each(
                [this, alpha](ssize_t i)
                {
                    vectors_[i]->scal(alpha);
                },
                this->size());
        }
    }

    void scal(const std::vector<F>& alpha, const std::optional<Indices>& indices = {}) override
    {
        if (!indices)
        {
            check(alpha.size() == this->size() || alpha.size() == 1,
                  "alpha must be scalar or have the same length as the array.");
            for (ssize_t i = 0; i < this->size(); ++i)
            {
                const auto alpha_index = alpha.size() == 1 ? 0 : i;
                vectors_[i]->scal(alpha_index);
            }
        }
        else
        {
            check(alpha.size() == indices->size(this->size()) || alpha.size() == 1,
                  "alpha must be scalar or have the same length as the array.");
            indices->for_each(
                [this, &alpha](ssize_t i)
                {
                    const auto alpha_index = alpha.size() == 1 ? 0 : i;
                    vectors_[i]->scal(alpha[alpha_index]);
                },
                this->size());
        }
    }

    void axpy(const std::vector<F>& alpha, const InterfaceType& x, const std::optional<Indices>& indices = {},
              const std::optional<Indices>& x_indices = {}) override
    {
        check(is_list_vector_array(x), "axpy is not implemented if x is not a ListVectorArray");
        const auto this_size = indices ? indices->size(this->size()) : this->size();
        const auto x_size = x_indices ? x_indices->size(x.size()) : x.size();
        check(x_size == this_size || x_size == 1, "x must have length 1 or the same length as this");
        check(alpha.size() == this_size || alpha.size() == 1,
              "alpha must be scalar or have the same length as x");
        for (ssize_t i = 0; i < this_size; ++i)
        {
            const auto this_index = indices ? indices->get(i, this->size()) : i;
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
            vectors_[this_index]->axpy(alpha[alpha_index],
                                       *dynamic_cast<const ThisType&>(x).vectors_[x_index]);
        }
    }

    void axpy(F alpha, const InterfaceType& x, const std::optional<Indices>& indices = {},
              const std::optional<Indices>& x_indices = {}) override
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

    void append_without_removal(const ThisType& other, const std::optional<Indices>& other_indices = {})
    {
        if (!other_indices)
        {
            std::ranges::transform(other.vectors_, std::back_inserter(vectors_),
                                   [](const auto& vec)
                                   {
                                       return vec->copy();
                                   });
        }
        else
        {
            vectors_.reserve(vectors_.size() + other_indices->size(other.size()));
            other_indices->for_each(
                [this, &other](ssize_t i)
                {
                    vectors_.push_back(other.vectors_[i]->copy());
                },
                other.size());
        }
    }

    void append_with_removal(ThisType& other, const std::optional<Indices>& other_indices)
    {
        if (!other_indices)
        {
            // move all vectors from other to this and then clear other
            vectors_.insert(vectors_.end(), std::make_move_iterator(other.vectors_.begin()),
                            std::make_move_iterator(other.vectors_.end()));
            other.vectors_.clear();
        }
        else
        {
            vectors_.reserve(vectors_.size() + other_indices->size(other.size()));
            // move selected entries of other to the end of this
            other_indices->for_each(
                [this, &other](ssize_t i)
                {
                    vectors_.push_back(std::move(other.vectors_[i]));
                },
                other.size());
            other.delete_vectors(other_indices);
        }
    }

    std::vector<std::shared_ptr<VectorInterfaceType>> vectors_;
    ssize_t dim_;
};


}  // namespace nias

#endif  // NIAS_CPP_VECTORARRAY_LIST_H
