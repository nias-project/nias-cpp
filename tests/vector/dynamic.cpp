#include <concepts>
#include <tuple>

#include <nias_cpp/concepts.h>
#include <nias_cpp/exceptions.h>
#include <nias_cpp/interfaces/vectorarray.h>
#include <nias_cpp/interpreter.h>
#include <nias_cpp/type_traits.h>
#include <nias_cpp/vector/dynamic.h>
#include <nias_cpp/vectorarray/numpy.h>
#include <pybind11/numpy.h>

#include "../boost_ext_ut_no_module.h"

// namespace
// {
// template <floating_point_or_complex F>
// void check_random_vector_access(const VectorArrayInterface<F>& v)
// {
//     using namespace boost::ut::bdd;
//
//     given("A ListVectorArray v containing DynamicVectors") = [&]()
//     {
//         const auto v_original = v.copy();
//         auto v_mut = v.copy();
//
//         when("Trying to access vectors by index") = [&]()
//         {
//             then("An error is thrown since NumpyVectorArray does not provide random vector access") = [&]()
//             {
//                 for (ssize_t i = 0; i < v.size(); ++i)
//                 {
//                     expect(fatal(throws<NotImplementedError>(
//                         [&]()
//                         {
//                             static_cast<void>(v.vector(i));
//                         })));
//                     expect(fatal(throws<NotImplementedError>(
//                         [&]()
//                         {
//                             v_mut->vector(i).scal(F(42));
//                         })));
//
//                     expect(fatal(throws<NotImplementedError>(
//                         [&]()
//                         {
//                             static_cast<void>(v.template vector_as<DynamicVector<F>>(i));
//                         })));
//
//                     expect(fatal(throws<NotImplementedError>(
//                         [&]()
//                         {
//                             v_mut->template vector_as<DynamicVector<F>>(i).scal(F(42));
//                         })));
//                 };
//                 then("v remains unchanged") = [&]()
//                 {
//                     expect(exactly_equal(v, *v_original));
//                     expect(exactly_equal(*v_mut, *v_original));
//                 };
//             };
//         };
//     };
// }
// }  // namespace

int main()
{
    using namespace nias;
    using namespace boost::ut;
    using namespace boost::ut::bdd;
    ensure_interpreter_and_venv_are_active();

    "DynamicVector"_test = []<floating_point_or_complex F>()
    {
        using Vec = DynamicVector<F>;

        for (ssize_t dim : {0, 1, 3, 4})
        {
            test(std::format("DynamicVector<{}> of dim {}", reflection::type_name<F>(), dim)) = [dim]()
            {
                test("Constructing a DynamicVector<F> by dim and value") = [=]()
                {
                    expect(nothrow(
                        [&]()
                        {
                            return Vec(dim);
                        }));
                    expect(nothrow(
                        [&]()
                        {
                            return Vec(dim, F(42));
                        }));
                    const auto check_vec = [dim](const auto& vec, F value)
                    {
                        for (ssize_t i = 0; i < dim; ++i)
                        {
                            expect(vec.get(i) == value);
                        }
                        expect(vec.dim() == dim);
                        expect(vec.size() == dim);
                    };
                    check_vec(Vec(dim), F(0));
                    check_vec(Vec(dim, F(42)), F(42));
                };
                //
                // const auto vec = VecArrayFactory::iota(size, dim);
                // scenario("Copying") = [&]()
                // {
                //     check_copy(*v, size, dim);
                // };
                //
                // scenario("append") = [&]()
                // {
                //     check_append<VecArray>(*v, size, dim);
                // };
                //
                // scenario("scal") = [&]()
                // {
                //     check_scal(*v, size, dim);
                // };
                //
                // scenario("axpy") = [&]()
                // {
                //     check_axpy<VecArray>(*v, size, dim);
                // };
                // scenario("random vector access") = [&]()
                // {
                //     check_random_vector_access(*v);
                // };
            };
        }
    } | std::tuple<float, double, std::complex<float>, std::complex<double>>{};

    return 0;
}
