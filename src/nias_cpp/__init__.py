# This file is part of the NiAS project (https://github.com/nias-project).
# Copyright NiAS developers and contributors. All rights reserved.
# License: BSD 2-Clause License (https://opensource.org/licenses/BSD-2-Clause)

from pathlib import Path

__version__ = "0.1.0"


def include_dir() -> str:
    "Return the path to the nanobind include directory"
    return Path(__file__).parent.resolve() / "include"


def cmake_dir() -> str:
    "Return the path to the nanobind CMake module directory."
    return Path(__file__).parent.resolve() / "cmake"


__all__ = (
    "__version__",
    "cmake_dir",
    "include_dir",
)
