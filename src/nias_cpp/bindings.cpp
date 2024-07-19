#include <complex>

#include <nias_cpp/bindings.h>
#include <pybind11/complex.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

PYBIND11_MODULE(nias_cpp, m)
{
    m.doc() = "nias-cpp bindings";  // optional module docstring
    nias::bind_nias_vectorinterface<float>(m, "FloatVectorInterface");
    nias::bind_nias_vectorinterface<double>(m, "DoubleVectorInterface");
    nias::bind_nias_vectorinterface<long double>(m, "LongDoubleVectorInterface");
    nias::bind_nias_vectorinterface<std::complex<float>>(m, "ComplexFloatVectorInterface");
    nias::bind_nias_vectorinterface<std::complex<double>>(m, "ComplexDoubleVectorInterface");
    nias::bind_nias_vectorinterface<std::complex<long double>>(m, "ComplexLongDoubleVectorInterface");

    nias::bind_nias_listvectorarray<float>(m, "Float");
    nias::bind_nias_listvectorarray<double>(m, "Double");
    nias::bind_nias_listvectorarray<long double>(m, "LongDouble");
    nias::bind_nias_listvectorarray<std::complex<float>>(m, "ComplexFloat");
    nias::bind_nias_listvectorarray<std::complex<double>>(m, "ComplexDouble");
    nias::bind_nias_listvectorarray<std::complex<long double>>(m, "ComplexLongDouble");

    nias::bind_cpp_gram_schmidt<float>(m, "float");
    nias::bind_cpp_gram_schmidt<double>(m, "double");
    nias::bind_cpp_gram_schmidt<long double>(m, "long_double");
}
