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

    // Configure board-owned pins before their peripherals start driving them.
    board::StatusLed::configure_output();
    board::StatusLed::set_low();
    board::Usart0Tx::configure_output();
    board::Usart0Rx::configure_input();
    board::Twi0Scl::configure_input();
    board::Twi0Sda::configure_input();
    board::Spi0Mosi::configure_output();
    board::Spi0Sck::configure_output();
    board::Spi0Miso::configure_input();

    // Prepare the button interrupt before global interrupts are enabled.
    button_irq::init();

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
    uart0::write_cstr("boot\r\n");

    // Enable ISRs only after all interrupt sources have been configured and
    // stale flags have been cleared.
    sei();
}

}  // namespace platform_init
