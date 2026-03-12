# infix-parser

[![Linux](https://github.com/foolnotion/infix-parser/actions/workflows/ci.yml/badge.svg?job=test-linux)](https://github.com/foolnotion/infix-parser/actions/workflows/ci.yml)
[![macOS](https://github.com/foolnotion/infix-parser/actions/workflows/ci.yml/badge.svg?job=test-macos)](https://github.com/foolnotion/infix-parser/actions/workflows/ci.yml)
[![Windows](https://github.com/foolnotion/infix-parser/actions/workflows/ci.yml/badge.svg?job=test-windows)](https://github.com/foolnotion/infix-parser/actions/workflows/ci.yml)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)

A fast C++20 infix expression parser built on [lexy](https://github.com/foonathan/lexy).
Parses mathematical expressions into a compact reverse-prefix AST suitable for
stack-based evaluation and automatic differentiation.

## Features

- **Fast number parsing** via [fast-float](https://github.com/fastfloat/fast_float)
- **Reverse-prefix encoding** — binary `[rhs, lhs, op]`, unary `[child, op]`, ternary `[else, then, cond, op]`
- **Rich operator set**

| Category | Symbols / names |
|---|---|
| Arithmetic | `+` `-` `*` `/` `^` `%` `\|` |
| Comparison | `<` `>` `<=` `>=` `==` `!=` |
| Unary | `abs` `exp` `exp2` `log` `log2` `log10` `sqrt` `cbrt` `erf` `sin` `cos` `tan` `asin` `acos` `atan` `sinh` `cosh` `tanh` `sigmoid` `square` `cube` |
| Binary | `aq` `pow` `min` `max` `atan2` `hypot` `fmod` |
| Ternary | `if` (soft, differentiable) |

- All functions are **differentiable** — designed to integrate with Ceres/operon

## Prerequisites

| Tool | Version |
|---|---|
| CMake | ≥ 3.14 |
| C++ compiler | C++20 (GCC 12+, Clang 15+, MSVC 2022) |
| vcpkg | any recent |

Dependencies (managed by vcpkg): `lexy`, `fast-float`, `fmt`.
Optional (enabled via `VCPKG_MANIFEST_FEATURES=test`): `catch2`, `nanobench`.

## Building

### Linux

```sh
# Bootstrap vcpkg (once)
git clone https://github.com/microsoft/vcpkg "$VCPKG_ROOT"
"$VCPKG_ROOT/bootstrap-vcpkg.sh"

cmake --preset=ci-ubuntu   # release + clang-tidy + cppcheck
cmake --build build -j$(nproc)
```

Or without presets:

```sh
cmake -S . -B build \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_CXX_STANDARD=20 \
  -DCMAKE_TOOLCHAIN_FILE="$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake"
cmake --build build -j$(nproc)
```

### macOS

```sh
cmake --preset=release-darwin   # Xcode generator, vcpkg
cmake --build build --config Release
```

Or without presets:

```sh
cmake -S . -B build \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_CXX_STANDARD=20 \
  -DCMAKE_TOOLCHAIN_FILE="$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake"
cmake --build build -j$(sysctl -n hw.logicalcpu)
```

### Windows (MSVC)

Run the commands in a **Developer PowerShell for VS 2022** or any shell where
`cl.exe` is on `PATH`.

```powershell
cmake --preset=ci-windows   # Visual Studio 17 2022, static vcpkg triplet
cmake --build build --config Release
```

Or without presets:

```powershell
cmake -S . -B build `
  -G "Visual Studio 17 2022" -A x64 `
  -DCMAKE_CXX_STANDARD=20 `
  -DCMAKE_TOOLCHAIN_FILE="$env:VCPKG_ROOT\scripts\buildsystems\vcpkg.cmake" `
  -DVCPKG_TARGET_TRIPLET=x64-windows-static-md
cmake --build build --config Release
```

### Nix flake

```sh
nix build github:foolnotion/infix-parser
```

To use as a flake input:

```nix
inputs.infix-parser.url = "github:foolnotion/infix-parser";
# Share the foolnotion overlay to avoid duplicate lexy/fast-float in closure:
inputs.infix-parser.inputs.foolnotion.follows = "foolnotion";
```

## Running tests

Tests require the `test` vcpkg feature and CMake's `BUILD_TESTING`:

```sh
cmake -S . -B build \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_TOOLCHAIN_FILE="$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake" \
  -DVCPKG_MANIFEST_FEATURES=test \
  -DBUILD_TESTING=ON
cmake --build build -j$(nproc)
ctest --test-dir build --output-on-failure
```

## Benchmarks

```sh
cmake -S . -B build \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_TOOLCHAIN_FILE="$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake" \
  -DVCPKG_MANIFEST_FEATURES=test \
  -DBUILD_BENCHMARKS=ON
cmake --build build -j$(nproc)
./build/benchmark/infix-parser-bench
```

## Consuming the library via CMake

After installation (`cmake --install build --prefix /usr/local`):

```cmake
find_package(infix-parser REQUIRED)
target_link_libraries(my_target PRIVATE infix-parser::infix-parser)
```

All dependencies (lexy, fast-float, fmt) are private implementation details —
consumers need no extra `find_package` calls.

## Acknowledgements

The parser is built on [lexy](https://github.com/foonathan/lexy) by Jonathan
Müller — a header-only parser combinator library that made expressing the infix
grammar straightforward and type-safe.

## License

MIT — see [LICENSE](LICENSE).
Copyright © 2026 Bogdan Burlacu \<bogdan.burlacu@pm.me\>
