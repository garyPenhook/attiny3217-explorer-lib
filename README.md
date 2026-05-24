# ATtiny3217 Explorer Library

Reusable C++23 board support for the ATtiny3217 Curiosity Nano on the Curiosity Nano Explorer board.

The library packages the board mappings and peripheral helpers from the working `avr_C++` project so later firmware can start from the same known-good pinout, clock, UART, SPI, TWI, ADC, timer, button interrupt, SSD1306 OLED, and fuse policy.

## Contents

| Path | Purpose |
| --- | --- |
| `include/` | Public headers for board aliases, GPIO, clock, ADC, USART0, SPI0, TWI0, timer, OLED, button, and formatting helpers. |
| `src/` | Non-header implementations and ISR-backed modules. |
| `examples/telemetry_demo/` | The current telemetry application as a buildable example. |
| `docs/` | Board map, peripheral notes, and integration guide. |
| `scripts/` | Artifact and memory-report helper scripts copied from the source project. |

Start with `docs/api-reference.md` for the complete public header API, then use `docs/integration.md` for CMake wiring and `docs/peripherals.md` for hardware behavior notes.

## Assumptions

- MCU: `ATtiny3217`
- Board: ATtiny3217 Curiosity Nano mounted on Curiosity Nano Explorer
- CPU clock: internal 20 MHz oscillator, no prescaler
- Build mode: `avr-g++`, `-std=gnu++23`, exceptions and RTTI disabled
- DFP include path default: `/home/gary/.mchp_packs/Microchip/ATtiny_DFP/3.4.278/include`

## Quick Build

```sh
cmake -S . -B build \
  -DCMAKE_TOOLCHAIN_FILE=/home/gary/Projects/avr_C++/cmake/avr-gcc-toolchain.cmake \
  -DATTINY3217_EXPLORER_BUILD_EXAMPLES=ON
cmake --build build
```

The reusable library archive is emitted as `build/out/lib/libattiny3217_explorer.a`.
The example output appears in `build/out/telemetry_demo/`.

## Use In A New Project

Add this directory as a subdirectory, then link `attiny3217_explorer`.

```cmake
add_subdirectory(/home/gary/Downloads/attiny3217-explorer-lib attiny3217_explorer_lib)

add_executable(my_firmware main.cpp)
target_link_libraries(my_firmware PRIVATE attiny3217_explorer)
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

## Notes

- The reusable library does not include `examples/telemetry_demo/app.cpp` in the library target. Future projects should provide their own foreground loop.
- `platform_init::run()` is included for projects that want the full known-good Explorer bring-up.
- `button_irq`, `timer0`, and `uart0_rx` install ISRs. Avoid defining the same interrupt vectors elsewhere unless you replace these modules.
