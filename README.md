# C++ bindings for NIAS - Numerics In Abstract Spaces

This library provides C++ bindings for [NiAS](https://github.com/nias-project/nias) which is a Python library
providing numerical algorithms (e.g., Gram-Schmidt orthonormalization) formulated in the context of abstract
(mathematical) spaces.

## Quick Start

To run the Gram-Schmidt test (which creates a few vectors and orthogonalizes them using NiAS's Gram-Schmidt algorithm)
you can use the following commands:

```bash
python3 -m venv venv
source venv/bin/activate
pip install .
mkdir build && cd build
cmake ..
make
./tests/test_gram_schmidt
```

Alternatively, you can use `uv` for the first steps

```bash
uv venv
uv pip install .
source .venv/bin/activate
```

Additional compiler flags can be added as usual, e.g., you can use

```bash
cmake -GNinja -DCMAKE_BUILD_TYPE=Debug -Wall -Wextra ..
```

to use ninja instead of make and build with debugging information and additional warnings.

To run the python test, copy the test over to the build directory and run it with the python interpreter:

```bash
cp ../tests/test_gram_schmidt.py .
python3 test_gram_schmidt.py
```

## Current Status

We currently have

- a very simple `VectorInterface` class
- a `VectorArrayInterface` class
  - does not use `VectorInterface`
  - indices are passed as `std::vector<size_t>`, floating point vectors as `std::vector<FieldType>`
- a `ListVectorArray` fulfilling `VectorArrayInterface` and operating on `VectorInterface`
  - uses `std::vector<std::shared_ptr<VectorInterface>>` for storage
- a `NumpyVectorArray` fulfilling `VectorArrayInterface` and operating on `pybind11::array_t<FieldType>`
- Gram-Schmidt algorithms:
  - A simple implementation in C++ operating on `VectorArrayInterface`
    - python bindings for this algorithm can be used with NumPy arrays (are stored in `NumpyVectorArray`)

       ```C++
         [](const pybind11::array_t<F>& numpy_array)
         {
             auto numpy_array_copy = pybind11::array_t<F>(numpy_array.request());
             NumpyVectorArray<F> vec_array(numpy_array_copy);
             gram_schmidt_cpp(vec_array);
             return vec_array.array();
         }
       ```

       Usage from python:

       ```python
       from nias_cpp import double_gram_schmidt_cpp
       test_array = np.array([[1, 2, 3], [4, 5, 6], [7, 8, 9]], dtype=np.float64)
       ortho_test_array = double_gram_schmidt_cpp(test_array)
       ```

  - An implementation operating on `std::shared_ptr<ListVectorArray<F>> vec_array` and using the\
  Python NiAS Gram-Schmidt algorithm:

     ```C++
       py::module_ nias_cpp_vectorarray = py::module::import("nias.bindings.nias_cpp.vectorarray");
       auto NiasVecArrayImpl = nias_cpp_vectorarray.attr("NiasCppVectorArrayImpl");
       auto NiasVecArray = nias_cpp_vectorarray.attr("NiasCppVectorArray");
       auto NiasCppInnerProduct =
           py::module::import("nias.bindings.nias_cpp.product").attr("NiasCppInnerProduct");
       auto NiasGramSchmidt = py::module::import("nias.linalg.gram_schmidt").attr("gram_schmidt");
       using namespace pybind11::literals;  // for the _a literal
       auto nias_vec_array = NiasVecArray("impl"_a = NiasVecArrayImpl(vec_array));
       py::object result = NiasGramSchmidt(nias_vec_array, NiasCppInnerProduct(), "copy"_a = true);
       auto ret = result.attr("impl").attr("impl").cast<std::shared_ptr<ListVectorArray<F>>>();
     ```

    - uses "reverse bindings" in NiAS (see [this PR](https://github.com/nias-project/nias/pull/32))

- CMake support (currently requires a Python environment with `nias_cpp` installed):

    ```cmake
    find_package(Python REQUIRED COMPONENTS Interpreter)
    execute_process(
        COMMAND "${Python_EXECUTABLE}" -m nias_cpp --cmake_dir
        OUTPUT_STRIP_TRAILING_WHITESPACE
        OUTPUT_VARIABLE NIAS_CPP_DIR)
    list(APPEND CMAKE_PREFIX_PATH "${NIAS_CPP_DIR}")
    find_package(nias_cpp CONFIG REQUIRED)
    # now nias_cpp library is available for linking to our target
    ```

TODOs:

- Packaging questions (currently, `pip install .` in a virtualenv works, but\
[including it in the pyproject.toml in oasys-core does not work](https://github.com/nias-project/nias-cpp/issues/9))
  - We currently use `scikit-build-core`
  - Integrate into NiAS repo?
  - Also install python bindings with `pip` (https://github.com/nias-project/nias-cpp/issues/7)
- Mimic all relevant Python classes on the C++ side and derive the bindings from the respective Python class, for example
 [directly derive bindings for `VectorArrayInterface` from `nias::VectorArray`](https://github.com/nias-project/nias-cpp/issues/6)
  - Needs an `Indices` class which can handle at least integers, lists of integers and `pybind11::slice`
  - Needs a `VectorArrayView` class
  - Benefits:
    - would allow us to get rid of the "reverse bindings" in NiAS (?)
    - would make our C++ vector array directly usable in all Python NiAS algorithms (?)
