#include <concepts>
#include <tuple>

#include <nias_cpp/concepts.h>
#include <nias_cpp/indices.h>
#include <nias_cpp/interfaces/vector.h>
#include <nias_cpp/interfaces/vectorarray.h>
#include <nias_cpp/interpreter.h>
#include <nias_cpp/type_traits.h>
#include <nias_cpp/vectorarray/list.h>
#include <pybind11/numpy.h>

#include "../boost_ut_no_module.h"
#include "common.h"

int main()
{
    using namespace nias;
    using namespace boost::ut;
    using namespace boost::ut::bdd;
    ensure_interpreter_is_running();

    "ListVectorArray"_test = []<std::floating_point F>()
    {
        using VecArray = ListVectorArray<F>;
        using VecArrayFactory = TestVectorArrayFactory<VecArray>;

        for (ssize_t size : {0, 1, 3, 4})
        {
            for (ssize_t dim : {0, 1, 3, 4})
            {
                test(std::format("{}x{} ListVectorArray<{}>", size, dim, reflection::type_name<F>())) =
                    [size, dim]
                {
                    // TODO: Properly test constructors
                    test("Construction") = [=]()
                    {
                        const auto vec_array = VecArrayFactory::iota(size, dim);
                        expect(vec_array->size() == size);
                        expect(vec_array->dim() == dim);
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
