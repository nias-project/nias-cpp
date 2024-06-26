#ifndef NIAS_TEST_MODULE_H
#define NIAS_TEST_MODULE_H

#include <vector>

#include <nias-cpp/vector.h>

class ExampleVector
{
   public:
    using Iterator = typename std::vector<double>::iterator;
    using ConstIterator = typename std::vector<double>::const_iterator;

    // constructors
    ExampleVector();
    ExampleVector(size_t dim, double value = 0.);
    ExampleVector(std::initializer_list<double> init_list);
    ExampleVector(const ExampleVector& other);
    ExampleVector(ExampleVector&& other);

    // iterators
    Iterator begin();
    Iterator end();
    ConstIterator begin() const;
    ConstIterator end() const;

    // methods
    size_t size() const;
    ExampleVector copy() const;
    double dot(const ExampleVector& other) const;
    void scal(double alpha);
    void axpy(double alpha, const ExampleVector& x);

   private:
    std::vector<double> data_;
};

// ensure that ExampleVector fulfills the NiasVector concept
static_assert(nias::NiasVector<ExampleVector>);

#endif  // NIAS_TEST_MODULE_H
