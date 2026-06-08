#pragma once

#include <avr/io.h>
#include <stdint.h>

#include "gpio.hpp"

// Central board mapping for the ATtiny3217 Curiosity Nano wiring used by this
// firmware. Keeping aliases here prevents peripheral code from hard-coding
// physical port bits.
namespace board {

// The build defines F_CPU, and runtime clock setup in platform_init keeps the
// CPU clock aligned with this value.
inline constexpr uint32_t cpu_hz = F_CPU;

// Human-readable pin aliases for application and peripheral setup.
using StatusLed = gpio::Pin<gpio::PortId::B, 5>;
using ButtonIn = gpio::Pin<gpio::PortId::B, 4>;
using Usart0Tx = gpio::Pin<gpio::PortId::B, 2>;
using Usart0Rx = gpio::Pin<gpio::PortId::B, 3>;
using Spi0Sck = gpio::Pin<gpio::PortId::A, 3>;
using Spi0Miso = gpio::Pin<gpio::PortId::A, 5>;
using Spi0Mosi = gpio::Pin<gpio::PortId::A, 4>;
using Twi0Scl = gpio::Pin<gpio::PortId::B, 0>;
using Twi0Sda = gpio::Pin<gpio::PortId::B, 1>;

// Analog sensor wiring/scaling for the Explorer board. Override these to retarget
// the analog_sensor wrapper at a different channel or supply reference.
inline constexpr ADC_MUXPOS_t analog_sensor_channel = ADC_MUXPOS_AIN7_gc;
inline constexpr uint16_t analog_sensor_vref_mv = 5000u;  // VDD reference, in mV

}  // namespace board
