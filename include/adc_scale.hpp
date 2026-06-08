#pragma once

#include <stdint.h>

// Pure ADC conversion math, free of any AVR register dependency so it can be
// unit tested on the host and reused for any reference/resolution.
namespace adc_scale {

// Convert a raw ADC count to millivolts against a reference voltage.
//   millivolts = round(raw * vref_mv / full_scale_counts)
// full_scale_counts is 2^resolution (1024 for the ATtiny3217's 10-bit ADC).
// 32-bit intermediate math keeps raw * vref_mv from overflowing.
constexpr uint16_t to_millivolts(uint16_t raw, uint16_t vref_mv, uint16_t full_scale_counts = 1024u)
{
    if (full_scale_counts == 0u) {
        return 0u;
    }

    const uint32_t scaled = static_cast<uint32_t>(raw) * static_cast<uint32_t>(vref_mv);
    const uint32_t half_lsb = static_cast<uint32_t>(full_scale_counts) / 2u;
    return static_cast<uint16_t>((scaled + half_lsb) / full_scale_counts);
}

}  // namespace adc_scale
