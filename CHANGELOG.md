# Changelog

All notable changes to this project are documented here. The format is based on
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/) and this project follows
[Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

## [0.2.0] - 2026-06-08

This release professionalizes the library: layered targets, packaging, typed
errors, configurability, host tests, CI, and documented contracts. It contains
breaking changes to CMake target names and the TWI API.

### Added
- AVR toolchain file (`cmake/avr-gcc-toolchain.cmake`) and `CMakePresets.json`
  so builds select `avr-gcc`/`avr-g++` automatically instead of the host
  compiler.
- Layered, namespaced CMake targets: `attiny3217_explorer::hal` (header-only),
  `::board`, `::drivers`, `::timer0_irq`, `::uart0_rx_irq`, `::button_irq`,
  `::platform_init`, `::fuses`, and an `::attiny3217_explorer` umbrella.
- Install rules and an exported package (`find_package(attiny3217_explorer)`),
  with version compatibility (`SameMajorVersion`).
- Build options: `BUILD_TESTS`, `STRICT_WARNINGS` (a `-Werror` gate), `INSTALL`.
- Typed TWI transaction API: `twi0::Status`, register/buffer read and write
  helpers, repeated-start reads, address/argument validation, and `[[nodiscard]]`.
- Board-configurable analog input (`board::analog_sensor_channel` /
  `analog_sensor_vref_mv`) and a host-testable `adc_scale::to_millivolts`.
- Host unit tests (CTest) for `bit_ops`, `fast_format`, and `adc_scale`.
- `.clang-format`, cppcheck configuration, and a GitHub Actions CI pipeline.
- `docs/contracts.md` with per-module clock/pin/blocking/ISR/error contracts.

### Changed
- Lowered the required C++ standard from C++23 to C++20 (the code only uses
  C++20 features).
- `analog_sensor` no longer hard-codes channel AIN7 or a 5000 mV reference.
- Promoted `append_decimal_u16` into `fast_format.hpp` for reuse.

### Removed
- `app.hpp` is no longer a public header; it now lives with the example.
- Dropped the redundant `__AVR_ATtiny3217__` compile definition; the MCU comes
  from `-mmcu`.

### Migration
- CMake: replace `attiny3217_explorer_timer0` / `_uart0_rx` with
  `attiny3217_explorer_timer0_irq` / `_uart0_rx_irq`, and prefer the
  `attiny3217_explorer::` namespaced targets.
- TWI: replace `if (!twi0::start_write(addr))` with
  `if (twi0::start_write(addr) != twi0::Status::ok)`; `write_byte` likewise.

## [0.1.0] - 2026-05-24
### Added
- Initial ATtiny3217 Explorer library: HAL helpers, SSD1306 driver, optional
  ISR modules, fuse policy, and the telemetry example.

[Unreleased]: https://github.com/garyPenhook/attiny3217-explorer-lib/compare/v0.2.0...HEAD
[0.2.0]: https://github.com/garyPenhook/attiny3217-explorer-lib/releases/tag/v0.2.0
[0.1.0]: https://github.com/garyPenhook/attiny3217-explorer-lib/releases/tag/v0.1.0
