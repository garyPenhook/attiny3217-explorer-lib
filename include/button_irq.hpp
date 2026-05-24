#pragma once

// Falling-edge button interrupt service. The ISR records a press event and the
// foreground loop consumes it without doing UART work inside the ISR.
namespace button_irq {

// Configure the button pin, pull-up, interrupt sense control, and event flag.
void init();

// Return the current latched press state without clearing it.
bool pressed();

// Clear the latched press state atomically.
void clear_pressed();

// Return true once per latched press and clear the flag.
bool consume_press();

}  // namespace button_irq
