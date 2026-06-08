#include "analog_sensor.hpp"

#include <avr/io.h>

#include "adc0.hpp"
#include "adc_scale.hpp"

namespace analog_sensor {

void init()
{
    // Configure ADC0 for VDD-referenced single conversions. The reference value
    // assumed during scaling is board::analog_sensor_vref_mv.
    adc0::init_vdd();
}

uint16_t sample(ADC_MUXPOS_t channel)
{
    // Channel defaults to the board analog input but is caller-overridable.
    return adc0::read_blocking(channel);
}

uint16_t to_millivolts(uint16_t raw_sample, uint16_t vref_mv)
{
    // 10-bit ADC: full scale is 1024 counts. Conversion policy lives in the
    // host-testable adc_scale helper.
    return adc_scale::to_millivolts(raw_sample, vref_mv, 1024u);
}

uint16_t sample_millivolts()
{
    // Convenience wrapper for foreground code that does not need the raw count.
    return to_millivolts(sample());
}

}  // namespace analog_sensor
