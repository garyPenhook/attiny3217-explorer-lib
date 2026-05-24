#pragma once

#include <stdint.h>

// Board-level analog input wrapper. The implementation currently samples ADC0
// channel AIN7 and converts that raw count into millivolts using the VDD
// reference assumption.
namespace analog_sensor {

// Configure the ADC reference and prescaler used by the sensor channel.
void init();

// Take one blocking raw ADC sample.
uint16_t sample();

// Convert a raw 10-bit ADC count to millivolts.
uint16_t to_millivolts(uint16_t raw_sample);

// Convenience path for callers that need only the scaled value.
uint16_t sample_millivolts();

}  // namespace analog_sensor
