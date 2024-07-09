#ifndef NIAS_TEST_MODULE_H
#define NIAS_TEST_MODULE_H

#include <vector>

#include <nias_cpp/vector.h>

class ExampleVector : public nias::VectorInterface<double>
{
   public:
    using Iterator = typename std::vector<double>::iterator;
    using ConstIterator = typename std::vector<double>::const_iterator;

    // constructors
    ExampleVector() = default;
    ExampleVector(size_t dim, double value = 0.);
    ExampleVector(std::initializer_list<double> init_list);
    ExampleVector(const ExampleVector& other);
    ExampleVector(ExampleVector&& other);
    ~ExampleVector() override;

    // copy and move assignment operators
    ExampleVector& operator=(const ExampleVector& other);
    ExampleVector& operator=(ExampleVector&& other);

    // iterators
    Iterator begin();
    Iterator end();
    ConstIterator begin() const;
    ConstIterator end() const;

    // accessors
    double& operator[](size_t i) override;
    const double& operator[](size_t i) const override;

    // methods
    size_t dim() const override;
    std::shared_ptr<nias::VectorInterface<double>> copy() const override;
    double dot(const nias::VectorInterface<double>& other) const override;
    void scal(double alpha) override;
    void axpy(double alpha, const nias::VectorInterface<double>& x) override;

   private:
    std::vector<double> data_;
};


#endif  // NIAS_TEST_MODULE_H
