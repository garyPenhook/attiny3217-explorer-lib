#pragma once

#include <stdint.h>

// Falling-edge button interrupt service. The ISR records a press event and the
// foreground loop consumes it without doing UART work inside the ISR.
namespace button_irq {

// Configure the button pin, pull-up, interrupt sense control, and event flag.
void init();

// Return the current latched press state without clearing it.
bool pressed();

// Clear the latched press state atomically.
void clear_pressed();

// Return true once per latched press with no debounce policy.
bool consume_press();

// Return true once per debounced press using a caller-owned tick source.
bool consume_press_debounced(uint16_t now_ticks, uint16_t debounce_ticks = 2u);

}  // namespace button_irq
