# Development

## Running Tests
The project uses **Google Test (gtest)** for tests.

### Building Tests
Tests are automatically configured when using the CMake `debug` preset. Ensure you have built the project before running tests.

```bash
export EPICS_BASE=/path/to/EPICS_BASE
cmake --preset debug
cmake --build --preset debug
```

### Executing Tests
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
