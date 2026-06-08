#pragma once

#include <avr/io.h>
#include <stdint.h>

// Small ADC0 helper layer. These functions are inline so call sites compile
// down to direct register writes without a runtime abstraction penalty.
namespace adc0 {

constexpr uint8_t reg8(auto value)
{
    // Microchip headers expose many register fields as enum types; convert
    // them to the raw 8-bit values expected by the peripheral registers.
    return static_cast<uint8_t>(value);
}

static inline void init_vdd(ADC_PRESC_t prescaler = ADC_PRESC_DIV16_gc)
{
    // Disable ADC0 before changing reference and prescaler settings.
    ADC0.CTRLA = 0u;

    // Use VDD as the conversion reference. With a 5 V board supply, each ADC
    // count is approximately 5000 mV / 1024.
    ADC0.CTRLC = reg8(ADC_REFSEL_VDDREF_gc) | reg8(prescaler);

    // Keep default sample accumulation/window modes and a minimum sample time.
    ADC0.CTRLE = 0u;
    ADC0.SAMPCTRL = 0u;

    // Enable ADC0 after all configuration registers are stable.
    ADC0.CTRLA = reg8(ADC_ENABLE_bm);
}

static inline void init_internal_1v1(ADC_PRESC_t prescaler = ADC_PRESC_DIV16_gc)
{
    // Select and enable the internal 1.1 V reference for ADC0.
    VREF.CTRLA = (VREF.CTRLA & static_cast<uint8_t>(~VREF_ADC0REFSEL_gm))
                 | reg8(VREF_ADC0REFSEL_1V1_gc);
    VREF.CTRLB |= VREF_ADC0REFEN_bm;

    // Reconfigure ADC0 to use the internal reference.
    ADC0.CTRLA = 0u;
    ADC0.CTRLC = reg8(ADC_REFSEL_INTREF_gc) | reg8(prescaler);
    ADC0.CTRLE = 0u;
    ADC0.SAMPCTRL = 0u;
    ADC0.CTRLA = reg8(ADC_ENABLE_bm);
}

static inline uint16_t read_blocking(ADC_MUXPOS_t channel)
{
    // Select the analog input, clear any stale result-ready flag, then start a
    // single conversion.
    ADC0.MUXPOS = reg8(channel);
    ADC0.INTFLAGS = ADC_RESRDY_bm;
    ADC0.COMMAND = ADC_STCONV_bm;

    // Polling is deterministic here and avoids an ADC ISR for one foreground
    // sample per application frame.
    while ((ADC0.INTFLAGS & ADC_RESRDY_bm) == 0u) {
    }

    // Reading RES returns the 10-bit conversion result from RESL/RESH.
    return ADC0.RES;
}

}  // namespace adc0
