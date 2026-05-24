#pragma once

// Platform bring-up owns one-time hardware configuration before app::run().
namespace platform_init {

// Configure clocks, GPIO, peripherals, timer, ADC, startup UART text, and
// finally global interrupts.
void run();

}  // namespace platform_init
