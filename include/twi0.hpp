#pragma once

#include <avr/io.h>
#include <stdint.h>

// TWI0/I2C master helper. Transactions are blocking with a bounded polling
// budget so a missing or stuck device cannot hang the foreground loop forever.
//
// Concurrency: foreground-only. None of these functions are ISR-safe; they busy
// poll MSTATUS and must run with the bus owned by the calling context.
namespace twi0 {

// Default polling budget for bus events. This is intentionally a loop count,
// not a wall-clock duration, to keep the helper dependency-free.
inline constexpr uint16_t default_timeout_iterations = 60000u;

// Result of a TWI transaction step. Prefer this over a bare bool so callers can
// distinguish a missing device (nack) from electrical/bus faults or a timeout.
enum class Status : uint8_t {
    ok,           // step completed and was acknowledged
    timeout,      // polling budget elapsed before completion
    nack,         // address or data byte was not acknowledged
    bus_error,    // illegal bus condition detected
    arb_lost,     // lost arbitration to another master
    invalid_arg,  // caller passed an out-of-range argument
};

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

    // ATtiny 1-series master baud formula from the device datasheet:
    //   MBAUD = (CpuHz / BusHz - 10) / 2
    // Guard the subtraction so an unreachably fast bus does not underflow.
    static_assert(CpuHz / BusHz >= 10u, "CpuHz/BusHz must be >= 10 for a valid MBAUD");
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

// --- Low-level status predicates (foreground-only) -------------------------

static inline bool write_ready()
{
    // WIF is set when the address or data byte phase completed.
    return (TWI0.MSTATUS & TWI_WIF_bm) != 0u;
}

static inline bool read_ready()
{
    // RIF is set when a received byte is available in MDATA.
    return (TWI0.MSTATUS & TWI_RIF_bm) != 0u;
}

static inline bool received_nack()
{
    // RXACK set means the addressed device or byte receiver did not ACK.
    return (TWI0.MSTATUS & TWI_RXACK_bm) != 0u;
}

static inline Status classify_bus_fault(uint8_t status)
{
    // Arbitration loss is reported before a generic bus error so the caller can
    // tell a multi-master collision apart from an electrical fault.
    if ((status & TWI_ARBLOST_bm) != 0u) {
        return Status::arb_lost;
    }
    if ((status & TWI_BUSERR_bm) != 0u) {
        return Status::bus_error;
    }
    return Status::ok;
}

[[nodiscard]] static inline Status wait_write_ready(uint16_t timeout_iterations = default_timeout_iterations)
{
    // Poll for the write-complete flag while also watching fatal bus faults.
    while (timeout_iterations-- != 0u) {
        const uint8_t status = TWI0.MSTATUS;
        const Status fault = classify_bus_fault(status);
        if (fault != Status::ok) {
            return fault;
        }
        if ((status & TWI_WIF_bm) != 0u) {
            return Status::ok;
        }
    }

    return Status::timeout;
}

// --- Transaction API -------------------------------------------------------

// Issue START (or repeated START if the bus is already owned) and send the
// address for a write. Returns ok only when the address was acknowledged.
[[nodiscard]] static inline Status start_write(uint8_t address_7bit, uint16_t timeout_iterations = default_timeout_iterations)
{
    if (address_7bit > 0x7fu) {
        return Status::invalid_arg;
    }

    // The hardware expects the 7-bit address shifted left with bit 0 clear for
    // write transactions.
    TWI0.MADDR = static_cast<uint8_t>(address_7bit << 1);
    const Status status = wait_write_ready(timeout_iterations);
    if (status != Status::ok) {
        return status;
    }

    return received_nack() ? Status::nack : Status::ok;
}

// Write one data byte in the current write transaction.
[[nodiscard]] static inline Status write_byte(uint8_t value, uint16_t timeout_iterations = default_timeout_iterations)
{
    // Loading MDATA starts the next data-byte phase.
    TWI0.MDATA = value;
    const Status status = wait_write_ready(timeout_iterations);
    if (status != Status::ok) {
        return status;
    }

    return received_nack() ? Status::nack : Status::ok;
}

// Write a buffer in the current write transaction. Stops at the first failure.
[[nodiscard]] static inline Status write_bytes(const uint8_t* data, uint8_t size, uint16_t timeout_iterations = default_timeout_iterations)
{
    if (size != 0u && data == nullptr) {
        return Status::invalid_arg;
    }

    for (uint8_t i = 0u; i < size; ++i) {
        const Status status = write_byte(data[i], timeout_iterations);
        if (status != Status::ok) {
            return status;
        }
    }

    return Status::ok;
}

// Issue START (or repeated START) and send the address for a read. Returns ok
// once the addressed device has acknowledged and the first byte is clocking in.
[[nodiscard]] static inline Status start_read(uint8_t address_7bit, uint16_t timeout_iterations = default_timeout_iterations)
{
    if (address_7bit > 0x7fu) {
        return Status::invalid_arg;
    }

    // Address with bit 0 set requests a read transaction.
    TWI0.MADDR = static_cast<uint8_t>((address_7bit << 1) | 1u);

    while (timeout_iterations-- != 0u) {
        const uint8_t status = TWI0.MSTATUS;
        const Status fault = classify_bus_fault(status);
        if (fault != Status::ok) {
            return fault;
        }
        // WIF set after the address byte: a NACK here means no such device.
        if ((status & TWI_WIF_bm) != 0u && (status & TWI_RXACK_bm) != 0u) {
            return Status::nack;
        }
        // RIF set means the first data byte has been received.
        if ((status & TWI_RIF_bm) != 0u) {
            return Status::ok;
        }
    }

    return Status::timeout;
}

// Read one byte in the current read transaction. When ack is true the controller
// ACKs and clocks in the next byte; when false it NACKs and issues STOP, ending
// the transaction (so the caller must not also call stop()).
[[nodiscard]] static inline Status read_byte(uint8_t& out, bool ack, uint16_t timeout_iterations = default_timeout_iterations)
{
    while ((TWI0.MSTATUS & TWI_RIF_bm) == 0u) {
        const Status fault = classify_bus_fault(TWI0.MSTATUS);
        if (fault != Status::ok) {
            return fault;
        }
        if (timeout_iterations-- == 0u) {
            return Status::timeout;
        }
    }

    // Reading MDATA returns the byte and clears RIF.
    out = TWI0.MDATA;

    if (ack) {
        // ACK and receive the next byte.
        TWI0.MCTRLB = reg8(TWI_MCMD_RECVTRANS_gc);
    } else {
        // NACK the byte and release the bus with a STOP.
        TWI0.MCTRLB = reg8(TWI_ACKACT_bm) | reg8(TWI_MCMD_STOP_gc);
    }

    return Status::ok;
}

static inline void stop()
{
    // Issue STOP to release the bus after the transaction.
    TWI0.MCTRLB = reg8(TWI_MCMD_STOP_gc);
}

// --- High-level register/buffer helpers ------------------------------------

// Write data to a device: START, address, payload, STOP.
[[nodiscard]] static inline Status write(uint8_t address_7bit, const uint8_t* data, uint8_t size, uint16_t timeout_iterations = default_timeout_iterations)
{
    Status status = start_write(address_7bit, timeout_iterations);
    if (status == Status::ok) {
        status = write_bytes(data, size, timeout_iterations);
    }
    stop();
    return status;
}

// Write a single device register: START, address, reg, value, STOP.
[[nodiscard]] static inline Status write_register(uint8_t address_7bit, uint8_t reg, uint8_t value, uint16_t timeout_iterations = default_timeout_iterations)
{
    Status status = start_write(address_7bit, timeout_iterations);
    if (status == Status::ok) {
        status = write_byte(reg, timeout_iterations);
    }
    if (status == Status::ok) {
        status = write_byte(value, timeout_iterations);
    }
    stop();
    return status;
}

// Read a block from a device register using repeated start:
// START, address(write), reg, repeated START, address(read), data..., STOP.
[[nodiscard]] static inline Status read_register(uint8_t address_7bit, uint8_t reg, uint8_t* out, uint8_t size, uint16_t timeout_iterations = default_timeout_iterations)
{
    if (size == 0u || out == nullptr) {
        return Status::invalid_arg;
    }

    Status status = start_write(address_7bit, timeout_iterations);
    if (status != Status::ok) {
        stop();
        return status;
    }

    status = write_byte(reg, timeout_iterations);
    if (status != Status::ok) {
        stop();
        return status;
    }

    status = start_read(address_7bit, timeout_iterations);
    if (status != Status::ok) {
        stop();
        return status;
    }

    for (uint8_t i = 0u; i < size; ++i) {
        const bool last = (i + 1u) == size;
        // read_byte with ack=false issues the closing STOP itself.
        status = read_byte(out[i], !last, timeout_iterations);
        if (status != Status::ok) {
            if (!last) {
                stop();
            }
            return status;
        }
    }

    return Status::ok;
}

// Convenience single-register read.
[[nodiscard]] static inline Status read_register(uint8_t address_7bit, uint8_t reg, uint8_t& out, uint16_t timeout_iterations = default_timeout_iterations)
{
    return read_register(address_7bit, reg, &out, 1u, timeout_iterations);
}

}  // namespace twi0
