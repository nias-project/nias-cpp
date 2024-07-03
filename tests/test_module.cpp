#include "test_module.h"

// #include <algorithm>
// #include <numeric>
#include <vector>

#include <nias-cpp/vector.h>
#include <pybind11/pybind11.h>

// ExampleVector constructors
ExampleVector::ExampleVector(size_t dim, double value)
    : data_(dim, value) {};

ExampleVector::ExampleVector(std::initializer_list<double> init_list)
    : data_(init_list) {};

ExampleVector::ExampleVector(const ExampleVector& other)
    : data_(other.data_) {
        // std::cout << "Example Vector copy constructor" << std::endl;
    };

ExampleVector::ExampleVector(ExampleVector&& other)
    : data_(std::move(other.data_)) {
        // std::cout << "Example Vector move constructor" << std::endl;
    };

ExampleVector::~ExampleVector()
{
    // std::cout << "ExampleVector destructor for " << this << std::endl;
}

// ExampleVector copy and move assignment operators
ExampleVector& ExampleVector::operator=(const ExampleVector& other)
{
    // std::cout << "Example Vector copy assignment" << std::endl;
    data_ = other.data_;
    return *this;
}

ExampleVector& ExampleVector::operator=(ExampleVector&& other)
{
    // std::cout << "Example Vector move assignment" << std::endl;
    data_ = std::move(other.data_);
    return *this;
}

// ExampleVector iterators
ExampleVector::Iterator ExampleVector::begin()
{
    return data_.begin();
}

ExampleVector::Iterator ExampleVector::end()
{
    return data_.end();
}

ExampleVector::ConstIterator ExampleVector::begin() const
{
    return data_.begin();
}

ExampleVector::ConstIterator ExampleVector::end() const
{
    return data_.end();
}

// ExampleVector accessors
double& ExampleVector::operator[](size_t i)
{
    return data_[i];
}

const double& ExampleVector::operator[](size_t i) const
{
    return data_[i];
}

// ExampleVector methods
size_t ExampleVector::dim() const
{
    return data_.size();
}

std::shared_ptr<nias::VectorInterface<double>> ExampleVector::copy() const
{
    // std::cout << "Copy called!" << std::endl;
    return std::make_shared<ExampleVector>(*this);
}

double ExampleVector::dot(const nias::VectorInterface<double>& other) const
{
    // We cannot use
    // return std::inner_product(begin(), end(), other.begin(), 0.0);
    // because VectorInterface does not define iterators
    double ret = 0;
    for (size_t i = 0; i < dim(); ++i)
    {
        ret += (*this)[i] * other[i];
    }
    return ret;
}

void ExampleVector::scal(double alpha)
{
    for (auto& x : *this)
    {
        x *= alpha;
    }
}

void ExampleVector::axpy(double alpha, const nias::VectorInterface<double>& x)
{
    // We cannot use
    // std::ranges::transform(*this, x, begin(),
    //                        [alpha](double a, double b)
    //                        {
    //                            return a + alpha * b;
    //                        });
    // because VectorInterface does not define iterators
    for (size_t i = 0; i < dim(); ++i)
    {
        (*this)[i] += alpha * x[i];
    }
}
