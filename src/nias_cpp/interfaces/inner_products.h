#ifndef NIAS_CPP_INTERFACES_INNER_PRODUCTS_H
#define NIAS_CPP_INTERFACES_INNER_PRODUCTS_H

#include <nias_cpp/concepts.h>
#include <nias_cpp/indices.h>
#include <nias_cpp/interfaces/vectorarray.h>
#include <pybind11/numpy.h>

namespace nias
{


template <floating_point_or_complex F>
class SesquilinearFormInterface
{
   public:
    using ScalarType = F;

    SesquilinearFormInterface() = default;
    virtual ~SesquilinearFormInterface() = default;

    /**
     * \brief Apply the sesquilinear form
     *
     * If \c pairwise is \c true, the form is applied to each pair of vectors in the arrays
     * and the result is a 1D array of the same length as the input arrays (which have to
     * have the same size in this case).
     * If \c pairwise is \c false, the form is applied to each vector in the first array with each vector
     * of the second array, and the result is a 2D array of shape <tt>(left.size(), right.size())</tt>.
     */
    virtual pybind11::array_t<ScalarType> apply(
        const VectorArrayInterface<ScalarType>& left, const VectorArrayInterface<ScalarType>& right,
        bool pairwise = false, const std::optional<Indices>& left_indices = std::nullopt,
        const std::optional<Indices>& right_indices = std::nullopt) const = 0;
};

template <floating_point_or_complex F>
class NormInterface
{
   public:
    using ScalarType = F;

    NormInterface() = default;
    virtual ~NormInterface() = default;

    /**
     * @brief Compute the norms of the vectors in the vector array.
     * @returns A numpy array containing the norms of the vectors in the array.
     */
    virtual pybind11::array_t<ScalarType> operator()(
        const VectorArrayInterface<ScalarType>& vec_array) const = 0;
};

// forward declaration
template <floating_point_or_complex F>
class InnerProductInterface;

template <floating_point_or_complex F>
class InnerProductBasedNorm : public NormInterface<F>
{
   public:
    InnerProductBasedNorm(const InnerProductInterface<F>& inner_product)
        : inner_product_(inner_product)
    {
    }

    virtual pybind11::array_t<F> operator()(const VectorArrayInterface<F>& vec_array) const override
    {
        auto norms = inner_product_.apply(vec_array, vec_array, true);
        for (ssize_t i = 0; i < vec_array.size(); ++i)
        {
            norms.mutable_at(i) = std::sqrt(norms.at(i));
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
    InnerProductInterface() = default;
    virtual ~InnerProductInterface() = default;

    virtual std::unique_ptr<NormInterface<F>> norm() const
    {
        return std::make_unique<InnerProductBasedNorm<F>>(*this);
    }
};


}  // namespace nias

#endif  // NIAS_CPP_INTERFACES_INNER_PRODUCTS_H
