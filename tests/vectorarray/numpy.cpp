#include <concepts>
#include <tuple>

#include <nias_cpp/concepts.h>
#include <nias_cpp/indices.h>
#include <nias_cpp/interfaces/vectorarray.h>
#include <nias_cpp/interpreter.h>
#include <nias_cpp/type_traits.h>
#include <nias_cpp/vectorarray/numpy.h>
#include <pybind11/numpy.h>

#include "../boost_ut_no_module.h"
#include "common.h"

int main()
{
    // TODO: Clean up, quite a lot of code duplication etc, but it works for now
    using namespace nias;
    using namespace boost::ut;
    using namespace boost::ut::bdd;
    ensure_interpreter_is_running();

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
                };
            }
        }
    } | std::tuple<float, double>{};

    return 0;
}
