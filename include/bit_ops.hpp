#pragma once

#include <stdint.h>

// constexpr bit helpers used where direct register masks are clearer than
// runtime arithmetic at the call site.
namespace bit_ops {

template<uint8_t Bit>
constexpr uint8_t mask8()
{
    // Catch invalid bit numbers during compilation, before they can become
    // accidental shifts with undefined behavior.
    static_assert(Bit < 8u, "Bit index must be 0..7");
    return static_cast<uint8_t>(1u << Bit);
}

constexpr bool test(uint8_t value, uint8_t mask)
{
    // True when any bit in mask is set in value.
    return (value & mask) != 0u;
}

constexpr uint8_t set(uint8_t value, uint8_t mask)
{
    // Return value with all bits from mask forced high.
    return static_cast<uint8_t>(value | mask);
}

constexpr uint8_t clear(uint8_t value, uint8_t mask)
{
    // Return value with all bits from mask forced low.
    return static_cast<uint8_t>(value & static_cast<uint8_t>(~mask));
}

constexpr uint8_t low_byte(uint16_t value)
{
    // Extract bits 7..0.
    return static_cast<uint8_t>(value);
}

constexpr uint8_t high_byte(uint16_t value)
{
    // Extract bits 15..8.
    return static_cast<uint8_t>(value >> 8);
}

}  // namespace bit_ops
