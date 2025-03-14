import sys
from pathlib import Path

import toml


def get_dependency_version(dependency_name):
    try:
        with Path.open("pyproject.toml") as file:
            pyproject_data = toml.load(file)

        dependencies = pyproject_data.get("project").get("dependencies")

        for dependency in dependencies:
            if dependency.startswith(dependency_name):
                return dependency.split("==")[-1]
        return f"Error: Dependency '{dependency_name}' not found."
    except FileNotFoundError:
        return "Error: pyproject.toml file not found."
    except Exception as e:
        return f"Error: An error occurred: {e}"


if __name__ == "__main__":
    if len(sys.argv) != 2:  # noqa: PLR2004
        print("Usage: python parse_pyproject_toml.py <dependency_name>")
    else:
        dependency_name = sys.argv[1]
        version = get_dependency_version(dependency_name)
        print(version, end="")
