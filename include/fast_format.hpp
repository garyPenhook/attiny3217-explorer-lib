#pragma once

#include <stdint.h>

#include "bit_ops.hpp"

// Allocation-free formatting helpers for small telemetry frames. These avoid
// stdio and heap use on the AVR target.
namespace fast_format {

constexpr char hex_nibble(uint8_t value)
{
    // Keep only the low nibble and map it to an uppercase ASCII hex digit.
    value &= 0x0Fu;
    return static_cast<char>(value < 10u ? ('0' + value) : ('A' + (value - 10u)));
}

inline char* append_hex8(char* out, uint8_t value)
{
    // Emit most-significant nibble first so the output reads normally.
    *out++ = hex_nibble(static_cast<uint8_t>(value >> 4));
    *out++ = hex_nibble(value);
    return out;
}

inline char* append_hex16(char* out, uint16_t value)
{
    // Emit high byte first, then low byte.
    out = append_hex8(out, bit_ops::high_byte(value));
    out = append_hex8(out, bit_ops::low_byte(value));
    return out;
}

inline uint8_t xor_checksum(const uint8_t* first, const uint8_t* last)
{
    // Simple XOR checksum over [first, last). Used for tiny frames where
    // corruption detection matters more than cryptographic strength.
    uint8_t checksum = 0u;
    while (first != last) {
        checksum ^= *first++;
    }
    return checksum;
}

}  // namespace fast_format
