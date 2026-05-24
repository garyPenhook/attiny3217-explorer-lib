#include "platform_init.hpp"

#include <avr/interrupt.h>

#include "analog_sensor.hpp"
#include "board.hpp"
#include "button_irq.hpp"
#include "clock.hpp"
#include "oled.hpp"
#include "spi0.hpp"
#include "timer0.hpp"
#include "twi0.hpp"
#include "uart0.hpp"
#include "uart0_rx.hpp"

namespace platform_init {

void run()
{
    // Match the runtime clock to board::cpu_hz/F_CPU.
    clock::use_internal_20mhz_no_prescaler();

    // Prepare the button interrupt before global interrupts are enabled.
    button_irq::init();

    // PB5=LED, PB2=USART0 TX as outputs.
    PORTB.DIRSET = static_cast<uint8_t>((1u << 5) | (1u << 2));
    PORTB.OUTCLR = static_cast<uint8_t>(1u << 5);
    // PB3=USART0 RX, PB1/PB0=TWI inputs.
    PORTB.DIRCLR = static_cast<uint8_t>((1u << 3) | (1u << 1) | (1u << 0));
    // PA4=SPI MOSI, PA3=SPI SCK outputs; PA5=SPI MISO input.
    PORTA.DIRSET = static_cast<uint8_t>((1u << 4) | (1u << 3));
    PORTA.DIRCLR = static_cast<uint8_t>(1u << 5);

    // Bring up serial receive/transmit first so startup text and later telemetry
    // have a configured UART.
    uart0::init<board::cpu_hz, 115200u>();
    uart0_rx::init();

    // Configure synchronous buses on their default PORTMUX routes.
    spi0::init_master();
    twi0::init_master<board::cpu_hz, 100000u>();
    oled::init();

    // 20 MHz / 64 / (31249 + 1) = 10 Hz foreground pacing.
    timer0::start_periodic_interrupt(31249u, TCA_SINGLE_CLKSEL_DIV64_gc);

    // ADC setup is last because it does not affect startup diagnostics.
    analog_sensor::init();

    // Emit a compact boot marker on USART0.
    uart0::write_byte('b');
    uart0::write_byte('o');
    uart0::write_byte('o');
    uart0::write_byte('t');
    uart0::write_byte('\r');
    uart0::write_byte('\n');

    // Enable ISRs only after all interrupt sources have been configured and
    // stale flags have been cleared.
    sei();
}

}  // namespace platform_init
