# bch-tree

**bch-tree** is a C++ framework and command-line tool for executing **Behavior Trees**
to control **EPICS-based systems** from outside an IOC.
It is intended to orchestrate EPICS PVs using Behavior Trees as high-level control logic.

It is built upon the [BehaviorTree.CPP](https://www.behaviortree.dev/) library.

## Features
- **EPICS Integration**: Specifically designed to work within EPICS-based control systems.
- **BehaviorTree.CPP**: Leverages the features of the BehaviorTree.CPP library.
- **Groot2 Support**: Provides an option for creating and visualizing a tree via **Groot2**.

## Setup and Installation

This project uses **vcpkg** for dependency management and **CMake** for the build system.

### 1. Prerequisites
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

### 2. (Optional) Update vcpkg baseline
To ensure the correct package versions (such as `bt.cpp`) are used as defined in the project configuration, update the vcpkg baseline:

```bash
vcpkg x-update-baseline
```

### 3. Build
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

## Usage
Sample Behavior Trees are provided in the `examples/` directory. Below is an example of how to execute the **`vacuum.xml`** tree.

### 1. Start the IOC
First, start an IOC that contains the PVs to be controlled by the tree:
```bash
softIoc -m user=ET_SASAKI -d examples/db/vacuum.db
```

### 2. Execute the Behavior Tree
Next, run the Behavior Tree using the CLI tool.
The `bch-tree-cli` tool loads a Behavior Tree XML file and continuously ticks the tree,
mapping node actions and conditions to EPICS PVs.
```bash
./build/release/bch-tree-cli -t ./examples/vacuum.xml -s head=ET_SASAKI:VAC:
```

### 3. Verify the Behavior
You can verify how the tree state changes by modifying the PV values from another terminal:
```bash
camonitor ET_SASAKI:VAC:PRES ET_SASAKI:VAC:CRYO ET_SASAKI:VAC:ROUGH ET_SASAKI:VAC:VALVE ET_SASAKI:VAC:STAT
```

```bash
# Example: Change the pressure value to trigger a state change
caput ET_SASAKI:VAC:PRES 0.0000054
caput ET_SASAKI:VAC:PRES 0.0000048
```

## Running Tests
The project uses **Google Test (gtest)** for tests.

### 1. Building Tests
Tests are automatically configured when using the CMake `debug` preset. Ensure you have built the project before running tests.

```bash
export EPICS_BASE=/path/to/EPICS_BASE
cmake --preset debug
cmake --build --preset debug
```

### 2. Executing Tests
You can run the entire test suite using `ctest`:
```bash
ctest --test-dir build/debug --output-on-failure
```

## Build documentation

```bash
python -m venv env
source env/bin/activate
pip install -r docs/requirements.txt
sphinx-multiversion docs docs/_build/html
```

## Project Status

This project is under active development.
APIs and Behavior Tree node semantics may change without notice.

## License

This project is licensed under the MIT License.
See the LICENSE file for details.
