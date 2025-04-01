#include <concepts>
#include <tuple>

#include <nias_cpp/checked_integer_cast.h>
#include <nias_cpp/concepts.h>
#include <nias_cpp/interfaces/vector.h>
#include <nias_cpp/interfaces/vectorarray.h>
#include <nias_cpp/interpreter.h>
#include <nias_cpp/type_traits.h>
#include <nias_cpp/vector/dynamic.h>
#include <nias_cpp/vectorarray/list.h>
#include <pybind11/numpy.h>

#include "../boost_ext_ut_no_module.h"
#include "common.h"

namespace
{
template <floating_point_or_complex F>
void check_random_vector_access(const VectorArrayInterface<F>& vec_array)
{
    using namespace boost::ut::bdd;

    given("A ListVectorArray v containing DynamicVectors") = [&]()
    {
        const auto& vec_array_as_list = dynamic_cast<const ListVectorArray<F>&>(vec_array);
        auto mut_vec_array = vec_array.copy();
        auto& mut_vec_array_as_list = dynamic_cast<ListVectorArray<F>&>(*mut_vec_array);

        when("Accessing vectors by index") = [&]()
        {
            then("const access as VectorInterface using the vector method works as expected") = [&]()
            {
                for (ssize_t i = 0; i < vec_array.size(); ++i)
                {
                    const auto& vec = vec_array.vector(i);
                    expect(constant<std::is_same_v<decltype(vec), const VectorInterface<F>&>>);
                    expect(exactly_equal(vec, *vec_array_as_list.vectors()[as_size_t(i)]));
                }
            };

            then("mutable access as VectorInterface using the vector method works as expected") = [&]()
            {
                for (ssize_t i = 0; i < vec_array.size(); ++i)
                {
                    auto& mut_vec = mut_vec_array->vector(i);
                    expect(constant<std::is_same_v<decltype(mut_vec), VectorInterface<F>&>>);
                    mut_vec.scal(F(2));
                    for (ssize_t j = 0; j < vec_array.dim(); ++j)
                    {
                        expect(exactly_equal(mut_vec.get(j),
                                             (*vec_array_as_list.vectors()[as_size_t(i)]).get(j) * F(2)));
                        expect(exactly_equal((*mut_vec_array_as_list.vectors()[as_size_t(i)]).get(j),
                                             (*vec_array_as_list.vectors()[as_size_t(i)]).get(j) * F(2)));
                    }
                }
            };

            then("const access as DynamicVector using the vector_as method works as expected") = [&]()
            {
                for (ssize_t i = 0; i < vec_array.size(); ++i)
                {
                    const auto& vec_as_dynamic_vec = vec_array.template vector_as<DynamicVector<F>>(i);
                    expect(constant<std::is_same_v<decltype(vec_as_dynamic_vec), const DynamicVector<F>&>>);
                    expect(exactly_equal(vec_as_dynamic_vec, *vec_array_as_list.vectors()[as_size_t(i)]));
                }
            };

            then("mutable access as DynamicVector using the vector_as method works as expected") = [&]()
            {
                for (ssize_t i = 0; i < vec_array.size(); ++i)
                {
                    auto& mut_vec_as_dynamic_vec = mut_vec_array->template vector_as<DynamicVector<F>>(i);
                    expect(constant<std::is_same_v<decltype(mut_vec_as_dynamic_vec), DynamicVector<F>&>>);
                    mut_vec_as_dynamic_vec.scal(F(2));
                    for (ssize_t j = 0; j < vec_array.dim(); ++j)
                    {
                        expect(exactly_equal(mut_vec_as_dynamic_vec.get(j),
                                             (*vec_array_as_list.vectors()[as_size_t(i)]).get(j) * F(4)));
                        expect(exactly_equal((*mut_vec_array_as_list.vectors()[as_size_t(i)]).get(j),
                                             (*vec_array_as_list.vectors()[as_size_t(i)]).get(j) * F(4)));
                    }
                }
            };
        };
    };
}
}  // namespace

int main()
{
    using namespace nias;
    using namespace boost::ut;
    using namespace boost::ut::bdd;
    ensure_interpreter_and_venv_are_active();

    "ListVectorArray"_test = []<floating_point_or_complex F>()
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

                    scenario("random vector access") = [&]()
                    {
                        check_random_vector_access(*v);
                    };
                };
            }
        }
    } | std::tuple<float, double, std::complex<float>, std::complex<double>>{};

    return 0;
}
