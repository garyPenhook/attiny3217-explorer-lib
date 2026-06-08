# ATtiny3217 Explorer Library Professionalization TODO

This library is usable, but it is not yet a professional-grade reusable library.
The main gaps are:

- board/demo concerns are mixed into reusable library code
- several modules have hidden coupling or blocking behavior
- packaging, install/export, CI, and automated verification are missing
- the API is too fixed to one demo board configuration in several places

The checklist below is ordered by priority.

## P0: Fix Architecture And Reuse Boundaries

- [ ] Split the code into layers:
  - `hal/` or `mcu/`: raw ATtiny3217 peripheral helpers (`gpio`, `clock`, `adc0`, `uart0`, `spi0`, `twi0`, `timer0`)
  - `board/`: Curiosity Nano Explorer pin map and board-specific devices
  - `drivers/`: reusable device drivers such as `ssd1306`
  - `app/examples/`: demo-only behavior
- [ ] Remove `include/app.hpp` from the public library API. It is example glue, not reusable library surface.
- [ ] Move `oled::write_status()` out of the library driver layer. It is demo/application formatting, not an SSD1306 driver responsibility.
- [ ] Replace `platform_init::run()` with smaller explicit init functions or a configurable board bring-up module. A professional library should not force one boot sequence.
- [ ] Stop shipping ISR-owning modules in the default monolithic target. `button_irq.cpp`, `timer0.cpp`, and `uart0_rx.cpp` should be optional components because they claim global vectors.
- [ ] Make the default library composition explicit in CMake:
  - `attiny3217_explorer_hal`
  - `attiny3217_explorer_board`
  - `attiny3217_explorer_drivers`
  - optional `..._timer0_irq`, `..._uart0_rx_irq`, `..._button_irq`
- [ ] Remove hidden cross-module coupling:
  - `button_irq` currently depends on `timer0::tick_count()` for debounce
  - replace this with injected timebase, callback-free timestamp input, or a documented debounce policy module

## P0: Fix Hardware Ownership And API Correctness

- [ ] Make each peripheral helper clearly own its required pin configuration, or document that ownership as part of the API contract. Right now `platform_init.cpp` manually configures pins with raw `PORTA/PORTB` writes while helpers assume the pins are already correct.
- [ ] Refactor `platform_init.cpp` to use `board.hpp` and `gpio.hpp` instead of repeating raw bit numbers.
- [ ] Add compile-time configuration points for board wiring:
  - analog input channel
  - OLED presence/address
  - UART/SPI/TWI route selection
  - optional peripherals
- [ ] Replace fixed analog assumptions in `analog_sensor.cpp`:
  - `AIN7` is hard-coded
  - `5000 mV` VDD is hard-coded
  - conversion policy should be configurable or board-defined
- [ ] Introduce typed status/error results instead of bare `bool` in bus/device APIs where failure reasons matter.
  - Example: `ok`, `timeout`, `nack`, `bus_error`, `arb_lost`, `invalid_arg`
- [ ] Add parameter validation where invalid inputs can silently misbehave.
  - Example: validate TWI baud math range, timer period assumptions, display page/text limits
- [ ] Remove or justify the public `__AVR_ATtiny3217__` compile definition in CMake. The active MCU should come from `-mmcu`, not from a hard-coded extra definition.
- [ ] Decide whether the library is truly C++23. If not, lower the public requirement to the minimum needed. Current code mostly looks compatible with older standards except for `auto` parameters in `reg8()`.

## P1: Make Drivers More Complete And Less Fragile

- [ ] Expand `twi0` beyond write-only transactions:
  - repeated-start support
  - reads
  - register read/write helpers
  - explicit transaction lifecycle
  - optional bus recovery/reset sequence
- [ ] Add timeout-aware or non-blocking variants for currently unbounded blocking APIs:
  - `adc0::read_blocking()`
  - `spi0::transfer()`
  - `uart0::write_byte()`
  - `timer0::wait_tick()`
- [ ] Define and document concurrency/ISR safety rules for each API.
  - what is safe in ISR context
  - what requires interrupts enabled
  - what is foreground-only
- [ ] Add overflow/error counters for `uart0_rx`.
  - dropped bytes are currently silent
  - framing/parity/buffer-overflow faults should be observable
- [ ] Add a way to flush/reset the UART RX buffer explicitly.
- [ ] Consider whether `timer0` should count missed ticks instead of using a single latched `bool`. The current design loses information if foreground work runs longer than one period.
- [ ] Make debounce policy configurable in `button_irq` instead of hard-coding `2` timer ticks.
- [ ] Rename `oled` to `ssd1306` if the driver is device-specific. Use `oled` only for higher-level board display wrappers.
- [ ] Separate text rendering from transport/display control in the OLED driver.
- [ ] Add a minimal framebuffer or page-buffer API if display composition is expected to grow.

## P1: Professionalize CMake And Packaging

- [ ] Add install rules for headers, archives, and exported CMake targets.
- [ ] Export a package config:
  - `attiny3217_explorerConfig.cmake`
  - `attiny3217_explorerTargets.cmake`
- [ ] Add version metadata in `project(... VERSION x.y.z)`.
- [ ] Add `CMakePresets.json` for common AVR configure/build flows.
- [ ] Stop forcing global archiver tools through cache variables unless necessary; scope toolchain behavior carefully and document why it is required for AVR LTO.
- [ ] Use generator expressions for build/install include paths.
- [ ] Make warnings configurable and strengthen them for development builds.
  - include `-Werror` option gate
  - include conversion/sign/shadow checks where supported
- [ ] Add separate options for:
  - building examples
  - building docs
  - building tests/host tests
  - enabling strict warnings
  - enabling size reports
- [ ] Add explicit toolchain expectations to CMake configure checks.
  - verify `avr-g++`, `avr-objcopy`, `avr-objdump`, `avr-readelf`, `avr-nm`
- [ ] Decide whether fuse support belongs in the base package or a separate optional target/package.

## P1: Add Testing And Verification

- [ ] Add host-side unit tests for pure logic:
  - `bit_ops`
  - formatting helpers
  - ADC scaling math
  - checksum generation
  - glyph lookup / decimal formatting
- [ ] Add compile-only tests that instantiate the public templates with expected parameters.
- [ ] Add target-side smoke tests or hardware validation procedures for:
  - UART TX/RX
  - TWI bus probing
  - SPI transfer
  - timer tick rate
  - button interrupt behavior
  - OLED init/write
- [ ] Add size-regression checks for flash and SRAM budgets.
- [ ] Add static analysis:
  - `clang-tidy` for host-checkable code
  - `cppcheck` where practical
- [ ] Add formatting/lint checks and make them reproducible.
- [ ] Add CI for at least:
  - configure
  - build library
  - build example
  - run host tests
  - run lint/static analysis

## P1: Improve Documentation And API Contracts

- [ ] Add a clear project scope statement:
  - MCU support
  - board support
  - what is generic vs board-specific
  - what is stable vs internal
- [ ] Add per-module contracts:
  - required clock assumptions
  - required pin routing
  - blocking behavior
  - ISR ownership
  - reentrancy/thread-safety
  - error semantics
- [ ] Add a compatibility/support matrix:
  - supported compiler versions
  - tested AVR toolchains
  - tested board revisions
- [ ] Document fuse policy in practical terms, including why defaults were chosen and when users should override them.
- [ ] Add migration guidance for consumers who want only the HAL pieces and not board/demo code.
- [ ] Add diagrams or tables for module dependencies and ISR ownership.
- [ ] Document timing formulas used by:
  - USART baud
  - TWI MBAUD
  - timer tick period
  - ADC millivolt conversion assumptions

## P2: Cleanup And Consistency

- [ ] Standardize naming:
  - choose `uart` vs `usart` terminology and use it consistently
  - choose `oled` vs `ssd1306` layering consistently
- [ ] Normalize header comments and remove typo-level noise such as double punctuation.
- [ ] Reduce repeated `reg8(auto value)` helpers by centralizing the conversion utility if it remains necessary.
- [ ] Prefer `[[nodiscard]]` on APIs where ignored return values are likely bugs.
  - especially bus/device operations returning status
- [ ] Consider replacing raw `uint8_t`/`uint16_t` option bags with small typed config structs.
- [ ] Audit every public header for minimal includes and forward-only dependencies.
- [ ] Add namespace-level design notes so consumers know which APIs are intended to remain stable.
- [ ] Add changelog and semantic versioning policy.
- [ ] Add contribution guidelines and release checklist.

## Suggested Execution Order

1. Split layers and optionalize ISR modules.
2. Remove demo behavior from the public library.
3. Make board wiring and analog scaling configurable.
4. Introduce typed error/status returns for bus/device code.
5. Add install/export/versioned CMake packaging.
6. Add host tests, CI, and static analysis.
7. Tighten docs, contracts, and release process.

## Short Verdict

Yes, the library can be improved substantially.

Its current state is best described as:

- a solid prototype / internal reusable code drop
- good low-level style for AVR register work
- not yet a professional reusable library distribution
