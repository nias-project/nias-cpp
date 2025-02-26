#ifndef NIAS_CPP_BINDINGS_H
#define NIAS_CPP_BINDINGS_H

#include <complex>
#include <concepts>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <nias_cpp/algorithms/gram_schmidt.h>
#include <nias_cpp/indices.h>
#include <nias_cpp/inner_products/euclidean.h>
#include <nias_cpp/inner_products/function_based.h>
#include <nias_cpp/interfaces/inner_products.h>
#include <nias_cpp/interfaces/vector.h>
#include <nias_cpp/interfaces/vectorarray.h>
#include <nias_cpp/type_traits.h>
#include <nias_cpp/vectorarray/list.h>
#include <nias_cpp/vectorarray/numpy.h>
#include <pybind11/detail/common.h>
#include <pybind11/pybind11.h>
#include <pybind11/pytypes.h>

namespace nias
{


template <class F>
    requires std::floating_point<F> || std::is_same_v<F, std::complex<typename F::value_type>>
auto bind_nias_vectorinterface(pybind11::module& m, const std::string& name = "VectorInterface")
{
    namespace py = pybind11;

    // See https://pybind11.readthedocs.io/en/stable/advanced/classes.html#overriding-virtual-functions-in-python
    class PyVecInterface : public VectorInterface<F>
    {
       public:
        using VecInterface = VectorInterface<F>;
        /* Inherit the constructors */
        using VecInterface::VecInterface;

        /* Trampolines (need one for each virtual function) */
        [[nodiscard]] ssize_t dim() const override
        {
            PYBIND11_OVERRIDE_PURE_NAME(ssize_t,      /* Return type */
                                        VecInterface, /* Parent class */
                                        "__len__",    /* Name of function in Python */
                                        dim           /* Name of function in C++ */
            );
        }

        [[nodiscard]] std::shared_ptr<VecInterface> copy() const override
        {
            PYBIND11_OVERRIDE_PURE(std::shared_ptr<VecInterface>, /* Return type */
                                   VecInterface,                  /* Parent class */
                                   copy /* Name of function in C++ (must match Python name) */
            );
        }

        void scal(F alpha) override
        {
            PYBIND11_OVERRIDE(void,         /* Return type */
                              VecInterface, /* Parent class */
                              scal,         /* Name of function in C++ (must match Python name) */
                              alpha         /* Argument(s) */
            );
        }

        void axpy(F alpha, const VecInterface& x) override
        {
            PYBIND11_OVERRIDE(void,         /* Return type */
                              VecInterface, /* Parent class */
                              axpy,         /* Name of function in C++ (must match Python name) */
                              alpha, x);
        }

        F& get(ssize_t i) override
        {
            PYBIND11_OVERRIDE_PURE_NAME(F&,            /* Return type */
                                        VecInterface,  /* Parent class */
                                        "__setitem__", /* Name of function in Python */
                                        get,           /* Name of function in C++ */
                                        i              /* Argument(s) */
            );
        }

        [[nodiscard]] const F& get(ssize_t i) const override
        {
            PYBIND11_OVERRIDE_PURE_NAME(const F&,      /* Return type */
                                        VecInterface,  /* Parent class */
                                        "__getitem__", /* Name of function in Python */
                                        get,           /* Name of function in C++ */
                                        i              /* Argument(s) */
            );
        }
    };

    using VecInterface = VectorInterface<F>;
    auto ret = py::class_<VecInterface, PyVecInterface, std::shared_ptr<VecInterface>>(m, name.c_str())
                   .def(py::init<>())
                   .def("__len__",
                        [](const VecInterface& v)
                        {
                            return v.dim();
                        })
                   .def("copy", &VecInterface::copy)
                   .def("scal", &VecInterface::scal)
                   .def("axpy", &VecInterface::axpy)
                   .def("get", py::overload_cast<ssize_t>(&VecInterface::get, py::const_));
    return ret;
}

template <class F>
    requires std::floating_point<F> || std::is_same_v<F, std::complex<typename F::value_type>>
auto bind_nias_listvectorarray(pybind11::module& m, const std::string& field_type_name)
{
    namespace py = pybind11;

    // See https://pybind11.readthedocs.io/en/stable/advanced/classes.html#overriding-virtual-functions-in-python
    class PyVecArrayInterface : public VectorArrayInterface<F>
    {
       public:
        using VecArrayInterface = VectorArrayInterface<F>;

        /* Inherit the constructors */
        using VecArrayInterface::VecArrayInterface;

        /* Trampolines (need one for each virtual function) */
        [[nodiscard]] ssize_t size() const override
        {
            PYBIND11_OVERRIDE_PURE_NAME(ssize_t,           /* Return type */
                                        VecArrayInterface, /* Parent class */
                                        "__len__",         /* Name of function in Python */
                                        size,              /* Name of function in C++ */
            );
        }

        [[nodiscard]] ssize_t dim() const override
        {
            PYBIND11_OVERRIDE_PURE(ssize_t,           /* Return type */
                                   VecArrayInterface, /* Parent class */
                                   dim,               /* Name of function in C++ */
            );
        }

        [[nodiscard]] bool is_compatible_array(const VecArrayInterface& other) const override
        {
            PYBIND11_OVERRIDE(bool,                /* Return type */
                              VecArrayInterface,   /* Parent class */
                              is_compatible_array, /* Name of function in C++ */
                              other                /* Argument(s) */
            );
        }

        [[nodiscard]] F get(ssize_t i, ssize_t j) const override
        {
            PYBIND11_OVERRIDE_PURE(F,                 /* Return type */
                                   VecArrayInterface, /* Parent class */
                                   get,               /* Name of function in C++ */
                                   i, j               /* Argument(s) */
            );
        }

        // copy (a subset of) the VectorArray to a new VectorArray
        [[nodiscard]] std::shared_ptr<VecArrayInterface> copy(
            const std::optional<Indices>& indices = std::nullopt) const override
        {
            PYBIND11_OVERRIDE_PURE(std::shared_ptr<VecArrayInterface>, /* Return type */
                                   VecArrayInterface,                  /* Parent class */
                                   copy,   /* Name of function in C++ (must match Python name) */
                                   indices /* Argument(s) */
            );
        }

        void append(VecArrayInterface& other, bool remove_from_other = false,
                    const std::optional<Indices>& other_indices = std::nullopt) override
        {
            PYBIND11_OVERRIDE_PURE(void,              /* Return type */
                                   VecArrayInterface, /* Parent class */
                                   append,            /* Name of function in C++ (must match Python name) */
                                   other, remove_from_other, other_indices /* Argument(s) */
            );
        }

        void scal(const std::vector<F>& alpha, const std::optional<Indices>& indices = std::nullopt) override
        {
            PYBIND11_OVERRIDE(void,              /* Return type */
                              VecArrayInterface, /* Parent class */
                              scal,              /* Name of function in C++ (must match Python name) */
                              alpha, indices     /* Argument(s) */
            );
        }

        void scal(F alpha, const std::optional<Indices>& indices = std::nullopt) override
        {
            PYBIND11_OVERRIDE(void,              /* Return type */
                              VecArrayInterface, /* Parent class */
                              scal,              /* Name of function in C++ (must match Python name) */
                              alpha, indices     /* Argument(s) */
            );
        }

        void axpy(const std::vector<F>& alpha, const VecArrayInterface& x,
                  const std::optional<Indices>& indices = std::nullopt,
                  const std::optional<Indices>& x_indices = std::nullopt) override
        {
            PYBIND11_OVERRIDE(void,              /* Return type */
                              VecArrayInterface, /* Parent class */
                              axpy,              /* Name of function in C++ (must match Python name) */
                              alpha, x, indices, x_indices /* Argument(s) */
            );
        }

        void axpy(F alpha, const VecArrayInterface& x, const std::optional<Indices>& indices = std::nullopt,
                  const std::optional<Indices>& x_indices = std::nullopt) override
        {
            PYBIND11_OVERRIDE(void,              /* Return type */
                              VecArrayInterface, /* Parent class */
                              axpy,              /* Name of function in C++ (must match Python name) */
                              alpha, x, indices, x_indices /* Argument(s) */
            );
        }

        void print() const override
        {
            PYBIND11_OVERRIDE(void,              /* Return type */
                              VecArrayInterface, /* Parent class */
                              print              /* Name of function in C++ (must match Python name) */
            );
        }
    };

    using VecArrayInterface = VectorArrayInterface<F>;
    py::class_<VecArrayInterface, PyVecArrayInterface, std::shared_ptr<VecArrayInterface>>(
        m, (field_type_name + "VectorArrayInterface").c_str())
        .def("__len__",
             [](const VecArrayInterface& v)
             {
                 return v.size();
             })
        .def_property_readonly("dim", &VecArrayInterface::dim)
        .def("scalar_zero", &VecArrayInterface::scalar_zero)
        .def("copy", &VecArrayInterface::copy)
        .def("append", &VecArrayInterface::append)
        // .def("delete", &VecArrayInterface::delete)
        .def("scal", py::overload_cast<F, const std::optional<Indices>&>(&VecArrayInterface::scal))
        .def("scal", py::overload_cast<const std::vector<F>&, const std::optional<Indices>&>(
                         &VecArrayInterface::scal))
        .def("axpy", py::overload_cast<F, const nias::VectorArrayInterface<F>&, const std::optional<Indices>&,
                                       const std::optional<Indices>&>(&VecArrayInterface::axpy))
        .def("axpy", py::overload_cast<const std::vector<F>&, const nias::VectorArrayInterface<F>&,
                                       const std::optional<Indices>&, const std::optional<Indices>&>(
                         &VecArrayInterface::axpy))
        .def("is_compatible_array", &VecArrayInterface::is_compatible_array);

    using ListVecArray = ListVectorArray<F>;
    auto ret =
        py::class_<ListVecArray, VecArrayInterface, std::shared_ptr<ListVecArray>>(
            m, (field_type_name + "ListVectorArray").c_str())
            .def("__len__",
                 [](const ListVecArray& v)
                 {
                     return v.size();
                 })
            .def_property_readonly("dim", &ListVecArray::dim)
            .def("get", py::overload_cast<ssize_t>(&ListVecArray::get, py::const_),
                 py::return_value_policy::reference)
            .def("copy", &ListVecArray::copy, py::arg("indices") = py::none())
            .def("append",
                 py::overload_cast<VecArrayInterface&, bool, const std::optional<Indices>&>(
                     &ListVecArray::append),
                 py::arg("other"), py::arg("remove_from_other") = false,
                 py::arg("other_indices") = py::none())
            .def("delete", &ListVecArray::delete_vectors, py::arg("indices"))
            // .def("scal", py::overload_cast<F, const std::optional<Indices>&>(&ListVecArray::scal),
            //      py::arg("alpha"), py::arg("indices") = py::none())
            // .def("scal",
            //      py::overload_cast<const std::vector<F>&, const std::optional<Indices>&>(&ListVecArray::scal),
            //      py::arg("alpha"), py::arg("indices") = py::none())
            // .def("axpy",
            //      py::overload_cast<F, const VecArrayInterface&, const std::optional<Indices>&,
            //                        const std::optional<Indices>&>(&ListVecArray::axpy),
            //      py::arg("alpha"), py::arg("x"), py::arg("indices") = py::none(),
            //      py::arg("x_indices") = py::none())
            // .def("axpy",
            //      py::overload_cast<const std::vector<F>&, const VecArrayInterface&,
            //                        const std::optional<Indices>&, const std::optional<Indices>&>(
            //          &ListVecArray::axpy),
            //      py::arg("alpha"), py::arg("x"), py::arg("indices") = py::none(),
            //      py::arg("x_indices") = py::none())
            .def("is_compatible_array", &ListVecArray::is_compatible_array)
            .def("print", &ListVecArray::print);
    return ret;
}

template <class F>
    requires std::floating_point<F> || std::is_same_v<F, std::complex<typename F::value_type>>
auto bind_cpp_gram_schmidt(pybind11::module& m, const std::string& field_type_name)
{
    m.def((field_type_name + "_gram_schmidt_cpp").c_str(),
          // TODO: Should take a Python Nias Vectorarray
          [](const pybind11::array_t<F>& numpy_array)
          {
              auto numpy_array_copy = pybind11::array_t<F>(numpy_array.request());
              NumpyVectorArray<F> vec_array(numpy_array_copy);
              gram_schmidt_cpp(vec_array);
              return vec_array.array();
          });
}

/**
 * \brief Call apply or apply_pairwise on inner_product and return the result as a numpy array.
 *
 * If \c pairwise is \c true, the form is applied to each pair of vectors in the arrays
 * and the result is a 1D array of the same length as the input arrays (which have to
 * have the same size in this case).
 * If \c pairwise is \c false, the form is applied to each vector in the first array with each vector
 * of the second array, and the result is a 2D array of shape <tt>(left.size(), right.size())</tt>.
 */
template <class F>
pybind11::array_t<F> py_apply_inner_product(const InnerProductInterface<F>& self,
                                            const VectorArrayInterface<F>& left,
                                            const VectorArrayInterface<F>& right, bool pairwise = false,
                                            const std::optional<Indices>& left_indices = std::nullopt,
                                            const std::optional<Indices>& right_indices = std::nullopt)
{
    if (pairwise)
    {
        const auto ret = self.apply_pairwise(left, right, left_indices, right_indices);
        // TODO: check if the following leads to a dangling pointer (does the array copy ret.data()?)
        // return pybind11::array(ret.size(), ret.data());
        // for now, copy the data explicitly
        pybind11::array_t<F> ret_array({std::ssize(ret)});
        auto ret_array_mutable = ret_array.mutable_unchecked();
        for (ssize_t i = 0; i < std::ssize(ret); ++i)
        {
            ret_array_mutable(i) = ret[i];
        }
    }

    const auto ret = self.apply(left, right, left_indices, right_indices);
    const ssize_t n = left_indices ? left_indices->size(left.size()) : left.size();
    const ssize_t m = right_indices ? right_indices->size(right.size()) : right.size();
    if (std::ssize(ret) != n * m)
    {
        throw nias::InvalidStateError("Result has wrong size.");
    }
    pybind11::array_t<F> ret_array({n, m});
    auto ret_array_mutable = ret_array.mutable_unchecked();
    for (ssize_t i = 0; i < n; ++i)
    {
        for (ssize_t j = 0; j < m; ++j)
        {
            ret_array_mutable(i, j) = ret[(i * m) + j];
        }
    }
    return ret_array;
}

template <class F>
    requires std::floating_point<F> || std::is_same_v<F, std::complex<typename F::value_type>>
auto bind_function_based_inner_product(pybind11::module& m, const std::string& field_type_name)
{
    namespace py = pybind11;

    class PyInnerProductInterface : public InnerProductInterface<F>
    {
       public:
        using InterfaceType = InnerProductInterface<F>;

        /* Inherit the constructors */
        using InterfaceType::InterfaceType;

        [[nodiscard]] std::vector<F> apply(
            const VectorArrayInterface<F>& left, const VectorArrayInterface<F>& right,
            const std::optional<Indices>& left_indices = std::nullopt,
            const std::optional<Indices>& right_indices = std::nullopt) const override
        {
            PYBIND11_OVERRIDE_PURE(std::vector<F>,                          /* Return type */
                                   InterfaceType,                           /* Parent class */
                                   apply,                                   /* Name of function in C++ */
                                   left, right, left_indices, right_indices /* Argument(s) */
            );
        }

        [[nodiscard]] std::vector<F> apply_pairwise(
            const VectorArrayInterface<F>& left, const VectorArrayInterface<F>& right,
            const std::optional<Indices>& left_indices = std::nullopt,
            const std::optional<Indices>& right_indices = std::nullopt) const override
        {
            PYBIND11_OVERRIDE_PURE(std::vector<F>,                          /* Return type */
                                   InterfaceType,                           /* Parent class */
                                   apply_pairwise,                          /* Name of function in C++ */
                                   left, right, left_indices, right_indices /* Argument(s) */
            );
        }
    };

    using InnerProdInterface = InnerProductInterface<F>;
    py::class_<InnerProdInterface, PyInnerProductInterface, std::shared_ptr<InnerProdInterface>>(
        m, (field_type_name + "InnerProductInterface").c_str())
        .def("apply",
             [](const InnerProdInterface& self, const VectorArrayInterface<F>& left,
                const VectorArrayInterface<F>& right, bool pairwise,
                const std::optional<Indices>& left_indices, const std::optional<Indices>& right_indices)
             {
                 return py_apply_inner_product<F>(self, left, right, pairwise, left_indices, right_indices);
             });

    using FunctionBasedInnerProd = FunctionBasedInnerProduct<F>;
    py::class_<FunctionBasedInnerProd, InnerProdInterface, std::shared_ptr<FunctionBasedInnerProd>>(
        m, (field_type_name + "FunctionBasedInnerProduct").c_str())
        .def("apply",
             [](const FunctionBasedInnerProd& self, const VectorArrayInterface<F>& left,
                const VectorArrayInterface<F>& right, bool pairwise,
                const std::optional<Indices>& left_indices, const std::optional<Indices>& right_indices)
             {
                 return py_apply_inner_product<F>(self, left, right, pairwise, left_indices, right_indices);
             });

    using EuclideanInnerProd = EuclideanInnerProduct<F>;
    auto ret =
        py::class_<EuclideanInnerProd, FunctionBasedInnerProd, std::shared_ptr<EuclideanInnerProd>>(
            m, (field_type_name + "EuclideanInnerProduct").c_str())
            .def("apply",
                 [](const EuclideanInnerProd& self, const VectorArrayInterface<F>& left,
                    const VectorArrayInterface<F>& right, bool pairwise,
                    const std::optional<Indices>& left_indices, const std::optional<Indices>& right_indices)
                 {
                     return py_apply_inner_product<F>(self, left, right, pairwise, left_indices,
                                                      right_indices);
                 });

    return ret;
}


}  // namespace nias

#endif  // NIAS_CPP_BINDINGS_H
