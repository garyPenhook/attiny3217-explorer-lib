#pragma once

#include <avr/io.h>
#include <stdint.h>

// TCA0 single-slope timer wrapper used as the application frame tick.
namespace timer0 {

constexpr uint8_t reg8(auto value)
{
    // Convert DFP enum and bit-mask values to raw register bytes.
    return static_cast<uint8_t>(value);
}

// Disable the timer and clear pending state.
void stop();

// Start TCA0 with overflow interrupts after PER reaches the requested period.
void start_periodic_interrupt(uint16_t period, TCA_SINGLE_CLKSEL_t prescaler);

// Return true if the overflow ISR has latched a tick.
bool tick_due();

// Return the free-running foreground tick counter.
uint16_t tick_count();

// Clear the latched tick flag atomically.
void clear_tick();

// Busy-wait until the next latched tick and consume it.
void wait_tick();

}  // namespace timer0
