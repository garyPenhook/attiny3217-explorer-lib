# Scope and module contracts

## Project scope

- **MCU:** ATtiny3217 only. Other tinyAVR 1-series parts may work but are not
  tested.
- **Board:** ATtiny3217 Curiosity Nano on the Curiosity Nano Explorer.
- **Clock:** internal 20 MHz oscillator, no prescaler (`F_CPU = 20000000UL`).
- **Toolchain:** `avr-gcc`/`avr-g++` with `-std=gnu++20`, exceptions and RTTI
  disabled.

### Generic vs board-specific

| Layer | Headers | Nature |
| --- | --- | --- |
| `hal` | `gpio`, `clock`, `adc0`, `uart0`, `spi0`, `twi0`, `timer0`, `bit_ops`, `fast_format`, `adc_scale`, `fuses` | Generic ATtiny3217 peripheral helpers. |
| `board` | `board`, `analog_sensor`, `blink_task` | Explorer board wiring and devices. |
| `drivers` | `oled` | Reusable device driver (SSD1306). |
| ISR components | `timer0`, `uart0_rx`, `button_irq` | Generic, but each claims a global vector. |
| `platform_init` | `platform_init` | Opinionated demo bring-up; not reusable as-is. |

### Stability

Public, intended-stable: `hal`, `board`, `drivers`, and the ISR component APIs.
Internal/example: `platform_init` and the example's `app.hpp`. Until the project
reaches 1.0.0, minor versions may still make breaking changes (documented in the
[changelog](../CHANGELOG.md)).

## Per-module contracts

Unless stated otherwise, functions are **foreground-only** (run with the bus/
peripheral owned by the caller) and assume `F_CPU = 20 MHz`.

| Module | Clock/pins | Blocking | ISR ownership | Reentrancy | Error semantics |
| --- | --- | --- | --- | --- | --- |
| `gpio` | Caller selects port/bit at compile time | No | None | Pin-stateless; safe anywhere | None |
| `clock` | Configures the main clock (CCP-protected) | No | None | Foreground-only | None |
| `adc0` | Caller owns the analog pin | `read_blocking` polls until result-ready | None | Foreground-only | None (unbounded poll) |
| `analog_sensor` | `board::analog_sensor_channel`, `_vref_mv` (overridable) | Yes (via `adc0`) | None | Foreground-only | None |
| `uart0` | `board::Usart0Tx/Rx`, default PORTMUX | `write_byte` polls DREIF | None | Foreground-only | None |
| `uart0_rx` | Same USART0 pins | No (ring buffer) | **`USART0_RXC_vect`** | `read_byte`/`available` are ISR-safe vs the RX ISR | Frame/parity/overflow bytes dropped silently |
| `spi0` | `board::Spi0*`, default PORTMUX | `transfer` polls RXCIF | None | Foreground-only | None |
| `twi0` | `board::Twi0Scl/Sda`, default PORTMUX | All transactions poll with a bounded iteration budget | None | Foreground-only; **not ISR-safe** | `Status` enum; `[[nodiscard]]` |
| `timer0` | TCA0 | `wait_tick` busy-waits | **`TCA0_OVF_vect`** | `tick_*` accessors are ISR-safe | Single latched tick (missed ticks not counted) |
| `button_irq` | `board::ButtonIn` (PB4) | No | **`PORTB_PORT_vect`** | `consume_*`/`pressed` are ISR-safe | Debounce timebase injected by caller |
| `oled` | TWI0 + `oled::address` | Yes (via `twi0`) | None | Foreground-only | `bool` per call |
| `blink_task` | `board::StatusLed` | No | None | Foreground-only | None |
| `platform_init` | Owns all of the above | Yes | Enables global interrupts (`sei`) | Call once at startup | None |

### ISR ownership summary

Linking these components claims the named vectors, so a project must not also
define them:

- `attiny3217_explorer::timer0_irq` → `TCA0_OVF_vect`
- `attiny3217_explorer::uart0_rx_irq` → `USART0_RXC_vect`
- `attiny3217_explorer::button_irq` → `PORTB_PORT_vect`

## Timing formulas

- **USART baud** (normal async): `BAUD = (4 * F_CPU) / baud_rate`.
- **TWI MBAUD**: `MBAUD = (F_CPU / bus_rate - 10) / 2` (requires
  `F_CPU / bus_rate >= 10`).
- **TCA0 tick period**: `f_tick = F_CPU / prescaler / (PER + 1)`. The example
  uses `20 MHz / 64 / (31249 + 1) = 10 Hz`.
- **ADC millivolts**: `mV = round(raw * vref_mv / 1024)` for the 10-bit result.

## Compatibility matrix

| Component | Tested |
| --- | --- |
| Compiler | `avr-g++` 15.2 (CI), 16.1 (dev) |
| C++ standard | `gnu++20` |
| MCU | ATtiny3217 |
| Board | ATtiny3217 Curiosity Nano + Curiosity Nano Explorer |
| Host tests | GCC/Clang on `ubuntu-latest` |
