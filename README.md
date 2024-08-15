# C++ bindings for NIAS - Numerics In Abstract Spaces

This library provides C++ bindings for [NiAS](https://github.com/nias-project/nias) which is a Python library
providing numerical algorithms (e.g., Gram-Schmidt orthonormalization) formulated in the context of abstract
(mathematical) spaces.

## Quick Start

To run the Gram-Schmidt test (which creates a few vectors and orthogonalizes them using NiAS's Gram-Schmidt algorithm)
you can use the following commands:

```bash
python3 -m venv venv
. venv/bin/activate
pip install .
mkdir build && cd build
cmake ..
make
./tests/test_gram_schmidt
```

To run the python test, copy the test over to the build directory and run it with the python interpreter:

```bash
cp ../tests/test_gram_schmidt.py .
python3 test_gram_schmidt.py
```
