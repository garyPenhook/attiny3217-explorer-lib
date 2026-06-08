# ATtiny3217 Explorer Library

Reusable C++23 board support for the ATtiny3217 Curiosity Nano on the Curiosity Nano Explorer board.

The library packages the board mappings and peripheral helpers from the working `avr_C++` project so later firmware can start from the same known-good pinout, clock, UART, SPI, TWI, ADC, SSD1306 OLED, and fuse policy.

## Contents

| Path | Purpose |
| --- | --- |
| `include/` | Public headers for board aliases, GPIO, clock, ADC, USART0, SPI0, TWI0, timer, OLED, button, and formatting helpers. |
| `src/` | Non-header implementations and optional ISR-backed modules. |
| `examples/telemetry_demo/` | The current telemetry application as a buildable example. |
| `docs/` | Board map, peripheral notes, and integration guide. |
| `scripts/` | Artifact and memory-report helper scripts copied from the source project. |

Start with `docs/api-reference.md` for the complete public header API, then use `docs/integration.md` for CMake wiring and `docs/peripherals.md` for hardware behavior notes.

## Assumptions

- MCU: `ATtiny3217`
- Board: ATtiny3217 Curiosity Nano mounted on Curiosity Nano Explorer
- CPU clock: internal 20 MHz oscillator, no prescaler
- Build mode: `avr-g++`, `-std=gnu++23`, exceptions and RTTI disabled
- DFP headers: supplied by the toolchain, by `ATTINY3217_EXPLORER_DFP_INCLUDE`, or by the environment variable of the same name

## Quick Build

The repository ships an AVR toolchain file and CMake presets, so a build picks up
`avr-g++` automatically (it does not fall back to the host compiler):

```sh
cmake --preset avr-examples
cmake --build build
```

This is equivalent to invoking CMake directly with the bundled toolchain file:

```sh
cmake -S . -B build \
  --toolchain cmake/avr-gcc-toolchain.cmake \
  -DATTINY3217_EXPLORER_DFP_INCLUDE=/path/to/ATtiny_DFP/include \
  -DATTINY3217_EXPLORER_BUILD_EXAMPLES=ON
cmake --build build
```

If the AVR tools are not on `PATH`, point the toolchain at them with
`-DAVR_TOOLCHAIN_DIR=/path/to/avr-gcc/bin` (or override `-DAVR_TOOLCHAIN_PREFIX=`).

Omit `ATTINY3217_EXPLORER_DFP_INCLUDE` only when your AVR toolchain already finds the Microchip ATtiny DFP headers needed by `<avr/io.h>`.

The reusable library archive is emitted as `build/out/lib/libattiny3217_explorer.a`.
The example output appears in `build/out/telemetry_demo/`.
Optional component archives are emitted in the same `build/out/lib/` directory.

## Use In A New Project

Add this directory as a subdirectory, then link `attiny3217_explorer`.

```cmake
add_subdirectory(path/to/attiny3217-explorer-lib attiny3217_explorer_lib)

add_executable(my_firmware main.cpp)
target_link_libraries(my_firmware PRIVATE
  attiny3217_explorer
  attiny3217_explorer_platform_init
  attiny3217_explorer_timer0
)
target_sources(my_firmware PRIVATE $<TARGET_OBJECTS:attiny3217_explorer_fuses>)
attiny3217_explorer_apply_avr_options(my_firmware)
```

`attiny3217_explorer` is a static-library target that builds `libattiny3217_explorer.a`. The fuse object is optional, but include it when you want the ELF to carry the source-controlled fuse bytes from `src/fuses.cpp`.

## Minimal Firmware

```cpp
#include <avr/interrupt.h>
#include "board.hpp"
#include "clock.hpp"
#include "timer0.hpp"
#include "uart0.hpp"

int main()
{
    clock::use_internal_20mhz_no_prescaler();
    board::StatusLed::configure_output();
    uart0::init<board::cpu_hz, 115200u>();
    timer0::start_periodic_interrupt(31249u, TCA_SINGLE_CLKSEL_DIV64_gc);
    sei();

    for (;;) {
        board::StatusLed::toggle();
        uart0::write_cstr("tick\r\n");
        timer0::wait_tick();
    }
}
```

Link `attiny3217_explorer_timer0` when using the timer ISR-backed helper shown above.

## Notes

- The reusable library does not include `examples/telemetry_demo/app.cpp` in the library target. Future projects should provide their own foreground loop.
- `platform_init::run()` is shipped as a separate `attiny3217_explorer_platform_init` target so projects opt into the full known-good Explorer bring-up explicitly.
- `button_irq`, `timer0`, and `uart0_rx` are separate ISR-owning targets. Link them only when you want those vectors owned by this library.
