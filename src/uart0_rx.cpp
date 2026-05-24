#include "uart0_rx.hpp"

#include <avr/interrupt.h>
#include <util/atomic.h>

#include <avr/io.h>

namespace {

// Power-of-two ring size allows wraparound with a cheap bit mask.
constexpr uint8_t rx_buffer_size = 32u;
constexpr uint8_t rx_buffer_mask = rx_buffer_size - 1u;
static_assert((rx_buffer_size & rx_buffer_mask) == 0u, "RX buffer size must be power-of-two");

// Ring storage and indices are shared with the USART RX ISR.
volatile uint8_t rx_buffer[rx_buffer_size] = {};
volatile uint8_t rx_head = 0u;
volatile uint8_t rx_tail = 0u;

uint8_t next_index(uint8_t index)
{
    // Wrap index into [0, rx_buffer_size) using the power-of-two mask.
    return static_cast<uint8_t>((index + 1u) & rx_buffer_mask);
}

}  // namespace

namespace uart0_rx {

void init()
{
    // Drop any stale bytes and enable receive-complete interrupts.
    rx_head = 0u;
    rx_tail = 0u;
    USART0.CTRLA |= USART_RXCIE_bm;
}

bool available()
{
    // The buffer is non-empty when producer and consumer indices differ.
    return rx_head != rx_tail;
}

bool read_byte(uint8_t& value)
{
    // Fast path avoids an atomic block when the buffer is empty.
    if (!available()) {
        return false;
    }

    // Keep head/tail comparison and tail update indivisible relative to RX ISR.
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
    {
        if (rx_head == rx_tail) {
            return false;
        }

        value = rx_buffer[rx_tail];
        rx_tail = next_index(rx_tail);
    }

    return true;
}

}  // namespace uart0_rx

ISR(USART0_RXC_vect)
{
    // Read status before data as required by the USART receive sequence.
    const uint8_t status = USART0.RXDATAH;
    const uint8_t data = USART0.RXDATAL;

    // Drop bytes with overflow, framing, or parity errors.
    if ((status & (USART_BUFOVF_bm | USART_FERR_bm | USART_PERR_bm)) != 0u) {
        return;
    }

    // Leave one empty slot so head == tail can represent an empty buffer.
    const uint8_t next_head = next_index(rx_head);
    if (next_head == rx_tail) {
        return;
    }

    // Store the received byte and publish the new producer index last.
    rx_buffer[rx_head] = data;
    rx_head = next_head;
}
