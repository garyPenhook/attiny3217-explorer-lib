#pragma once

#include <avr/io.h>
#include <stdint.h>

// USART0 transmit helper. Receive buffering is implemented separately in
// uart0_rx so foreground code can opt into the ISR-backed path.
namespace uart0 {

constexpr uint8_t reg8(auto value)
{
    // Convert DFP enum and bit-mask values to raw register bytes.
    return static_cast<uint8_t>(value);
}

// PORTMUX route selection for USART0 pins.
enum class PinRoute : uint8_t {
    Default,
    Alternate,
};

template<uint32_t CpuHz, uint32_t Baud>
struct Config {
    // Avoid division by zero in the constexpr baud register calculation.
    static_assert(Baud != 0u, "Baud must be non-zero");

    // Normal asynchronous mode baud formula for tinyAVR 1-series USART.
    static constexpr uint16_t baud_reg =
        static_cast<uint16_t>((4u * CpuHz) / Baud);
};

static inline void select_pins(PinRoute route)
{
    // PORTMUX.CTRLB controls whether USART0 uses default or alternate pins.
    if (route == PinRoute::Alternate) {
        PORTMUX.CTRLB |= PORTMUX_USART0_bm;
    } else {
        PORTMUX.CTRLB &= static_cast<uint8_t>(~PORTMUX_USART0_bm);
    }
}

template<uint32_t CpuHz, uint32_t Baud>
static inline void init(PinRoute route = PinRoute::Default)
{
    // Route pins before enabling TX/RX.
    select_pins(route);

    // Set baud, 8N1 asynchronous frame format, and enable transmitter/receiver.
    USART0.BAUD = Config<CpuHz, Baud>::baud_reg;
    USART0.CTRLC = reg8(USART_CMODE_ASYNCHRONOUS_gc)
                 | reg8(USART_PMODE_DISABLED_gc)
                 | reg8(USART_CHSIZE_8BIT_gc);
    USART0.CTRLB = reg8(USART_RXMODE_NORMAL_gc)
                 | reg8(USART_TXEN_bm)
                 | reg8(USART_RXEN_bm);
}

static inline void write_byte(uint8_t value)
{
    // DREIF indicates TXDATAL can accept a new byte.
    while ((USART0.STATUS & USART_DREIF_bm) == 0u) {
    }
    USART0.TXDATAL = value;
}

static inline void write_cstr(const char* text)
{
    // Send a NUL-terminated string from RAM or flash-mapped data.
    while (*text != '\0') {
        write_byte(static_cast<uint8_t>(*text++));
    }
}

static inline void write_buffer(const char* data, uint8_t size)
{
    // Send exactly size bytes; no terminator is required.
    const char* end = data + size;
    while (data != end) {
        write_byte(static_cast<uint8_t>(*data++));
    }
}

}  // namespace uart0
