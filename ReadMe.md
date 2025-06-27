[![CMake workflow](https://github.com/LegalizeAdulthood/formula-compiler/actions/workflows/cmake.yml/badge.svg)](https://github.com/LegalizeAdulthood/formula-compiler/actions/workflows/cmake.yml)
[![C++17](https://img.shields.io/badge/C%2B%2B-17-blue.svg)](https://en.wikipedia.org/wiki/C%2B%2B17)
[![License](https://img.shields.io/github/license/LegalizeAdulthood/iterated-dynamics?label=License)](https://github.com/LegalizeAdulthood/iterated-dynamics/blob/master/LICENSE.txt)

# Formula Compiler

A library for parsing, interpreting and compiling fractal formulas, written in C++17.

# Obtaining the Source

Use git to clone this repository, then update the vcpkg submodule to bootstrap
the dependency process.

```
git clone https://github.com/LegalizeAdulthood/formula-compiler
cd formula-compiler
git submodule init
git submodule update --depth 1
```

# Building

A CMake preset has been provided to perform the usual CMake steps of
configure, build and test.

```
cmake --workflow --preset default
```

Places the build outputs in a sibling directory of the source code directory, e.g. up
and outside of the source directory.

# License

This project is licensed under the GPL-3.0-only License. See the LICENSE.txt file for details.
