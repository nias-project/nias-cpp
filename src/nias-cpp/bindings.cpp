#include <nias-cpp/bindings.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

PYBIND11_MODULE(nias_cpp, m)
{
    m.doc() = "nias-cpp bindings";  // optional module docstring
    nias::bind_nias_vectorinterface<double>(m);
    nias::bind_nias_listvectorarray<double>(m);
}
