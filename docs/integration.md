# Integration Guide

## Add To Firmware

Use the library with `add_subdirectory()` from a new CMake firmware project:

```cmake
add_subdirectory(/home/gary/Downloads/attiny3217-explorer-lib attiny3217_explorer_lib)

add_executable(my_firmware
    main.cpp
    $<TARGET_OBJECTS:attiny3217_explorer_fuses>
)

target_link_libraries(my_firmware PRIVATE attiny3217_explorer)
attiny3217_explorer_apply_avr_options(my_firmware)
```

Override paths or clock values at configure time when needed:

```sh
cmake -S . -B build \
  -DCMAKE_TOOLCHAIN_FILE=/path/to/avr-gcc-toolchain.cmake \
  -DATTINY3217_EXPLORER_DFP_INCLUDE=/path/to/ATtiny_DFP/include \
  -DATTINY3217_EXPLORER_F_CPU=20000000UL
```

## Startup Choices

Use `platform_init::run()` when a project wants the full Explorer bring-up:

- 20 MHz clock
- Button interrupt on `PB4`
- USART0 at 115200 baud
- SPI0 master
- TWI0 master at 100 kHz
- SSD1306 OLED init
- TCA0 10 Hz foreground tick
- ADC0 VDD-referenced sampling
- Global interrupts enabled after setup

For smaller firmware, initialize only the pieces you need.

## ISR Ownership

These modules define interrupt vectors:

| Module | Vector |
| --- | --- |
| `timer0.cpp` | `TCA0_OVF_vect` |
| `button_irq.cpp` | `PORTB_PORT_vect` |
| `uart0_rx.cpp` | `USART0_RXC_vect` |

If a new project needs one of those vectors for different behavior, remove that source from the library target or replace the module consistently.

## Target Shape

`attiny3217_explorer` is a static library. A normal build emits:

```text
build/out/lib/libattiny3217_explorer.a
```

The final firmware target should still call `attiny3217_explorer_apply_avr_options()` so its compile and link flags match the archive's MCU, clock, LTO, exception, RTTI, and size-optimization settings.

## Fuse Policy

`attiny3217_explorer_fuses` is a separate object target. Link it explicitly when the firmware should include `.fuse` bytes:

```cmake
target_sources(my_firmware PRIVATE $<TARGET_OBJECTS:attiny3217_explorer_fuses>)
```

This avoids losing fuse data due to static archive member selection.
