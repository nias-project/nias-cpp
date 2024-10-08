#ifndef NIAS_CPP_BINDINGS_H
#define NIAS_CPP_BINDINGS_H

#include <concepts>
#include <cstddef>
#include <memory>
#include <string>

#include <nias_cpp/algorithms/gram_schmidt.h>
#include <nias_cpp/interfaces/vector.h>
#include <nias_cpp/interfaces/vectorarray.h>
#include <nias_cpp/vectorarray/list.h>
#include <nias_cpp/vectorarray/numpy.h>
#include <pybind11/detail/common.h>
#include <pybind11/pybind11.h>

namespace nias
{


template <class F>
    requires std::floating_point<F> || std::is_same_v<F, std::complex<typename F::value_type>>
auto bind_nias_vectorinterface(pybind11::module& m, std::string name = "VectorInterface")
{
    namespace py = pybind11;
    using VecInterface = VectorInterface<F>;

    // See https://pybind11.readthedocs.io/en/stable/advanced/classes.html#overriding-virtual-functions-in-python
    class PyVecInterface : public VecInterface
    {
       public:
        /* Inherit the constructors */
        using VecInterface::VecInterface;

        /* Trampolines (need one for each virtual function) */
        size_t dim() const override
        {
            PYBIND11_OVERRIDE_PURE_NAME(size_t,       /* Return type */
                                        VecInterface, /* Parent class */
                                        "__len__",    /* Name of function in Python */
                                        dot,          /* Name of function in C++ */
            );
        }

        std::shared_ptr<VecInterface> copy() const override
        {
            PYBIND11_OVERRIDE_PURE(std::shared_ptr<VecInterface>, /* Return type */
                                   VecInterface,                  /* Parent class */
                                   copy, /* Name of function in C++ (must match Python name) */
            );
        }

        F dot(const VecInterface& x) const override
        {
            PYBIND11_OVERRIDE_PURE(F,            /* Return type */
                                   VecInterface, /* Parent class */
                                   dot,          /* Name of function in C++ (must match Python name) */
                                   x             /* Argument(s) */
            );
        }

        void scal(F alpha) override
        {
            PYBIND11_OVERRIDE_PURE(void,         /* Return type */
                                   VecInterface, /* Parent class */
                                   scal,         /* Name of function in C++ (must match Python name) */
                                   alpha         /* Argument(s) */
            );
        }

        void axpy(F alpha, const VecInterface& x) override
        {
            PYBIND11_OVERRIDE_PURE(void,         /* Return type */
                                   VecInterface, /* Parent class */
                                   axpy,         /* Name of function in C++ (must match Python name) */
                                   alpha, x);
        }

        F& get(size_t i) override
        {
            throw std::invalid_argument("Not implemented yet");
            PYBIND11_OVERRIDE_PURE_NAME(F&,            /* Return type */
                                        VecInterface,  /* Parent class */
                                        "__setitem__", /* Name of function in Python */
                                        get,           /* Name of function in C++ */
                                        i              /* Argument(s) */
            );
        }

        const F& get(size_t i) const override
        {
            throw std::invalid_argument("Not implemented yet");
            PYBIND11_OVERRIDE_PURE_NAME(const F&,      /* Return type */
                                        VecInterface,  /* Parent class */
                                        "__getitem__", /* Name of function in Python */
                                        get,           /* Name of function in C++ */
                                        i              /* Argument(s) */
            );
        }
    };

    auto ret = py::class_<VecInterface, PyVecInterface, std::shared_ptr<VecInterface>>(m, name.c_str())
                   .def(py::init<>())
                   .def("__len__",
                        [](const VecInterface& v)
                        {
                            return v.dim();
                        })
                   .def("copy", &VecInterface::copy)
                   .def("dot", &VecInterface::dot)
                   .def("scal", &VecInterface::scal)
                   .def("axpy", &VecInterface::axpy);
    return ret;
}

template <class F>
    requires std::floating_point<F> || std::is_same_v<F, std::complex<typename F::value_type>>
auto bind_nias_listvectorarray(pybind11::module& m, std::string field_type_name)
{
    namespace py = pybind11;
    using VecArray = ListVectorArray<F>;
    using VecArrayInterface = VectorArrayInterface<F>;

    // // See https://pybind11.readthedocs.io/en/stable/advanced/classes.html#overriding-virtual-functions-in-python
    class PyVecArrayInterface : public VecArrayInterface
    {
       public:
        /* Inherit the constructors */
        using VecArrayInterface::VecArrayInterface;

        /* Trampolines (need one for each virtual function) */
        size_t size() const override
        {
            PYBIND11_OVERRIDE_PURE_NAME(size_t,            /* Return type */
                                        VecArrayInterface, /* Parent class */
                                        "__len__",         /* Name of function in Python */
                                        size,              /* Name of function in C++ */
            );
        }

        size_t dim() const override
        {
            PYBIND11_OVERRIDE_PURE(size_t,            /* Return type */
                                   VecArrayInterface, /* Parent class */
                                   dim,               /* Name of function in C++ */
            );
        }

        bool is_compatible_array(const VecArrayInterface& other) const override
        {
            PYBIND11_OVERRIDE(bool,                /* Return type */
                              VecArrayInterface,   /* Parent class */
                              is_compatible_array, /* Name of function in C++ */
                              other                /* Argument(s) */
            );
        }

        F get(size_t i, size_t j) const override
        {
            PYBIND11_OVERRIDE_PURE(F,                 /* Return type */
                                   VecArrayInterface, /* Parent class */
                                   get,               /* Name of function in C++ */
                                   i, j               /* Argument(s) */
            );
        }

        // copy (a subset of) the VectorArray to a new VectorArray
        std::shared_ptr<VecArrayInterface> copy(const std::vector<size_t>& indices = {}) const
        {
            PYBIND11_OVERRIDE_PURE(std::shared_ptr<VecArrayInterface>, /* Return type */
                                   VecArrayInterface,                  /* Parent class */
                                   copy,   /* Name of function in C++ (must match Python name) */
                                   indices /* Argument(s) */
            );
        }

        void append(VecArrayInterface& other, bool remove_from_other = false,
                    const std::vector<size_t>& other_indices = {}) override
        {
            PYBIND11_OVERRIDE_PURE(void,              /* Return type */
                                   VecArrayInterface, /* Parent class */
                                   append,            /* Name of function in C++ (must match Python name) */
                                   other, remove_from_other, other_indices /* Argument(s) */
            );
        }

        void scal(const std::vector<F>& alpha, const std::vector<size_t>& indices = {}) override
        {
            PYBIND11_OVERRIDE_PURE(void,              /* Return type */
                                   VecArrayInterface, /* Parent class */
                                   scal,              /* Name of function in C++ (must match Python name) */
                                   alpha, indices     /* Argument(s) */
            );
        }

        virtual void scal(F alpha, const std::vector<size_t>& indices = {})
        {
            PYBIND11_OVERRIDE(void,              /* Return type */
                              VecArrayInterface, /* Parent class */
                              scal,              /* Name of function in C++ (must match Python name) */
                              alpha, indices     /* Argument(s) */
            );
        }

        virtual void axpy(const std::vector<F>& alpha, const VecArrayInterface& x,
                          const std::vector<size_t>& indices = {}, const std::vector<size_t>& x_indices = {})
        {
            PYBIND11_OVERRIDE_PURE(void,              /* Return type */
                                   VecArrayInterface, /* Parent class */
                                   axpy,              /* Name of function in C++ (must match Python name) */
                                   alpha, x, indices, x_indices /* Argument(s) */
            );
        }

        void axpy(F alpha, const VecArrayInterface& x, const std::vector<size_t>& indices = {},
                  const std::vector<size_t>& x_indices = {}) override
        {
            PYBIND11_OVERRIDE(void,              /* Return type */
                              VecArrayInterface, /* Parent class */
                              axpy,              /* Name of function in C++ (must match Python name) */
                              alpha, x, indices, x_indices /* Argument(s) */
            );
        }
    };

    py::class_<VecArrayInterface, PyVecArrayInterface, std::shared_ptr<VecArrayInterface>>(
        m, (field_type_name + "VectorArrayInterface").c_str())
        .def("__len__",
             [](const VecArrayInterface& v)
             {
                 return v.size();
             })
        .def_property_readonly("dim", &VecArrayInterface::dim)
        .def("copy", &VecArrayInterface::copy)
        .def("append", &VecArrayInterface::append)
        // .def("delete", &VecArrayInterface::delete)
        .def("scal", py::overload_cast<F, const std::vector<size_t>&>(&VecArrayInterface::scal))
        .def("scal",
             py::overload_cast<const std::vector<F>&, const std::vector<size_t>&>(&VecArrayInterface::scal))
        .def("axpy", py::overload_cast<F, const nias::VectorArrayInterface<F>&, const std::vector<size_t>&,
                                       const std::vector<size_t>&>(&VecArrayInterface::axpy))
        .def("axpy", py::overload_cast<const std::vector<F>&, const nias::VectorArrayInterface<F>&,
                                       const std::vector<size_t>&, const std::vector<size_t>&>(
                         &VecArrayInterface::axpy))
        .def("is_compatible_array", &VecArrayInterface::is_compatible_array);

    auto ret =
        py::class_<VecArray, VecArrayInterface, std::shared_ptr<VecArray>>(
            m, (field_type_name + "ListVectorArray").c_str())
            .def("__len__",
                 [](const VecArray& v)
                 {
                     return v.size();
                 })
            .def_property_readonly("dim", &VecArray::dim)
            .def("get", py::overload_cast<size_t>(&VecArray::get, py::const_),
                 py::return_value_policy::reference)
            .def("copy", &VecArray::copy)
            .def("append", &VecArray::append)
            .def("delete", &VecArray::delete_vectors)
            .def("scal", py::overload_cast<F, const std::vector<size_t>&>(&VecArray::scal))
            .def("scal",
                 py::overload_cast<const std::vector<F>&, const std::vector<size_t>&>(&VecArray::scal))
            .def("axpy", py::overload_cast<F, const VecArrayInterface&, const std::vector<size_t>&,
                                           const std::vector<size_t>&>(&VecArray::axpy))
            .def("axpy",
                 py::overload_cast<const std::vector<F>&, const VecArrayInterface&,
                                   const std::vector<size_t>&, const std::vector<size_t>&>(&VecArray::axpy))
            .def("is_compatible_array", &VecArray::is_compatible_array);
    return ret;
}

template <class F>
    requires std::floating_point<F> || std::is_same_v<F, std::complex<typename F::value_type>>
auto bind_cpp_gram_schmidt(pybind11::module& m, std::string field_type_name)
{
    m.def((field_type_name + "_gram_schmidt_cpp").c_str(),
          [](pybind11::array_t<F>& numpy_array)
          {
              NumpyVectorArray<F> vec_array(numpy_array);
              gram_schmidt_cpp(vec_array);
          });
}


}  // namespace nias

#endif  // NIAS_CPP_BINDINGS_H
