#ifndef NIAS_TEST_MODULE_H
#define NIAS_TEST_MODULE_H

#include <initializer_list>
#include <memory>
#include <vector>
// #include <iostream>

#include <nias_cpp/interfaces/vector.h>
#include <nias_cpp/type_traits.h>

// Example Vector class for testing
template <class F>
class DynamicVector : public nias::VectorInterface<F>
{
   public:
    using Iterator = typename std::vector<F>::iterator;
    using ConstIterator = typename std::vector<F>::const_iterator;

    // constructors
    DynamicVector() = default;

    explicit DynamicVector(ssize_t dim, F value = 0.)
        : data_(dim, value) {};

    DynamicVector(std::initializer_list<F> init_list)
        : data_(init_list) {};

    // destructor
    ~DynamicVector() override = default;

    // copy and move constructors and assignment operators
    DynamicVector(const DynamicVector& other) = default;
    DynamicVector(DynamicVector&& other) noexcept = default;
    DynamicVector& operator=(const DynamicVector& other) = default;
    DynamicVector& operator=(DynamicVector&& other) noexcept = default;

    // functions which include printing for debugging

    // DynamicVector(const DynamicVector& other)
    //     : data_(other.data_) {
    //         // std::cout << "Example Vector copy constructor" << std::endl;
    //     };
    //
    // DynamicVector(DynamicVector&& other) noexcept
    //     : data_(std::move(other.data_)) {
    //           // std::cout << "Example Vector move constructor" << std::endl;
    //       };
    //
    // ~DynamicVector() override
    // {
    //     // std::cout << "DynamicVector destructor for " << this << std::endl;
    // }
    //
    // // DynamicVector copy and move assignment operators
    // DynamicVector& operator=(const DynamicVector& other)
    // {
    //     // std::cout << "Example Vector copy assignment" << std::endl;
    //     data_ = other.data_;
    //     return *this;
    // }
    //
    // DynamicVector& operator=(DynamicVector&& other) noexcept
    // {
    //     // std::cout << "Example Vector move assignment" << std::endl;
    //     data_ = std::move(other.data_);
    //     return *this;
    // }

    // DynamicVector iterators
    Iterator begin()
    {
        return data_.begin();
    }

    Iterator end()
    {
        return data_.end();
    }

    [[nodiscard]] ConstIterator begin() const
    {
        return data_.begin();
    }

    [[nodiscard]] ConstIterator end() const
    {
        return data_.end();
    }

    // DynamicVector accessors
    F& get(ssize_t i) override
    {
        return data_[i];
    }

    [[nodiscard]] const F& get(ssize_t i) const override
    {
        return data_[i];
    }

    // DynamicVector methods
    [[nodiscard]] ssize_t dim() const override
    {
        return data_.size();
    }

    [[nodiscard]] std::shared_ptr<nias::VectorInterface<F>> copy() const override
    {
        // std::cout << "Copy called!" << std::endl;
        return std::make_shared<DynamicVector>(*this);
    }

    void scal(F alpha) override
    {
        for (auto& x : *this)
        {
            x *= alpha;
        }
    }

    void axpy(F alpha, const nias::VectorInterface<F>& x) override
    {
        // We cannot use
        // std::ranges::transform(*this, x, begin(),
        //                        [alpha](F a, F b)
        //                        {
        //                            return a + alpha * b;
        //                        });
        // because VectorInterface does not define iterators
        for (ssize_t i = 0; i < dim(); ++i)
        {
            data_[i] += alpha * x.get(i);
        }
    }

   private:
    std::vector<F> data_;
};


#endif  // NIAS_TEST_MODULE_H
