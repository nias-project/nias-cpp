#ifndef NIAS_CPP_BINDINGS_H
#define NIAS_CPP_BINDINGS_H

#include <concepts>
#include <memory>

#include <nias-cpp/vector.h>
#include <pybind11/detail/common.h>
#include <pybind11/pybind11.h>

namespace nias
{


template <std::floating_point F>
auto bind_nias_vectorinterface(pybind11::module& m)
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

        F& operator[](size_t i) override
        {
            throw std::invalid_argument("Not implemented yet");
            PYBIND11_OVERRIDE_PURE_NAME(F&,            /* Return type */
                                        VecInterface,  /* Parent class */
                                        "__setitem__", /* Name of function in Python */
                                        operator[],    /* Name of function in C++ */
                                        i              /* Argument(s) */
            );
        }

        const F& operator[](size_t i) const override
        {
            throw std::invalid_argument("Not implemented yet");
            PYBIND11_OVERRIDE_PURE_NAME(const F&,      /* Return type */
                                        VecInterface,  /* Parent class */
                                        "__getitem__", /* Name of function in Python */
                                        operator[],    /* Name of function in C++ */
                                        i              /* Argument(s) */
            );
        }
    };

    auto ret = py::class_<VecInterface, PyVecInterface, std::shared_ptr<VecInterface>>(m, "VectorInterface")
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

template <std::floating_point F>
auto bind_nias_listvectorarray(pybind11::module& m)
{
    namespace py = pybind11;
    using VecArray = ListVectorArray<F>;
    // using VecArrayInterface = VectorArrayInterface<F>;

    // // See https://pybind11.readthedocs.io/en/stable/advanced/classes.html#overriding-virtual-functions-in-python
    // class PyVecArrayInterface : public VecArrayInterface
    // {
    //    public:
    //     /* Inherit the constructors */
    //     using VecArrayInterface::VecArrayInterface;
    //
    //     /* Trampolines (need one for each virtual function) */
    //     size_t size() const override
    //     {
    //         PYBIND11_OVERRIDE_PURE_NAME(size_t,            /* Return type */
    //                                     VecArrayInterface, /* Parent class */
    //                                     "__len__",         /* Name of function in Python */
    //                                     size,              /* Name of function in C++ */
    //         );
    //     }
    //
    //     size_t dim() const override
    //     {
    //         PYBIND11_OVERRIDE_PURE(size_t,            /* Return type */
    //                                VecArrayInterface, /* Parent class */
    //                                dim,               /* Name of function in C++ */
    //         );
    //     }
    //
    //     bool is_compatible_array(const VecArrayInterface& other) const override
    //     {
    //         PYBIND11_OVERRIDE(bool,                /* Return type */
    //                           VecArrayInterface,   /* Parent class */
    //                           is_compatible_array, /* Name of function in C++ */
    //                           other                /* Argument(s) */
    //         );
    //     }
    //
    //     // copy (a subset of) the VectorArray to a new VectorArray
    //     std::shared_ptr<VecArrayInterface> copy(const std::vector<size_t>& indices = {}) const
    //     {
    //         PYBIND11_OVERRIDE_PURE(std::shared_ptr<VecArrayInterface>, /* Return type */
    //                                VecArrayInterface,                  /* Parent class */
    //                                copy,   /* Name of function in C++ (must match Python name) */
    //                                indices /* Argument(s) */
    //         );
    //     }
    //
    //     void append(VecArrayInterface& other, bool remove_from_other = false,
    //                 const std::vector<size_t>& other_indices = {}) override
    //     {
    //         PYBIND11_OVERRIDE_PURE(void,              /* Return type */
    //                                VecArrayInterface, /* Parent class */
    //                                append,            /* Name of function in C++ (must match Python name) */
    //                                other, remove_from_other, other_indices /* Argument(s) */
    //         );
    //     }
    //
    //     void scal(const std::vector<F>& alpha, const std::vector<size_t>& indices = {}) override
    //     {
    //         PYBIND11_OVERRIDE_PURE(void,              /* Return type */
    //                                VecArrayInterface, /* Parent class */
    //                                scal,              /* Name of function in C++ (must match Python name) */
    //                                alpha, indices     /* Argument(s) */
    //         );
    //     }
    //
    //     virtual void scal(F alpha, const std::vector<size_t>& indices = {})
    //     {
    //         PYBIND11_OVERRIDE(void,              /* Return type */
    //                           VecArrayInterface, /* Parent class */
    //                           scal,              /* Name of function in C++ (must match Python name) */
    //                           alpha, indices     /* Argument(s) */
    //         );
    //     }
    //
    //     virtual void axpy(const std::vector<F>& alpha, const VecArrayInterface& x,
    //                       const std::vector<size_t>& indices = {}, const std::vector<size_t>& x_indices = {})
    //     {
    //         PYBIND11_OVERRIDE_PURE(void,              /* Return type */
    //                                VecArrayInterface, /* Parent class */
    //                                axpy,              /* Name of function in C++ (must match Python name) */
    //                                alpha, x, indices, x_indices /* Argument(s) */
    //         );
    //     }
    //
    //     void axpy(F alpha, const VecArrayInterface& x, const std::vector<size_t>& indices = {},
    //               const std::vector<size_t>& x_indices = {}) override
    //     {
    //         PYBIND11_OVERRIDE(void,              /* Return type */
    //                           VecArrayInterface, /* Parent class */
    //                           axpy,              /* Name of function in C++ (must match Python name) */
    //                           alpha, x, indices, x_indices /* Argument(s) */
    //         );
    //     }
    //
    //     const std::vector<std::shared_ptr<VectorInterface<F>>>& vectors() const override
    //     {
    //         PYBIND11_OVERRIDE(const std::vector<std::shared_ptr<VectorInterface<F>>>&, /* Return type */
    //                           VecArrayInterface,                                      /* Parent class */
    //                           vectors,                                                /* Name of function in C++ */
    //         );
    //     }
    // };
    //
    // auto ret = py::class_<VecArrayInterface, PyVecArrayInterface, std::shared_ptr<VecArrayInterface>>(
    //                m, "VectorArrayInterface")
    //                .def("__len__",
    //                     [](const VecArrayInterface& v)
    //                     {
    //                         return v.size();
    //                     })
    //                .def_property_readonly("dim", &VecArrayInterface::dim)
    //                .def_property_readonly("vectors", &VecArrayInterface::vectors)
    //                .def("copy", &VecArrayInterface::copy)
    //                .def("append", &VecArrayInterface::append)
    //                .def("scal", py::overload_cast<F, const std::vector<size_t>&>(&VecArrayInterface::scal))
    //                .def("scal", py::overload_cast<const std::vector<F>&, const std::vector<size_t>&>(
    //                                 &VecArrayInterface::scal))
    //                .def("axpy",
    //                     py::overload_cast<F, const nias::VectorArrayInterface<F>&, const std::vector<size_t>&,
    //                                       const std::vector<size_t>&>(&VecArrayInterface::axpy))
    //                .def("axpy", py::overload_cast<const std::vector<F>&, const nias::VectorArrayInterface<F>&,
    //                                               const std::vector<size_t>&, const std::vector<size_t>&>(
    //                                 &VecArrayInterface::axpy))
    //                .def("is_compatible_array", &VecArrayInterface::is_compatible_array);
    //
    auto ret =
        py::class_<VecArray, std::shared_ptr<VecArray>>(m, "ListVectorArray")
            .def("__len__",
                 [](const VecArray& v)
                 {
                     return v.size();
                 })
            .def_property_readonly("dim", &VecArray::dim)
            // vectors() returns a vector of shared pointers to VectorInterface objects
            // The following compile fine but lead to double free or corruption errors when called from python,
            // independent of the return_value_policy that is used.
            // .def_property_readonly("vectors", &VecArray::vectors)
            // .def("vectors", &VecArray::vectors, py::return_value_policy::copy)
            .def("get", &VecArray::get, py::return_value_policy::reference)
            .def("copy", &VecArray::copy)
            .def("append", &VecArray::append)
            .def("delete", &VecArray::delete_vectors)
            .def("scal", py::overload_cast<F, const std::vector<size_t>&>(&VecArray::scal))
            .def("scal",
                 py::overload_cast<const std::vector<F>&, const std::vector<size_t>&>(&VecArray::scal))
            .def(
                "axpy",
                py::overload_cast<F, const VecArray&, const std::vector<size_t>&, const std::vector<size_t>&>(
                    &VecArray::axpy))
            .def("axpy", py::overload_cast<const std::vector<F>&, const VecArray&, const std::vector<size_t>&,
                                           const std::vector<size_t>&>(&VecArray::axpy))
            .def("is_compatible_array", &VecArray::is_compatible_array);
    return ret;
}


}  // namespace nias

#endif  // NIAS_CPP_BINDINGS_H
