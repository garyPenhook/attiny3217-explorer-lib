#pragma once

// Platform bring-up owns one-time hardware configuration before the foreground
// application loop starts.
namespace platform_init {

// Configure clocks, GPIO, peripherals, timer, ADC, startup UART text, and
// finally global interrupts.
void run();

}  // namespace platform_init
