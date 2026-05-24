#pragma once

#include <avr/cpufunc.h>
#include <avr/io.h>
#include <stdint.h>

// Protected clock-control helpers for the ATtiny3217 main clock.
namespace clock {

constexpr uint8_t reg8(auto value)
{
    // Convert typed clock enum values from the DFP header to register bytes.
    return static_cast<uint8_t>(value);
}

static inline void use_internal_20mhz_no_prescaler()
{
    // CLKCTRL is protected by CCP, so use avr-libc's timed write helper.
    ccp_write_io(&CLKCTRL.MCLKCTRLA, reg8(CLKCTRL_CLKSEL_OSC20M_gc));

    // Disable the main prescaler so F_CPU matches the 20 MHz oscillator.
    ccp_write_io(&CLKCTRL.MCLKCTRLB, 0u);
}

static inline void use_internal_20mhz_with_prescaler(CLKCTRL_PDIV_t divider)
{
    // Select the same oscillator, then enable the requested prescaler.
    ccp_write_io(&CLKCTRL.MCLKCTRLA, reg8(CLKCTRL_CLKSEL_OSC20M_gc));
    ccp_write_io(&CLKCTRL.MCLKCTRLB, reg8(CLKCTRL_PEN_bm) | reg8(divider));
}

}  // namespace clock
