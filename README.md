# C++ bindings for NIAS - Numerics In Abstract Spaces

This library provides C++ bindings for [NiAS](https://github.com/nias-project/nias) which is a Python library
providing numerical algorithms (e.g., Gram-Schmidt orthonormalization) formulated in the context of abstract
(mathematical) spaces.

## Quick Start

```bash
python -m venv venv
. venv/bin/activate
pip install build
python -m build
pip install dist/nias_cpp*.whl
```
