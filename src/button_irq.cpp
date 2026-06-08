#include "button_irq.hpp"

#include <avr/interrupt.h>
#include <util/atomic.h>

#include "board.hpp"

namespace {

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
    // Raw event path for applications that own their own debounce strategy.
    return take_latched_press();
}

bool consume_press_debounced(uint16_t now_ticks, uint16_t debounce_ticks)
{
    // Fast path avoids disabling interrupts when no event is pending.
    if (!pressed()) {
        return false;
    }

    if (!take_latched_press()) {
        return false;
    }

    if (debounce_ticks != 0u
        && g_press_debounce_active
        && static_cast<uint16_t>(now_ticks - g_last_press_tick) < debounce_ticks) {
        return false;
    }

    g_press_debounce_active = true;
    g_last_press_tick = now_ticks;
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
