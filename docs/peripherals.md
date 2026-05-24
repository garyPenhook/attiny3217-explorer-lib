# Peripheral Notes

## Clock

`clock::use_internal_20mhz_no_prescaler()` selects the internal 20 MHz oscillator and disables the main prescaler. `F_CPU` is expected to match `board::cpu_hz`, which defaults to `20000000UL`.

## GPIO

`gpio::Pin<PortId, Bit>` resolves port operations at compile time and uses `DIRSET`, `DIRCLR`, `OUTSET`, `OUTCLR`, and `OUTTGL` to avoid read-modify-write GPIO hazards.

## USART0

`uart0::init<CpuHz, Baud>()` configures 8N1 transmit and receive. `uart0_rx::init()` enables the receive-complete ISR and provides a 32-byte power-of-two ring buffer.

Interrupt vector used: `USART0_RXC_vect`.

## Timer

`timer0::start_periodic_interrupt(period, prescaler)` uses TCA0 single-slope overflow as a foreground tick source.

Interrupt vector used: `TCA0_OVF_vect`.

The telemetry demo uses `PER=31249` and `DIV64` for 10 Hz at 20 MHz.

## Button

`button_irq::init()` configures `PB4` as an input with pull-up and falling-edge pin interrupt. Foreground code should call `button_irq::consume_press()` instead of doing work in the ISR.

Interrupt vector used: `PORTB_PORT_vect`.

## TWI0/I2C

`twi0::init_master<CpuHz, BusHz>()` configures TWI0 master mode and sets the bus idle. Write helpers use bounded polling so a missing device does not hang forever.

The Explorer SSD1306 OLED address is `0x3d`. The telemetry demo also probes the MCP9808 address `0x1c`.

## SPI0

`spi0::init_master()` configures SPI0 in master mode on the default route. `spi0::transfer()` is a blocking full-duplex byte transfer.

## ADC0

`adc0::init_vdd()` uses VDD as the ADC reference. `analog_sensor::sample()` reads `ADC_MUXPOS_AIN7_gc`; adjust `src/analog_sensor.cpp` if a later project uses a different analog input.

## OLED

`oled::init()` sends the SSD1306 setup sequence for the Explorer 128x64 panel and clears display RAM. Text rendering is intentionally small: 5x7 uppercase glyphs, one 8-pixel page at a time.
