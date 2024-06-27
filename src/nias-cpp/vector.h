#ifndef NIAS_CPP_VECTOR_H
#define NIAS_CPP_VECTOR_H

#include <algorithm>
#include <concepts>
#include <iostream>
#include <set>
#include <vector>

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace nias
{


// C++20 concept for a vector of doubles which has copy, dot, scal, and axpy methods
template <class V>
concept NiasVector = requires(V v) {
    // TODO: update requirements
    { v.size() } -> std::convertible_to<std::size_t>;
    { v.copy() } -> std::same_as<V>;
    { v.dot(v) } -> std::convertible_to<double>;
    { v.scal(1.0) };
    { v.axpy(1.0, v) };
};

template <NiasVector VectorType>
class VectorArray
{
   public:
    VectorArray(const std::vector<VectorType>& vectors, size_t dim)
        : vectors_(std::move(vectors))
        , dim_(dim)
    {
        check_vec_dimensions();
    }

    VectorArray(std::vector<VectorType>&& vectors, size_t dim)
        : vectors_(std::move(vectors))
        , dim_(dim)
    {
        check_vec_dimensions();
    }

    size_t size() const
    {
        return vectors_.size();
    }

    size_t dim() const
    {
        return dim_;
    }

    const std::vector<VectorType>& vectors() const
    {
        return vectors_;
    }

    bool is_compatible_array(const VectorArray& other) const
    {
        return dim_ == other.dim_;
    }

    VectorArray copy(const std::vector<size_t>& indices = {}) const
    {
        if (indices.empty())
        {
            return VectorArray(vectors_, dim_);
        }
        else
        {
            std::vector<VectorType> copied_vectors;
            copied_vectors.reserve(indices.size());
            std::ranges::transform(indices, std::back_inserter(copied_vectors),
                                   [this](size_t i)
                                   {
                                       return vectors_.at(i);
                                   });
            return VectorArray(copied_vectors, dim_);
        }
    }

    void append(VectorArray& other, bool remove_from_other = false,
                const std::vector<size_t>& other_indices = {})
    {
        if (!is_compatible_array(other))
        {
            throw std::invalid_argument("VectorArray: incompatible dimensions.");
        }
        vectors_.reserve(vectors_.size() + other_indices.size());
        if (remove_from_other)
        {
            append_with_removal(other, other_indices);
        }
        else
        {
            append_without_removal(other, other_indices);
        }
    }

    void delete_vectors(const std::vector<size_t>& indices)
    {
        // We first sort and deduplicate indices by converting to a std::set
        // We sort in reverse order (by using std::greater as second template argument) to avoid
        // invalidating indices when removing elements from the vector
        std::set<size_t, std::greater<size_t>> sorted_indices(indices.begin(), indices.end());
        for (auto&& i : sorted_indices)
        {
            vectors_.erase(vectors_.begin() + i);
        }
    }

    void scal(const double& alpha, const std::vector<size_t>& indices = {})
    {
        if (indices.empty())
        {
            for (auto& vector : vectors_)
            {
                vector.scal(alpha);
            }
        }
        else
        {
            for (auto i : indices)
            {
                vectors_.at(i).scal(alpha);
            }
        }
    }

    void scal(const std::vector<double>& alpha, const std::vector<size_t>& indices = {})
    {
        if (indices.empty())
        {
            if (alpha.size() != size())
            {
                throw std::invalid_argument(
                    "VectorArray: alpha must be scalar or have the same length as the array.");
            }
            for (size_t i = 0; i < vectors_.size(); ++i)
            {
                vectors_.at(i).scal(alpha.at(i));
            }
        }
        else
        {
            if (alpha.size() != indices.size())
            {
                throw std::invalid_argument("VectorArray: number of scalars must match number of indices.");
            }
            for (size_t i = 0; i < indices.size(); ++i)
            {
                vectors_.at(indices.at(i)).scal(alpha.at(i));
            }
        }
    }

    void axpy(const std::vector<double>& alpha, const VectorArray& x, const std::vector<size_t>& indices = {},
              const std::vector<size_t>& x_indices = {})
    {
        if (!is_compatible_array(x))
        {
            throw std::invalid_argument("VectorArray: incompatible dimensions.");
        }
        const auto this_size = indices.empty() ? size() : indices.size();
        const auto x_size = x_indices.empty() ? x.size() : x_indices.size();
        if (!(this_size == x_size || x_size == 1))
        {
            throw std::invalid_argument("VectorArray: x must have length 1 or the same length as this");
        }
        if (!(alpha.size() == x_size || alpha.size() == 1))
        {
            throw std::invalid_argument("VectorArray: alpha must be a scalar or have the same length as x");
        }

        for (size_t i = 0; i < this_size; ++i)
        {
            const auto this_index = indices.empty() ? i : indices.at(i);
            // x can either have the same length as this or length 1
            size_t x_index;
            if (x_size == this_size)
            {
                x_index = x_indices.empty() ? i : x_indices.at(i);
            }
            else
            {
                // x has length 1
                x_index = x_indices.empty() ? 0 : x_indices.at(0);
            }
            const auto a = alpha.size() == 1 ? alpha.at(0) : alpha.at(x_index);
            vectors_.at(this_index).axpy(a, x.vectors_.at(x_index));
        }
    }

    void axpy(double alpha, const VectorArray& x, const std::vector<size_t>& indices = {},
              const std::vector<size_t>& x_indices = {})
    {
        axpy(std::vector<double> {alpha}, x, indices, x_indices);
    }

   private:
    void check_vec_dimensions()
    {
        for (const auto& vector : vectors_)
        {
            if (vector.size() != dim_)
            {
                throw std::invalid_argument("VectorArray: all vectors must have the same length.");
            }
        }
    }

    void append_without_removal(const VectorArray& other, std::vector<size_t> other_indices = {})
    {
        if (other_indices.empty())
        {
            vectors_.insert(vectors_.end(), other.vectors_.begin(), other.vectors_.end());
        }
        else
        {
            std::ranges::transform(other_indices, std::back_inserter(vectors_),
                                   [&other](size_t i)
                                   {
                                       return other.vectors_[i];
                                   });
        }
    }

    void append_with_removal(VectorArray& other, const std::vector<size_t>& other_indices)
    {
        if (other_indices.empty())
        {
            // move all vectors from other to this and then clear other
            vectors_.insert(vectors_.end(), std::make_move_iterator(other.vectors_.begin()),
                            std::make_move_iterator(other.vectors_.end()));
            other.vectors_.clear();
        }
        else
        {
            // move selected entries of other to the end of this
            std::ranges::transform(other_indices, std::back_inserter(vectors_),
                                   [&other](size_t i)
                                   {
                                       return std::move(other.vectors_[i]);
                                   });
            other.delete_vectors(other_indices);
        }
    }

    std::vector<VectorType> vectors_;
    size_t dim_;
};

template <NiasVector V>
auto bind_nias_vector(pybind11::module& m, const std::string name)
{
    namespace py = pybind11;
    auto ret = py::class_<V>(m, name.c_str())
                   .def("__len__",
                        [](const V& v)
                        {
                            return v.size();
                        })
                   .def("copy", &V::copy)
                   .def("dot", &V::dot)
                   .def("scal", &V::scal)
                   .def("axpy", &V::axpy);
    return ret;
}

template <NiasVector V>
auto bind_nias_vectorarray(pybind11::module& m, const std::string name)
{
    namespace py = pybind11;
    using VecArray = VectorArray<V>;
    auto ret =
        py::class_<VecArray>(m, name.c_str())
            .def("__len__",
                 [](const VecArray& v)
                 {
                     return v.size();
                 })
            .def("size", &VecArray::size)
            .def_property_readonly("dim", &VecArray::dim)
            .def_property_readonly("vectors", &VecArray::vectors)
            .def("copy", &VecArray::copy)
            .def("append", &VecArray::append)
            .def("delete", &VecArray::delete_vectors)
            .def("scal", py::overload_cast<const double&, const std::vector<size_t>&>(&VecArray::scal))
            .def("scal",
                 py::overload_cast<const std::vector<double>&, const std::vector<size_t>&>(&VecArray::scal))
            .def("axpy", py::overload_cast<double, const VecArray&, const std::vector<size_t>&,
                                           const std::vector<size_t>&>(&VecArray::axpy))
            .def("axpy",
                 py::overload_cast<const std::vector<double>&, const VecArray&, const std::vector<size_t>&,
                                   const std::vector<size_t>&>(&VecArray::axpy))
            .def("is_compatible_array", &VecArray::is_compatible_array);
    return ret;
}


}  // namespace nias

#endif  // NIAS_CPP_VECTOR_H
