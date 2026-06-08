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

inline char* append_decimal_u16(char* out, uint16_t value)
{
    // Emit decimal digits without leading zeros (except a lone "0"). The output
    // is at most five digits for a 16-bit value.
    constexpr uint16_t divisors[] = {10000u, 1000u, 100u, 10u, 1u};
    bool started = false;

    for (uint8_t i = 0u; i < static_cast<uint8_t>(sizeof(divisors) / sizeof(divisors[0])); ++i) {
        const uint16_t divisor = divisors[i];
        const uint8_t digit = static_cast<uint8_t>(value / divisor);
        if (digit != 0u || started) {
            *out++ = static_cast<char>('0' + digit);
            started = true;
        }
        value = static_cast<uint16_t>(value % divisor);
    }

    if (!started) {
        *out++ = '0';
    }

    return out;
}

}  // namespace fast_format
