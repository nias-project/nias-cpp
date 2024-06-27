#include "test_module.h"

#include <algorithm>
#include <numeric>
#include <vector>

#include <nias-cpp/vector.h>
#include <pybind11/pybind11.h>

// ExampleVector constructors
ExampleVector::ExampleVector(size_t dim, double value)
    : data_(dim, value) {};

ExampleVector::ExampleVector(std::initializer_list<double> init_list)
    : data_(init_list) {};

ExampleVector::ExampleVector(const ExampleVector& other)
    : data_(other.data_) {};

ExampleVector::ExampleVector(ExampleVector&& other)
    : data_(std::move(other.data_)) {};

// ExampleVector copy and move assignment operators
ExampleVector& ExampleVector::operator=(const ExampleVector& other)
{
    data_ = other.data_;
    return *this;
}

ExampleVector& ExampleVector::operator=(ExampleVector&& other)
{
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

// ExampleVector methods
size_t ExampleVector::size() const
{
    return data_.size();
}

ExampleVector ExampleVector::copy() const
{
    return *this;
}

double ExampleVector::dot(const ExampleVector& other) const
{
    return std::inner_product(begin(), end(), other.begin(), 0.0);
}

void ExampleVector::scal(double alpha)
{
    for (auto& x : *this)
    {
        x *= alpha;
    }
}

void ExampleVector::axpy(double alpha, const ExampleVector& x)
{
    std::ranges::transform(*this, x, begin(),
                           [alpha](double a, double b)
                           {
                               return a + alpha * b;
                           });
}

// ExampleVector bindings
PYBIND11_MODULE(nias_cpp_test, m)
{
    m.doc() = "nias-cpp test bindings";  // optional module docstring
    nias::bind_nias_vector<ExampleVector>(m, "ExampleVector");
    nias::bind_nias_vectorarray<ExampleVector>(m, "ExampleVectorArray");
}
