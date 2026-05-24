#pragma once

#include <stdint.h>

// Interrupt-driven USART0 receive ring buffer.
namespace uart0_rx {

// Reset buffer indices and enable the receive-complete interrupt.
void init();

// Return true when at least one received byte is buffered.
bool available();

// Pop one received byte if available.
bool read_byte(uint8_t& value);

}  // namespace uart0_rx
