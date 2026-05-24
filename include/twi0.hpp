#pragma once

#include <avr/io.h>
#include <stdint.h>

// TWI0/I2C master helper. It provides blocking byte writes with bounded waits
// so a missing device does not hang the foreground loop forever.
namespace twi0 {

// Default polling budget for bus events. This is intentionally a loop count,
// not a wall-clock duration, to keep the helper dependency-free.
inline constexpr uint16_t default_timeout_iterations = 60000u;

constexpr uint8_t reg8(auto value)
{
    // Convert DFP enum and bit-mask values to raw register bytes.
    return static_cast<uint8_t>(value);
}

// PORTMUX route selection for TWI0 pins.
enum class PinRoute : uint8_t {
    Default,
    Alternate,
};

static inline void select_pins(PinRoute route)
{
    // PORTMUX.CTRLB controls whether TWI0 uses default or alternate pins.
    if (route == PinRoute::Alternate) {
        PORTMUX.CTRLB |= PORTMUX_TWI0_bm;
    } else {
        PORTMUX.CTRLB &= static_cast<uint8_t>(~PORTMUX_TWI0_bm);
    }
}

template<uint32_t CpuHz, uint32_t BusHz>
struct Config {
    // Avoid division by zero in the constexpr MBAUD calculation.
    static_assert(BusHz != 0u, "Bus frequency must be non-zero");

    // ATtiny 1-series master baud formula from the device datasheet.
    static constexpr uint8_t mbaud =
        static_cast<uint8_t>(((CpuHz / BusHz) - 10u) / 2u);
};

template<uint32_t CpuHz, uint32_t BusHz>
static inline void init_master(PinRoute route = PinRoute::Default)
{
    // Route pins before enabling the peripheral.
    select_pins(route);

    // Program bus speed, enable master mode, and mark the bus idle so the first
    // transaction can start.
    TWI0.MBAUD = Config<CpuHz, BusHz>::mbaud;
    TWI0.MCTRLA = reg8(TWI_ENABLE_bm);
    TWI0.MSTATUS = reg8(TWI_BUSSTATE_IDLE_gc);
}

static inline bool write_ready()
{
    // WIF is set when the address or data byte phase completed.
    return (TWI0.MSTATUS & TWI_WIF_bm) != 0u;
}

static inline bool bus_error()
{
    // Treat bus errors and arbitration loss as failed transfers.
    return (TWI0.MSTATUS & (TWI_BUSERR_bm | TWI_ARBLOST_bm)) != 0u;
}

static inline bool received_nack()
{
    // RXACK set means the addressed device or byte receiver did not ACK.
    return (TWI0.MSTATUS & TWI_RXACK_bm) != 0u;
}

static inline bool wait_write_ready(uint16_t timeout_iterations = default_timeout_iterations)
{
    // Poll for the write-complete flag while also watching fatal bus errors.
    while (timeout_iterations-- != 0u) {
        if (write_ready()) {
            return true;
        }
        if (bus_error()) {
            return false;
        }
    }

    return false;
}

static inline bool start_write(uint8_t address_7bit, uint16_t timeout_iterations = default_timeout_iterations)
{
    // The hardware expects the 7-bit address shifted left with bit 0 clear for
    // write transactions.
    TWI0.MADDR = static_cast<uint8_t>(address_7bit << 1);
    if (!wait_write_ready(timeout_iterations)) {
        return false;
    }

    // A valid address phase must complete without NACK or bus error.
    return !received_nack() && !bus_error();
}

static inline bool write_byte(uint8_t value, uint16_t timeout_iterations = default_timeout_iterations)
{
    // Loading MDATA starts the next data-byte phase.
    TWI0.MDATA = value;
    if (!wait_write_ready(timeout_iterations)) {
        return false;
    }

    // A valid data phase must complete without NACK or bus error.
    return !received_nack() && !bus_error();
}

static inline void stop()
{
    // Issue STOP to release the bus after the transaction.
    TWI0.MCTRLB = reg8(TWI_MCMD_STOP_gc);
}

}  // namespace twi0
