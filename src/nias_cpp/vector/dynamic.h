#ifndef NIAS_TEST_MODULE_H
#define NIAS_TEST_MODULE_H

#include <initializer_list>
#include <iostream>
#include <memory>
#include <vector>

#include <nias_cpp/checked_integer_cast.h>
#include <nias_cpp/concepts.h>
#include <nias_cpp/interfaces/vector.h>
#include <nias_cpp/type_traits.h>

namespace nias
{


/*
 * \brief A simple vector based on std::vector
 *
 * \tparam F The type of the vector elements
 * \tparam debug_printing If true, prints messages to std::cout whenever the vector is constructed, destructed,
 * copied or assigned. Useful for detecting unnecessary copies in the python bindings.
 */
template <floating_point_or_complex F, bool debug_printing = false>
class DynamicVector : public nias::VectorInterface<F>
{
   public:
    // constructors
    DynamicVector()
    {
        if constexpr (debug_printing)
        {
            std::cout << "DynamicVector default constructor" << '\n';
        }
    }

    explicit DynamicVector(ssize_t dim, F value = 0.)
        : data_(nias::as_size_t(dim), value)
    {
        if constexpr (debug_printing)
        {
            std::cout << std::format("DynamicVector(dim={}, value={}) constructor", dim, value) << '\n';
        }
    };

    DynamicVector(std::initializer_list<F> init_list)
        : data_(init_list)
    {
        if constexpr (debug_printing)
        {
            std::cout << "DynamicVector initializer_list constructor" << '\n';
        };
    }

    // destructor
    ~DynamicVector() override
    {
        if constexpr (debug_printing)
        {
            std::cout << std::format("DynamicVector destructor for {}", this) << '\n';
        }
    }

    // copy and move constructors
    DynamicVector(const DynamicVector& other)
        : data_(other.data_)
    {
        if constexpr (debug_printing)
        {
            std::cout << "DynamicVector copy constructor" << '\n';
        }
    };

    DynamicVector(DynamicVector&& other) noexcept
        : data_(std::move(other.data_))
    {
        if constexpr (debug_printing)
        {
            std::cout << "DynamicVector move constructor" << '\n';
        }
    };

    // copy and move assignment operators
    DynamicVector& operator=(const DynamicVector& other)
    {
        if constexpr (debug_printing)
        {
            std::cout << "DynamicVector copy assignment" << '\n';
        }
        data_ = other.data_;
        return *this;
    }

    DynamicVector& operator=(DynamicVector&& other) noexcept
    {
        if constexpr (debug_printing)
        {
            std::cout << "DynamicVector move assignment" << '\n';
        }
        data_ = std::move(other.data_);
        return *this;
    }

    // iterators
    [[nodiscard]] auto begin()
    {
        return data_.begin();
    }

    [[nodiscard]] auto end()
    {
        return data_.end();
    }

    [[nodiscard]] auto begin() const
    {
        return data_.begin();
    }

    [[nodiscard]] auto end() const
    {
        return data_.end();
    }

    // element access
    [[nodiscard]] F& get(ssize_t i) override
    {
        return data_.at(nias::as_size_t(i));
    }

    [[nodiscard]] const F& get(ssize_t i) const override
    {
        return data_.at(nias::as_size_t(i));
    }

    // interface methods
    [[nodiscard]] ssize_t dim() const override
    {
        return std::ssize(data_);
    }

    [[nodiscard]] std::shared_ptr<nias::VectorInterface<F>> copy() const override
    {
        if constexpr (debug_printing)
        {
            std::cout << "DynamicVector copy() called" << '\n';
        }
        return std::make_shared<DynamicVector>(*this);
    }

   private:
    std::vector<F> data_;
};


}  // namespace nias


#endif  // NIAS_TEST_MODULE_H
