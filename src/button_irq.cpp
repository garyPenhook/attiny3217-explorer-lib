#include "button_irq.hpp"

#include <avr/interrupt.h>
#include <util/atomic.h>

#include "board.hpp"
#include "timer0.hpp"

namespace {

// Number of TCA0 application ticks to ignore after an accepted press. With the
// current 10 Hz timer setup this is approximately 200 ms.
constexpr uint16_t debounce_ticks = 2u;

// Set by PORTB_PORT_vect and consumed by foreground code..
volatile bool g_button_pressed = false;

// Foreground-only debounce state.
bool g_press_debounce_active = false;
uint16_t g_last_press_tick = 0u;

bool take_latched_press()
{
    bool was_pressed = false;

    // Atomically consume the ISR latch so repeated bounce edges inside the
    // debounce window cannot accumulate multiple pending foreground events.
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
    {
        was_pressed = g_button_pressed;
        g_button_pressed = false;
    }

    return was_pressed;
}

bool debounce_window_expired(uint16_t now)
{
    // Unsigned subtraction keeps working across uint16_t wraparound.
    return static_cast<uint16_t>(now - g_last_press_tick) >= debounce_ticks;
}

}  // namespace 

namespace button_irq {

void init()
{
    // Button input is on board::ButtonIn, currently PB4.
    board::ButtonIn::configure_input();

    // Enable the internal pull-up and interrupt on the falling edge, which
    // corresponds to an active-low button press.
    PORTB.PIN4CTRL = PORT_PULLUPEN_bm | static_cast<uint8_t>(PORT_ISC_FALLING_gc);

    // Clear any stale interrupt flag before the application enables interrupts.
    PORTB.INTFLAGS = PORT_INT4_bm;
    g_button_pressed = false;
    g_press_debounce_active = false;
    g_last_press_tick = 0u;
}

bool pressed()
{
    // Single-byte volatile bool read is atomic on AVR..
    return g_button_pressed;
}

void clear_pressed()
{
    // Protect against the ISR setting the flag while foreground clears it.
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
    {
        g_button_pressed = false;
    }
}

bool consume_press()
{
    // Fast path avoids disabling interrupts when no event is pending.
    if (!pressed()) {
        return false;
    }

    if (!take_latched_press()) {
        return false;
    }

    const uint16_t now = timer0::tick_count();
    if (g_press_debounce_active && !debounce_window_expired(now)) {
        return false;
    }

    g_press_debounce_active = true;
    g_last_press_tick = now;
    return true;
}

}  // namespace button_irq

ISR(PORTB_PORT_vect)
{
    // PORTB shares one vector for all pin interrupts, so check PB4 explicitly..
    if ((PORTB.INTFLAGS & PORT_INT4_bm) != 0u) {
        // Writing a one clears the latched pin interrupt flag.
        PORTB.INTFLAGS = PORT_INT4_bm;
        g_button_pressed = true;
    }
}
