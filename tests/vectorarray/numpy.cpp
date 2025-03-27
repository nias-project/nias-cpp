#include <concepts>
#include <tuple>

#include <nias_cpp/concepts.h>
#include <nias_cpp/exceptions.h>
#include <nias_cpp/interfaces/vectorarray.h>
#include <nias_cpp/interpreter.h>
#include <nias_cpp/type_traits.h>
#include <nias_cpp/vectorarray/numpy.h>
#include <pybind11/numpy.h>

#include "../boost_ext_ut_no_module.h"
#include "../test_vector.h"
#include "common.h"

namespace
{
template <floating_point_or_complex F>
void check_random_vector_access(const VectorArrayInterface<F>& v)
{
    using namespace boost::ut::bdd;

    given("A ListVectorArray v containing DynamicVectors") = [&]()
    {
        const auto v_original = v.copy();
        auto v_mut = v.copy();

        when("Trying to access vectors by index") = [&]()
        {
            then("An error is thrown since NumpyVectorArray does not provide random vector access") = [&]()
            {
                for (ssize_t i = 0; i < v.size(); ++i)
                {
                    expect(fatal(throws<NotImplementedError>(
                        [&]()
                        {
                            static_cast<void>(v.vector(i));
                        })));
                    expect(fatal(throws<NotImplementedError>(
                        [&]()
                        {
                            v_mut->vector(i).scal(F(42));
                        })));

                    expect(fatal(throws<NotImplementedError>(
                        [&]()
                        {
                            static_cast<void>(v.template vector_as<DynamicVector<F>>(i));
                        })));

                    expect(fatal(throws<NotImplementedError>(
                        [&]()
                        {
                            v_mut->template vector_as<DynamicVector<F>>(i).scal(F(42));
                        })));
                };
                then("v remains unchanged") = [&]()
                {
                    expect(exactly_equal(v, *v_original));
                    expect(exactly_equal(*v_mut, *v_original));
                };
            };
        };
    };
}
}  // namespace

int main()
{
    // TODO: Clean up, quite a lot of code duplication etc, but it works for now
    using namespace nias;
    using namespace boost::ut;
    using namespace boost::ut::bdd;
    ensure_interpreter_and_venv_are_active();

    "NumpyVectorArray"_test = []<std::floating_point F>()
    {
        using VecArray = NumpyVectorArray<F>;
        using VecArrayFactory = TestVectorArrayFactory<VecArray>;
        using PyArrayFactory = TestVectorArrayFactory<pybind11::array_t<F>>;

        for (ssize_t size : {0, 1, 3, 4})
        {
            for (ssize_t dim : {0, 1, 3, 4})
            {
                test(std::format("NumpyVectorArray<{}> of size {} and dim {}", reflection::type_name<F>(),
                                 size, dim)) = [size, dim]()
                {
                    test("Constructing a NumpyVectorArray<F> from a pybind11::array_t<F>") = [=]()
                    {
                        const auto py_array = PyArrayFactory::iota(size, dim);
                        expect(nothrow(
                            [&]()
                            {
                                return NumpyVectorArray<F>(*py_array);
                            }));
                        NumpyVectorArray<F> vec_array(*py_array);
                        for (ssize_t i = 0; i < size; ++i)
                        {
                            for (ssize_t j = 0; j < dim; ++j)
                            {
                                expect(vec_array.array().at(i, j) == F((i * dim) + j + 1))
                                    << "vec array has correct entries";
                            }
                        }
                        expect(vec_array.size() == size);
                        expect(vec_array.dim() == dim);
                    };

                    const auto v = VecArrayFactory::iota(size, dim);
                    scenario("Copying") = [&]()
                    {
                        check_copy(*v, size, dim);
                    };

                    scenario("append") = [&]()
                    {
                        check_append<VecArray>(*v, size, dim);
                    };

                    scenario("scal") = [&]()
                    {
                        check_scal(*v, size, dim);
                    };

                    scenario("axpy") = [&]()
                    {
                        check_axpy<VecArray>(*v, size, dim);
                    };
                    scenario("random vector access") = [&]()
                    {
                        check_random_vector_access(*v);
                    };
                };
            }
        }
    } | std::tuple<float, double>{};

    return 0;
}
