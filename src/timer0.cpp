#include "timer0.hpp"

#include <avr/interrupt.h>
#include <util/atomic.h>

namespace {

// Latched by TCA0_OVF_vect and consumed by the foreground loop.
volatile bool g_tick_due = false;

// Incremented by TCA0_OVF_vect and used as a simple monotonic timebase.
volatile uint16_t g_tick_count = 0u;

}  // namespace

namespace timer0 {

void stop()
{
    // Disable TCA0 and its overflow interrupt before changing timer state.
    TCA0.SINGLE.CTRLA = 0u;
    TCA0.SINGLE.INTCTRL = 0u;

    // Clear any pending overflow flag and reset the foreground latch.
    TCA0.SINGLE.INTFLAGS = TCA_SINGLE_OVF_bm;
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
    {
        g_tick_due = false;
        g_tick_count = 0u;
    }
}

void start_periodic_interrupt(uint16_t period, TCA_SINGLE_CLKSEL_t prescaler)
{
    // Start from a known disabled state to avoid partial timer reconfiguration.
    stop();

    // Normal single-slope counting from 0 to PER.
    TCA0.SINGLE.CTRLD = 0u;
    TCA0.SINGLE.CNT = 0u;
    TCA0.SINGLE.PER = period;
    TCA0.SINGLE.CTRLB = 0u;

    // Enable overflow interrupt, then enable the timer with the selected clock.
    TCA0.SINGLE.INTCTRL = TCA_SINGLE_OVF_bm;
    TCA0.SINGLE.CTRLA = reg8(TCA_SINGLE_ENABLE_bm) | reg8(prescaler);
}

bool tick_due()
{
    // Single-byte volatile bool read is atomic on AVR.
    return g_tick_due;
}

uint16_t tick_count()
{
    uint16_t count = 0u;

    // A 16-bit read is not atomic on 8-bit AVR, so protect it from the ISR.
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
    {
        count = g_tick_count;
    }

    return count;
}

void clear_tick()
{
    // Protect against the ISR setting the flag while foreground clears it.
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
    {
        g_tick_due = false;
    }
}

void wait_tick()
{
    // Busy-wait is acceptable because this is the foreground pacing point.
    while (!tick_due()) {
    }
    clear_tick();
}

}  // namespace timer0

ISR(TCA0_OVF_vect)
{
    // Writing a one clears the TCA0 overflow interrupt flag.
    TCA0.SINGLE.INTFLAGS = TCA_SINGLE_OVF_bm;

    // Latch one application tick for foreground code.
    g_tick_due = true;
    g_tick_count = static_cast<uint16_t>(g_tick_count + 1u);
}
