#pragma once

#include <avr/io.h>
#include <stdint.h>

// SPI0 master helper. The default route matches the board aliases in board.hpp.
namespace spi0 {

constexpr uint8_t reg8(auto value)
{
    // Convert DFP enum and bit-mask values to raw register bytes.
    return static_cast<uint8_t>(value);
}

// PORTMUX route selection for SPI0 pins.
enum class PinRoute : uint8_t {
    Default,
    Alternate,
};

static inline void select_pins(PinRoute route)
{
    // PORTMUX.CTRLB controls whether SPI0 uses default or alternate pins.
    if (route == PinRoute::Alternate) {
        PORTMUX.CTRLB |= PORTMUX_SPI0_bm;
    } else {
        PORTMUX.CTRLB &= static_cast<uint8_t>(~PORTMUX_SPI0_bm);
    }
}

static inline void init_master(
    SPI_PRESC_t prescaler = SPI_PRESC_DIV16_gc,
    SPI_MODE_t mode = SPI_MODE_0_gc,
    bool double_speed = false,
    PinRoute route = PinRoute::Default)
{
    // Route pins before enabling the peripheral so signals appear in the
    // intended location as soon as SPI0 starts driving.
    select_pins(route);

    // Enable SPI0 as master and select clock rate. SPI_CLK2X halves the
    // effective prescaler when double_speed is true.
    SPI0.CTRLA = reg8(SPI_ENABLE_bm)
                 | reg8(SPI_MASTER_bm)
                 | reg8(prescaler)
                 | (double_speed ? reg8(SPI_CLK2X_bm) : 0u);

    // Mode selects clock polarity/phase. SSD keeps master mode stable if SS is
    // configured as an input elsewhere.
    SPI0.CTRLB = reg8(mode) | reg8(SPI_SSD_bm);
}

static inline uint8_t transfer(uint8_t value)
{
    // Writing DATA starts the full-duplex transfer.
    SPI0.DATA = value;

    // Wait for receive-complete; the received byte is valid in DATA then.
    while ((SPI0.INTFLAGS & SPI_RXCIF_bm) == 0u) {
    }
    return SPI0.DATA;
}

}  // namespace spi0
