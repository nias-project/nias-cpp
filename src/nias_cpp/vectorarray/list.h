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

#include <nias_cpp/checked_integer_cast.h>
#include <nias_cpp/concepts.h>
#include <nias_cpp/exceptions.h>
#include <nias_cpp/indices.h>
#include <nias_cpp/interfaces/vector.h>
#include <nias_cpp/interfaces/vectorarray.h>
#include <nias_cpp/type_traits.h>
#include <nias_cpp/vector/wrapper.h>

namespace nias
{


template <class VectorType, floating_point_or_complex F>
class ListVectorArray : public VectorArrayInterface<F>
{
    using ThisType = ListVectorArray;
    using VectorInterfaceType = VectorInterface<F>;
    using InterfaceType = VectorArrayInterface<F>;
    using VectorWrapperType = VectorWrapper<VectorType, F>;
    using VectorTraitsType = VectorTraits<VectorType>;

   public:
    // Create an empty ListVectorArray with the given dimension
    explicit ListVectorArray(ssize_t dim)
        : dim_(dim)
    {
    }

    ListVectorArray(const std::vector<VectorType>& vectors, ssize_t dim)
        : vectors_()
        , dim_(dim)
    {
        // std::cout << "VectorArray constructor" << std::endl;
        vectors_.reserve(vectors.size());
        for (const auto& vector : vectors)
        {
            vectors_.push_back(VectorWrapperType(vector));
        }
        check_vec_dimensions();
    }

    ListVectorArray(const std::vector<VectorWrapperType>& vectors, ssize_t dim)
        : vectors_(vectors)
        , dim_(dim)
    {
        check_vec_dimensions();
    }

    // NOLINTNEXTLINE(cppcoreguidelines-rvalue-reference-param-not-moved)
    ListVectorArray(std::vector<VectorType>&& vectors, ssize_t dim)
        : vectors_()
        , dim_(dim)
    {
        vectors_.reserve(vectors.size());
        for (auto&& vector : vectors)
        {
            vectors_.emplace_back(VectorWrapperType(std::move(vector)));
        }
        check_vec_dimensions();
    }

    ~ListVectorArray() override = default;

    ListVectorArray(const ListVectorArray& other) = delete;
    ListVectorArray(ListVectorArray&& other) = delete;
    ListVectorArray& operator=(const ListVectorArray& other) = delete;
    ListVectorArray& operator=(ListVectorArray&& other) = delete;

    [[nodiscard]] ssize_t size() const override
    {
        return std::ssize(vectors_);
    }

    [[nodiscard]] ssize_t dim() const override
    {
        return dim_;
    }

    [[nodiscard]] bool is_compatible_array(const InterfaceType& other) const override
    {
        return dim() == other.dim();
    }

    [[nodiscard]] const VectorInterface<F>& vector(ssize_t i) const override
    {
        return vectors_.at(as_size_t(i)).backend();
    }

    [[nodiscard]] VectorInterface<F>& vector(ssize_t i) override
    {
        return vectors_.at(as_size_t(i)).backend();
    }

    [[nodiscard]] F get(ssize_t i, ssize_t j) const override
    {
        this->check_indices(i, j);
        return vectors_[as_size_t(i)][j];
    }

    void set(ssize_t i, ssize_t j, F value) override
    {
        this->check_indices(i, j);
        vectors_[as_size_t(i)][j] = value;
    }

    [[nodiscard]] const std::vector<VectorWrapperType>& vectors() const
    {
        return vectors_;
    }

    [[nodiscard]] std::shared_ptr<InterfaceType> copy(
        const std::optional<Indices>& indices = std::nullopt) const override
    {
        // std::cout << "Copy called in VecArray!" << std::endl;
        if (!indices)
        {
            return std::make_shared<ThisType>(vectors_, dim_);
        }
        std::vector<VectorWrapperType> copied_vectors;
        indices->check_valid(this->size());
        copied_vectors.reserve(as_size_t(indices->size(this->size())));
        indices->for_each(
            [this, &copied_vectors](ssize_t i)
            {
                copied_vectors.push_back(vectors_[as_size_t(i)]);
            },
            this->size());
        return std::make_shared<ThisType>(copied_vectors, dim_);
    }

    void append(InterfaceType& other, bool remove_from_other = false,
                const std::optional<Indices>& other_indices = std::nullopt) override
    {
        check(is_list_vector_array(other), "append is not (yet) implemented if x is not a ListVectorArray");
        remove_from_other ? append_with_removal(dynamic_cast<ThisType&>(other), other_indices)
                          : append_without_removal(dynamic_cast<ThisType&>(other), other_indices);
    }

    // TODO: Think about append signatures
    void append(const VectorType& new_vector)
    {
        vectors_.push_back(VectorWrapperType(VectorTraitsType::copy_(new_vector)));
    }

    void append(const std::vector<VectorType>& new_vectors)
    {
        vectors_.reserve(vectors_.size() + new_vectors.size());
        for (const auto& vec : new_vectors)
        {
            vectors_.push_back(VectorWrapperType(VectorTraitsType::copy_(vec)));
        }
    }

    template <typename... Args>
    void emplace_back(Args&&... args)
    {
        vectors_.emplace_back(VectorWrapperType(VectorType(std::forward<Args>(args)...)));
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
        const std::set<ssize_t, std::greater<>> sorted_indices(indices_vec.begin(), indices_vec.end());
        for (auto&& i : sorted_indices)
        {
            vectors_.erase(vectors_.begin() + i);
        }
    }

    using InterfaceType::axpy;
    using InterfaceType::scal;

   private:
    [[nodiscard]] bool is_list_vector_array(const InterfaceType& other) const
    {
        try
        {
            const auto& ret = dynamic_cast<const ThisType&>(other);
            static_cast<void>(ret);
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
                                      return vector.dim() == dim;
                                  }),
              "All vectors must have the same length.");
    }

    void append_without_removal(const ThisType& other,
                                const std::optional<Indices>& other_indices = std::nullopt)
    {
        if (!other_indices)
        {
            vectors_.insert(vectors_.end(), other.vectors_.begin(), other.vectors_.end());
        }
        else
        {
            other_indices->check_valid(other.size());
            vectors_.reserve(vectors_.size() + as_size_t(other_indices->size(other.size())));
            other_indices->for_each(
                [this, &other](ssize_t i)
                {
                    vectors_.push_back(other.vectors_[as_size_t(i)]);
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
            vectors_.reserve(vectors_.size() + as_size_t(other_indices->size(other.size())));
            // copy selected entries of other to the end of this
            other_indices->for_each(
                [this, &other](ssize_t i)
                {
                    // we cannot move here because there might be duplicated indices
                    vectors_.push_back(other.vectors_[as_size_t(i)]);
                },
                other.size());
            other.delete_vectors(other_indices);
        }
    }

    std::vector<VectorWrapperType> vectors_;
    ssize_t dim_;
};


}  // namespace nias

#endif  // NIAS_CPP_VECTORARRAY_LIST_H
