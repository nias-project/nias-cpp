#ifndef NIAS_CPP_INTERFACES_INNER_PRODUCTS_H
#define NIAS_CPP_INTERFACES_INNER_PRODUCTS_H

#include <algorithm>
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
     * The form is applied to each combination of vectors from \c left and \c right,
     * i.e., the <tt>(i, j)</tt>-th element of the returned vector of vectors contains
     * the result of applying the form to the <tt>i</tt>-th vector of the \c left array
     * with the <tt>j</tt>-th vector of the \c right array).
     */
    [[nodiscard]] virtual std::vector<std::vector<ScalarType>> apply(
        const VectorArrayInterface<ScalarType>& left, const VectorArrayInterface<ScalarType>& right,
        const std::optional<Indices>& left_indices = std::nullopt,
        const std::optional<Indices>& right_indices = std::nullopt) const = 0;

    /**
     * \brief Apply the sesquilinear form pairwise
     *
     * The form is applied to each pair of vectors (first vector in \c left paired with first vector of \c right,
     * second vector of \c left with second vector of \c right, and so on).
     * The result has the same length as the input arrays (which have to have the same size in this case).
     */
    [[nodiscard]] virtual std::vector<ScalarType> apply_pairwise(
        const VectorArrayInterface<ScalarType>& left, const VectorArrayInterface<ScalarType>& right,
        const std::optional<Indices>& left_indices = std::nullopt,
        const std::optional<Indices>& right_indices = std::nullopt) const = 0;
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
        auto norms = inner_product_.apply_pairwise(vec_array, vec_array);
        std::ranges::transform(norms, norms.begin(),
                               [](const auto& norm)
                               {
                                   return std::sqrt(norm);
                               });
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
    ~InnerProductInterface() override = default;

    [[nodiscard]] virtual const NormInterface<F>& induced_norm() const
    {
        return *induced_norm_;
    }

   private:
    std::unique_ptr<NormInterface<F>> induced_norm_;
};


}  // namespace nias

#endif  // NIAS_CPP_INTERFACES_INNER_PRODUCTS_H
