# API Reference

This reference covers the public headers in `include/`. Headers that define ISRs or depend on specific board wiring are called out explicitly.

## `board.hpp`

Board-level aliases for the ATtiny3217 Curiosity Nano on the Curiosity Nano Explorer board.

| Symbol | Meaning |
| --- | --- |
| `board::cpu_hz` | Compile-time CPU clock, derived from `F_CPU`. |
| `board::StatusLed` | GPIO alias for `PB5`. |
| `board::ButtonIn` | GPIO alias for `PB4`. |
| `board::Usart0Tx` | GPIO alias for `PB2`. |
| `board::Usart0Rx` | GPIO alias for `PB3`. |
| `board::Spi0Sck` | GPIO alias for `PA3`. |
| `board::Spi0Mosi` | GPIO alias for `PA4`. |
| `board::Spi0Miso` | GPIO alias for `PA5`. |
| `board::Twi0Scl` | GPIO alias for `PB0`. |
| `board::Twi0Sda` | GPIO alias for `PB1`. |

## `gpio.hpp`

Zero-storage compile-time GPIO wrapper.

| Symbol | Meaning |
| --- | --- |
| `gpio::PortId` | Enum for `A`, `B`, and `C`. |
| `gpio::Pin<Port, Bit>` | Compile-time pin type. |
| `Pin::bit` | Pin bit number. |
| `Pin::mask` | Pin bit mask. |
| `Pin::port()` | Returns the matching `PORT_t` register block. |
| `Pin::configure_output()` | Sets the direction bit with `DIRSET`. |
| `Pin::configure_input()` | Clears the direction bit with `DIRCLR`. |
| `Pin::set_high()` | Sets the output latch with `OUTSET`. |
| `Pin::set_low()` | Clears the output latch with `OUTCLR`. |
| `Pin::toggle()` | Toggles the output latch with `OUTTGL`. |

Example:

```cpp
board::StatusLed::configure_output();
board::StatusLed::toggle();
```

## `clock.hpp`

Protected main-clock helpers.

| Symbol | Meaning |
| --- | --- |
| `clock::use_internal_20mhz_no_prescaler()` | Selects the internal 20 MHz oscillator and disables the main prescaler. |
| `clock::use_internal_20mhz_with_prescaler(divider)` | Selects the internal 20 MHz oscillator and enables the requested prescaler. |

These functions use `ccp_write_io()` because `CLKCTRL` registers are protected.

## `adc0.hpp`

Blocking ADC0 helper functions.

| Symbol | Meaning |
| --- | --- |
| `adc0::init_vdd(prescaler)` | Configures ADC0 with VDD as the reference. Default prescaler is `ADC_PRESC_DIV16_gc`. |
| `adc0::init_internal_1v1(prescaler)` | Enables the internal 1.1 V reference and configures ADC0 to use it. |
| `adc0::read_blocking(channel)` | Performs one blocking conversion on an `ADC_MUXPOS_t` channel and returns the 10-bit result. |

## `analog_sensor.hpp`

Explorer-level analog input wrapper.

| Symbol | Meaning |
| --- | --- |
| `analog_sensor::init()` | Initializes ADC0 for VDD-referenced sampling. |
| `analog_sensor::sample()` | Samples the configured analog channel, currently `ADC0 AIN7`. |
| `analog_sensor::to_millivolts(raw_sample)` | Converts a 10-bit ADC count to millivolts using a 5000 mV VDD assumption. |
| `analog_sensor::sample_millivolts()` | Samples and converts in one call. |

Adjust `src/analog_sensor.cpp` for projects that use a different analog input or board supply.

## `uart0.hpp`

Blocking USART0 transmit and basic USART setup.

| Symbol | Meaning |
| --- | --- |
| `uart0::PinRoute` | `Default` or `Alternate` PORTMUX route. |
| `uart0::Config<CpuHz, Baud>::baud_reg` | Compile-time USART baud register value. |
| `uart0::select_pins(route)` | Selects the USART0 PORTMUX route. |
| `uart0::init<CpuHz, Baud>(route)` | Configures USART0 for 8N1 transmit and receive. |
| `uart0::write_byte(value)` | Blocks until TX data register is ready, then sends one byte. |
| `uart0::write_cstr(text)` | Sends a NUL-terminated string. |
| `uart0::write_buffer(data, size)` | Sends exactly `size` bytes. |

Example:

```cpp
uart0::init<board::cpu_hz, 115200u>();
uart0::write_cstr("ready\r\n");
```

## `uart0_rx.hpp`

Interrupt-backed USART0 receive ring buffer.

| Symbol | Meaning |
| --- | --- |
| `uart0_rx::init()` | Clears indices and enables receive-complete interrupts. |
| `uart0_rx::available()` | Returns true when at least one byte is buffered. |
| `uart0_rx::read_byte(value)` | Pops one byte into `value` if available. |

ISR ownership: defines `USART0_RXC_vect`.

## `spi0.hpp`

Blocking SPI0 master helper.

| Symbol | Meaning |
| --- | --- |
| `spi0::PinRoute` | `Default` or `Alternate` PORTMUX route. |
| `spi0::select_pins(route)` | Selects the SPI0 PORTMUX route. |
| `spi0::init_master(prescaler, mode, double_speed, route)` | Enables SPI0 as master. Defaults to mode 0, prescaler DIV16, default route. |
| `spi0::transfer(value)` | Blocks until one full-duplex byte transfer completes and returns the received byte. |

## `twi0.hpp`

Blocking TWI0/I2C master helper with bounded polling.

| Symbol | Meaning |
| --- | --- |
| `twi0::default_timeout_iterations` | Default polling budget for bus events. |
| `twi0::PinRoute` | `Default` or `Alternate` PORTMUX route. |
| `twi0::Config<CpuHz, BusHz>::mbaud` | Compile-time TWI master baud value. |
| `twi0::select_pins(route)` | Selects the TWI0 PORTMUX route. |
| `twi0::init_master<CpuHz, BusHz>(route)` | Configures TWI0 master mode and marks the bus idle. |
| `twi0::write_ready()` | Returns true when the write interrupt flag is set. |
| `twi0::bus_error()` | Returns true for bus error or arbitration lost. |
| `twi0::received_nack()` | Returns true when the last address/data phase was NACKed. |
| `twi0::wait_write_ready(timeout)` | Waits for write-ready or error until timeout expires. |
| `twi0::start_write(address_7bit, timeout)` | Starts a write transaction to a 7-bit address. |
| `twi0::write_byte(value, timeout)` | Writes one data byte in an active transaction. |
| `twi0::stop()` | Issues STOP. |

Example:

```cpp
twi0::init_master<board::cpu_hz, 100000u>();
if (twi0::start_write(0x3du)) {
    twi0::write_byte(0x00u);
    twi0::write_byte(0xAFu);
}
twi0::stop();
```

## `timer0.hpp`

TCA0 single-slope periodic tick source.

| Symbol | Meaning |
| --- | --- |
| `timer0::stop()` | Disables TCA0, clears overflow state, and resets the tick counter. |
| `timer0::start_periodic_interrupt(period, prescaler)` | Starts TCA0 overflow interrupts with the requested period and clock select. |
| `timer0::tick_due()` | Returns true when the ISR has latched a foreground tick. |
| `timer0::tick_count()` | Returns the 16-bit foreground tick counter atomically. |
| `timer0::clear_tick()` | Clears the latched tick flag atomically. |
| `timer0::wait_tick()` | Busy-waits until a tick is due, then clears it. |

ISR ownership: defines `TCA0_OVF_vect`.

## `button_irq.hpp`

Active-low button interrupt service for `board::ButtonIn`.

| Symbol | Meaning |
| --- | --- |
| `button_irq::init()` | Configures `PB4` input, pull-up, falling-edge interrupt, and clears state. |
| `button_irq::pressed()` | Returns the current latched press state. |
| `button_irq::clear_pressed()` | Clears the latched press state atomically. |
| `button_irq::consume_press()` | Returns true once per latched press with no debounce policy. |
| `button_irq::consume_press_debounced(now_ticks, debounce_ticks)` | Returns true once per debounced press using a caller-owned tick source. |

ISR ownership: defines `PORTB_PORT_vect`.

## `blink_task.hpp`

Minimal foreground status LED task.

| Symbol | Meaning |
| --- | --- |
| `blink_task::init()` | Configures `board::StatusLed` as an output and drives it low. |
| `blink_task::tick()` | Toggles `board::StatusLed` once. Callers own the timing policy. |

## `oled.hpp`

SSD1306 OLED helper for the Explorer 128x64 display.

| Symbol | Meaning |
| --- | --- |
| `oled::address` | Explorer OLED 7-bit TWI address, `0x3d`. |
| `oled::width` | Display width, `128`. |
| `oled::height` | Display height, `64`. |
| `oled::page_count` | Number of 8-pixel pages, `8`. |
| `oled::init()` | Sends SSD1306 init commands and clears the display. |
| `oled::clear()` | Clears all display RAM pages. |
| `oled::write_text_page(page, text)` | Writes text to one 8-pixel page. |
Text rendering supports a compact uppercase/numeric 5x7 glyph set.

## `platform_init.hpp`

Full known-good Explorer board bring-up.

| Symbol | Meaning |
| --- | --- |
| `platform_init::run()` | Configures clock, GPIO direction, button IRQ, USART0, USART0 RX ISR, SPI0, TWI0, OLED, TCA0 tick, ADC0, boot UART text, and then enables global interrupts. |

Use this when a project links `attiny3217_explorer_platform_init` and wants the same startup sequence as the telemetry demo. For smaller firmware, initialize only the required modules.

## `app.hpp` (example-only, not part of the public API)

`app.hpp` is **not** a public library header. It lives in
`examples/telemetry_demo/app.hpp` as glue so the example's `main.cpp` can call an
application loop supplied alongside it. It is documented here only so readers of
the example can follow `main.cpp`.

| Symbol | Meaning |
| --- | --- |
| `app::run()` | Non-returning foreground loop entry point used by the telemetry example: samples sensors, services button/UART events, updates the OLED, emits telemetry, and waits for the TCA0 frame tick. |

New firmware defines its own `main()`/loop and does not depend on this header.

## `fuses.hpp`

Source-controlled fuse values used by `src/fuses.cpp`.

| Symbol | Meaning |
| --- | --- |
| `fuses::reserved` | Reserved fuse byte value, `0xff`. |
| `fuses::wdtcfg` | Watchdog fuse value. |
| `fuses::bodcfg` | Brown-out detector fuse value. |
| `fuses::osccfg` | Oscillator fuse value. |
| `fuses::tcd0cfg` | TCD0 fuse value. |
| `fuses::syscfg0` | System config 0 fuse value. |
| `fuses::syscfg1` | System config 1 fuse value. |
| `fuses::append` | Application section end fuse value. |
| `fuses::bootend` | Boot section end fuse value. |

Link `$<TARGET_OBJECTS:attiny3217_explorer_fuses>` into firmware to emit these values into the ELF `.fuse` section.

## `fast_format.hpp`

Small allocation-free formatting helpers.

| Symbol | Meaning |
| --- | --- |
| `fast_format::hex_nibble(value)` | Converts the low nibble to an uppercase ASCII hex digit. |
| `fast_format::append_hex8(out, value)` | Appends two hex digits and returns the advanced pointer. |
| `fast_format::append_hex16(out, value)` | Appends four hex digits and returns the advanced pointer. |
| `fast_format::xor_checksum(first, last)` | Computes XOR over a byte range. |

## `bit_ops.hpp`

Tiny constexpr bit helpers.

| Symbol | Meaning |
| --- | --- |
| `bit_ops::mask8<Bit>()` | Returns an 8-bit mask for `Bit`; compile-time checks `Bit < 8`. |
| `bit_ops::test(value, mask)` | Returns true if any masked bit is set. |
| `bit_ops::set(value, mask)` | Returns `value` with masked bits set. |
| `bit_ops::clear(value, mask)` | Returns `value` with masked bits cleared. |
| `bit_ops::low_byte(value)` | Returns bits 7..0 of a 16-bit value. |
| `bit_ops::high_byte(value)` | Returns bits 15..8 of a 16-bit value. |
