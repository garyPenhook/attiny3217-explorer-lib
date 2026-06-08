# Contributing

Thanks for helping improve the ATtiny3217 Explorer library. This document covers
the local workflow that CI also enforces.

## Prerequisites

- An AVR GCC toolchain (`avr-g++` 15+ for C++20). If it is not on `PATH`, point
  the toolchain file at it with `-DAVR_TOOLCHAIN_DIR=/path/to/avr-gcc/bin`.
- CMake 3.21+.
- For host tests/lint: a host C++20 compiler, `cppcheck`, and `clang-format`
  (pinned to 22.1.5 in CI; other versions may format differently).

## Build the firmware and example

```sh
cmake --preset avr-examples
cmake --build build
```

## Run host unit tests

```sh
cmake -S . -B build-tests -DATTINY3217_EXPLORER_BUILD_TESTS=ON
cmake --build build-tests
ctest --test-dir build-tests --output-on-failure
```

Pure logic (`bit_ops`, `fast_format`, `adc_scale`, …) should get a host test.
Keep that logic free of AVR register dependencies so it stays host-testable.

## Format and static analysis

```sh
# Format (writes in place); CI runs the --dry-run --Werror equivalent.
clang-format -i $(find include src tests examples -name '*.hpp' -o -name '*.cpp')

cppcheck --std=c++20 --enable=warning,style,performance,portability \
  --error-exitcode=1 --inline-suppr \
  --suppressions-list=.cppcheck-suppressions -Iinclude src include
```

Hand-laid tables (e.g. the OLED command arrays) are wrapped in
`// clang-format off` / `on`; keep new tabular data the same way.

## Strict warnings

Before sending a change, build once with warnings as errors:

```sh
cmake -S . -B build-strict --toolchain cmake/avr-gcc-toolchain.cmake \
  -DATTINY3217_EXPLORER_BUILD_EXAMPLES=ON -DATTINY3217_EXPLORER_STRICT_WARNINGS=ON
cmake --build build-strict
```

## Conventions

- Match the surrounding style: 4-space indent, `reg8()` for DFP enum→register
  conversions, explicit `static_cast` for narrowing, and a comment explaining
  *why* for non-obvious register writes.
- Bus/device operations that can fail return a typed status and are
  `[[nodiscard]]`.
- Document a new module's contract (clock, pins, blocking, ISR ownership,
  reentrancy, errors) in [`docs/contracts.md`](docs/contracts.md).
- Keep ISR-owning code in its own optional component so it does not force a
  global vector on consumers.

## Release checklist

1. Update [`CHANGELOG.md`](CHANGELOG.md) (move Unreleased → new version).
2. Bump `project(... VERSION x.y.z)` in `CMakeLists.txt`.
3. Ensure CI is green (host tests, lint, firmware build).
4. Tag `vX.Y.Z` and push the tag.

Pre-1.0.0, minor releases may include breaking changes; call them out in the
changelog with a migration note.
