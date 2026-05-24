#include "analog_sensor.hpp"

#include <avr/io.h>

#include "adc0.hpp"

namespace analog_sensor {

namespace {

// The analog input is currently measured against VDD, assumed to be 5 V.
constexpr uint32_t vref_mv = 5000u;

// Fixed-point scale factor precision for millivolt conversion.
constexpr uint8_t scale_shift = 10u;

// Q10 millivolts-per-count factor for a 10-bit ADC result.
constexpr uint32_t mv_per_count_q10 = (vref_mv << scale_shift) / 1024u;

// Half an LSB in Q10 space, used to round instead of truncate.
constexpr uint32_t scale_rounding = 1u << (scale_shift - 1u);

uint16_t scale_to_millivolts(uint16_t raw)
{
    // Use 32-bit math so raw * scale cannot overflow before shifting back down.
    const uint32_t scaled = static_cast<uint32_t>(raw) * mv_per_count_q10;
    return static_cast<uint16_t>((scaled + scale_rounding) >> scale_shift);
}

}  // namespace

void init()
{
    // Configure ADC0 for VDD-referenced single conversions.
    adc0::init_vdd();
}

uint16_t sample()
{
    // AIN7 is the board-level analog sensor input for this firmware.
    return adc0::read_blocking(ADC_MUXPOS_AIN7_gc);
}

uint16_t to_millivolts(uint16_t raw_sample)
{
    // Keep conversion policy in one place for callers with cached raw samples.
    return scale_to_millivolts(raw_sample);
}

uint16_t sample_millivolts()
{
    // Convenience wrapper for foreground code that does not need the raw count.
    return scale_to_millivolts(sample());
}

}  // namespace analog_sensor
