#ifndef NIAS_CPP_INTERFACES_INNER_PRODUCTS_H
#define NIAS_CPP_INTERFACES_INNER_PRODUCTS_H

#include <memory>
#include <optional>

#include <nias_cpp/concepts.h>
#include <nias_cpp/indices.h>
#include <nias_cpp/interfaces/vectorarray.h>
#include <nias_cpp/type_traits.h>
#include <pybind11/numpy.h>

namespace nias
{


template <floating_point_or_complex F>
class SesquilinearFormInterface
{
   public:
    using ScalarType = F;

    SesquilinearFormInterface() = default;
    SesquilinearFormInterface(const SesquilinearFormInterface&) = default;
    SesquilinearFormInterface(SesquilinearFormInterface&&) = default;
    SesquilinearFormInterface& operator=(const SesquilinearFormInterface&) = default;
    SesquilinearFormInterface& operator=(SesquilinearFormInterface&&) = default;
    virtual ~SesquilinearFormInterface() = default;

    /**
     * \brief Apply the sesquilinear form
     *
     * If \c pairwise is \c true, the form is applied to each pair of vectors in the arrays and
     * the result has the same length as the input arrays (which have to have the same size in this case).
     * If \c pairwise is \c false, the form is applied to each vector in the first array with each vector
     * of the second array, and the result has size <tt>left.size() * right.size()</tt>
     * (where the <tt>i * right.size() + j</tt>-th element contains the result of applying the form to
     * the <tt>i</tt>-th vector of the left array with the <tt>j</tt>-th vector of the right array).
     */
    virtual std::vector<ScalarType> apply(
        const VectorArrayInterface<ScalarType>& left, const VectorArrayInterface<ScalarType>& right,
        bool pairwise = false, const std::optional<Indices>& left_indices = std::nullopt,
        const std::optional<Indices>& right_indices = std::nullopt) const = 0;

    /**
      * \brief Call apply and return the result as a numpy array
      *
      * If \c pairwise is \c true, the form is applied to each pair of vectors in the arrays
      * and the result is a 1D array of the same length as the input arrays (which have to
      * have the same size in this case).
      * If \c pairwise is \c false, the form is applied to each vector in the first array with each vector
      * of the second array, and the result is a 2D array of shape <tt>(left.size(), right.size())</tt>.
      *
      * \note Only used for python bindings at the moment, can be removed/improved
      * once we have a better return type for apply
      */
    virtual pybind11::array_t<ScalarType> py_apply(
        const VectorArrayInterface<ScalarType>& left, const VectorArrayInterface<ScalarType>& right,
        bool pairwise = false, const std::optional<Indices>& left_indices = std::nullopt,
        const std::optional<Indices>& right_indices = std::nullopt) const
    {
        const auto ret = apply(left, right, pairwise, left_indices, right_indices);
        if (pairwise)
        {
            return pybind11::array(ret.size(), ret.data());
        }

        const ssize_t n = left_indices ? left_indices->size(left.size()) : left.size();
        const ssize_t m = right_indices ? right_indices->size(right.size()) : right.size();
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
};

template <floating_point_or_complex F>
class NormInterface
{
   public:
    using ScalarType = F;

    NormInterface() = default;
    NormInterface(const NormInterface&) = default;
    NormInterface(NormInterface&&) = default;
    NormInterface& operator=(const NormInterface&) = default;
    NormInterface& operator=(NormInterface&&) = default;
    virtual ~NormInterface() = default;

    /**
     * @brief Compute the norms of the vectors in the vector array.
     * @returns A numpy array containing the norms of the vectors in the array.
     */
    virtual std::vector<ScalarType> operator()(const VectorArrayInterface<ScalarType>& vec_array) const = 0;
};

// forward declaration
template <floating_point_or_complex F>
class InnerProductInterface;

template <floating_point_or_complex F>
class InnerProductBasedNorm : public NormInterface<F>
{
   public:
    explicit InnerProductBasedNorm(const InnerProductInterface<F>& inner_product)
        : inner_product_(inner_product)
    {
    }

    std::vector<F> operator()(const VectorArrayInterface<F>& vec_array) const override
    {
        auto norms = inner_product_.apply(vec_array, vec_array, true);
        for (ssize_t i = 0; i < vec_array.size(); ++i)
        {
            norms[i] = std::sqrt(norms[i]);
        }
        return norms;
    }

   private:
    const InnerProductInterface<F>& inner_product_;
};

template <floating_point_or_complex F>
class InnerProductInterface : public SesquilinearFormInterface<F>
{
   public:
    InnerProductInterface()
        : induced_norm_(std::make_unique<InnerProductBasedNorm<F>>(*this)) {};

    InnerProductInterface(const InnerProductInterface&) = delete;
    InnerProductInterface(InnerProductInterface&&) = default;
    InnerProductInterface& operator=(const InnerProductInterface&) = delete;
    InnerProductInterface& operator=(InnerProductInterface&&) = default;
    virtual ~InnerProductInterface() = default;

    virtual const NormInterface<F>& norm() const
    {
        return *induced_norm_;
    }

   private:
    std::unique_ptr<NormInterface<F>> induced_norm_;
};


}  // namespace nias

#endif  // NIAS_CPP_INTERFACES_INNER_PRODUCTS_H
