#ifndef NIAS_TEST_MODULE_H
#define NIAS_TEST_MODULE_H

#include <complex>
#include <vector>

#include <nias_cpp/vector.h>

template <class F>
class ExampleVector : public nias::VectorInterface<F>
{
   public:
    using Iterator = typename std::vector<F>::iterator;
    using ConstIterator = typename std::vector<F>::const_iterator;

    // constructors
    ExampleVector() = default;

    ExampleVector(size_t dim, F value = 0.)
        : data_(dim, value) {};

    ExampleVector(std::initializer_list<F> init_list)
        : data_(init_list) {};

    ExampleVector(const ExampleVector& other)
        : data_(other.data_) {
            // std::cout << "Example Vector copy constructor" << std::endl;
        };

    ExampleVector(ExampleVector&& other)
        : data_(std::move(other.data_)) {
            // std::cout << "Example Vector move constructor" << std::endl;
        };

    ~ExampleVector() override
    {
        // std::cout << "ExampleVector destructor for " << this << std::endl;
    }

    // ExampleVector copy and move assignment operators
    ExampleVector& operator=(const ExampleVector& other)
    {
        // std::cout << "Example Vector copy assignment" << std::endl;
        data_ = other.data_;
        return *this;
    }

    ExampleVector& operator=(ExampleVector&& other)
    {
        // std::cout << "Example Vector move assignment" << std::endl;
        data_ = std::move(other.data_);
        return *this;
    }

    // ExampleVector iterators
    Iterator begin()
    {
        return data_.begin();
    }

    Iterator end()
    {
        return data_.end();
    }

    ConstIterator begin() const
    {
        return data_.begin();
    }

    ConstIterator end() const
    {
        return data_.end();
    }

    // ExampleVector accessors
    F& get(size_t i) override
    {
        return data_[i];
    }

    const F& get(size_t i) const override
    {
        return data_[i];
    }

    // ExampleVector methods
    size_t dim() const override
    {
        return data_.size();
    }

    std::shared_ptr<nias::VectorInterface<F>> copy() const override
    {
        // std::cout << "Copy called!" << std::endl;
        return std::make_shared<ExampleVector>(*this);
    }

    F dot(const nias::VectorInterface<F>& other) const override
    {
        // We cannot use
        // return std::inner_product(begin(), end(), other.begin(), 0.0);
        // because VectorInterface does not define iterators
        F ret = 0;
        for (size_t i = 0; i < dim(); ++i)
        {
            if constexpr (std::is_floating_point_v<F>)
            {
                ret += data_[i] * other.get(i);
            }
            else
            {
                ret += data_[i] * std::conj(other.get(i));
            }
        }
        return ret;
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
        for (size_t i = 0; i < dim(); ++i)
        {
            data_[i] += alpha * x.get(i);
        }
    }

   private:
    std::vector<F> data_;
};


#endif  // NIAS_TEST_MODULE_H
