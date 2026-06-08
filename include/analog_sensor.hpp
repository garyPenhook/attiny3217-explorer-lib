#pragma once

#include <avr/io.h>
#include <stdint.h>

#include "board.hpp"

// Board-level analog input wrapper. The channel and supply reference default to
// the Explorer board wiring (board::analog_sensor_channel / _vref_mv) but can be
// overridden per call so the same code serves a differently wired board.
namespace analog_sensor {

// Configure the ADC reference and prescaler used by the sensor channel.
void init();

// Take one blocking raw ADC sample from the given channel.
uint16_t sample(ADC_MUXPOS_t channel = board::analog_sensor_channel);

// Convert a raw 10-bit ADC count to millivolts for the given reference voltage.
uint16_t to_millivolts(uint16_t raw_sample, uint16_t vref_mv = board::analog_sensor_vref_mv);

// Convenience path: sample the board channel and scale with the board reference.
uint16_t sample_millivolts();

}  // namespace analog_sensor
