#ifndef NIAS_CPP_VECTORARRAY_LIST_H
#define NIAS_CPP_VECTORARRAY_LIST_H

#include <algorithm>
#include <functional>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <typeinfo>
#include <vector>

#include <nias_cpp/concepts.h>
#include <nias_cpp/exceptions.h>
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
    // Create an empty ListVectorArray with the given dimension
    explicit ListVectorArray(ssize_t dim)
        : dim_(dim)
    {
    }

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

    F get(ssize_t i, ssize_t j) const override
    {
        this->check_indices(i, j);
        return vectors_[i]->get(j);
    }

    void set(ssize_t i, ssize_t j, F value) override
    {
        this->check_indices(i, j);
        vectors_[i]->get(j) = value;
    }

    const VectorInterfaceType& get(ssize_t i) const
    {
        this->check_first_index(i);
        return *vectors_[i];
    }

    const std::vector<std::shared_ptr<VectorInterfaceType>>& vectors() const
    {
        return vectors_;
    }

    std::shared_ptr<InterfaceType> copy(const std::optional<Indices>& indices = std::nullopt) const override
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
            indices->check_valid(this->size());
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
                const std::optional<Indices>& other_indices = std::nullopt) override
    {
        check(is_list_vector_array(other), "append is not (yet) implemented if x is not a ListVectorArray");
        remove_from_other ? append_with_removal(dynamic_cast<ThisType&>(other), other_indices)
                          : append_without_removal(dynamic_cast<ThisType&>(other), other_indices);
    }

    // TODO: Think about append signatures
    void append(const std::shared_ptr<VectorInterfaceType>& new_vector)
    {
        vectors_.push_back(new_vector->copy());
    }

    void append(const std::vector<std::shared_ptr<VectorInterfaceType>>& new_vectors)
    {
        vectors_.reserve(vectors_.size() + new_vectors.size());
        for (const auto& vec : new_vectors)
        {
            vectors_.push_back(vec->copy());
        }
    }

    void delete_vectors(const std::optional<Indices>& indices) override
    {
        if (!indices)
        {
            vectors_.clear();
            return;
        }
        indices->check_valid(this->size());
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

    using InterfaceType::axpy;
    using InterfaceType::scal;

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
            throw InvalidArgumentError("ListVectorArray: " + message);
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

    void append_without_removal(const ThisType& other,
                                const std::optional<Indices>& other_indices = std::nullopt)
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
            other_indices->check_valid(other.size());
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
            other_indices->check_valid(other.size());
            vectors_.reserve(vectors_.size() + other_indices->size(other.size()));
            // copy selected entries of other to the end of this
            other_indices->for_each(
                [this, &other](ssize_t i)
                {
                    // we cannot move here because there might be duplicated indices
                    vectors_.push_back(other.vectors_[i]->copy());
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
