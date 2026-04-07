# Installation

This project uses **vcpkg** for dependency management and **CMake** for the build system.

### Requirements
Ensure you have the following installed:

- C++ Compiler (supporting C++17 or later)
- CMake
- vcpkg
- EPICS Base

If you haven't installed vcpkg yet, follow these steps to set up the environment:

```bash
git clone https://github.com/microsoft/vcpkg.git
cd vcpkg && ./bootstrap-vcpkg.sh
export VCPKG_ROOT=/path/to/vcpkg
export PATH=$VCPKG_ROOT:$PATH
```

### (Optional) Update vcpkg baseline
To ensure the correct package versions (such as `bt.cpp`) are used as defined in the project configuration, update the vcpkg baseline:

```bash
vcpkg x-update-baseline
```

### Build
Use the provided CMake presets for a build process:
```bash
export EPICS_BASE=/path/to/EPICS_BASE

cmake --preset=release
cmake --build --preset=release
```

To remove build artifacts and start a fresh build, use the following command:
```bash
cmake --build --preset release --target clean
```
