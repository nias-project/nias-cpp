# C++ bindings for NIAS - Numerics In Abstract Spaces

This library provides C++ bindings for [NiAS](https://github.com/nias-project/nias) which is a Python library providing numerical algorithms (e.g., Gram-Schmidt orthonormalization) formulated in the context of abstract (mathematical) spaces.

## Usage

To use the library, simply add `nias-cpp` as a submodule to your project

```bash
git submodule add https://github.com/nias-project/nias-cpp.git nias_cpp
git submodule update --init
```

and include it in your project's `CMakeLists.txt`:

```cmake
add_subdirectory(nias-cpp)
```

In order to use the algorithms provided by NiAS, you will have to make sure that the vector class which you are using has all the methods that the algorithm needs (e.g., an `axpy` method) and that Python bindings are available for the vector class. nias-cpp provides a `NiasVector` C++20 concept which you can use to check whether a vector class is compatible with NiAS:

```cpp
// my_vector.h

#include <nias_cpp/vector.h>

class MyVector;

// check that MyVector fulfills the NiasVector concept
static_assert(nias::NiasVector<MyVector>);
```

nias-cpp also provides a convenience function to create Python bindings for a vector class which fulfills the `NiasVector` concept:

```cpp
// my_vector.cpp

# include <my_vector.h>

// ... implementation of MyVector ...

// create Python bindings for MyVector
PYBIND11_MODULE(my_vector, m)
{
    nias::bind_nias_vector<MyVector>(m, "MyVector");
}
```

You can then create a library target for your vector class and its bindings using the `nias_cpp_add_module` CMake function:

```cmake
nias_cpp_add_module(myvector my_vector.h my_vector.cpp)
add_executable(my_executable ...)
target_link_libraries(my_executable PRIVATE myvector)
```
