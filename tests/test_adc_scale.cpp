#include "adc_scale.hpp"

#include "test_assert.hpp"

void run_tests()
{
    // 10-bit ADC, 5000 mV reference (Explorer board default).
    CHECK_EQ(adc_scale::to_millivolts(0u, 5000u), 0u);
    CHECK_EQ(adc_scale::to_millivolts(1u, 5000u), 5u);
    CHECK_EQ(adc_scale::to_millivolts(512u, 5000u), 2500u);
    CHECK_EQ(adc_scale::to_millivolts(1023u, 5000u), 4995u);

    // 10-bit ADC, 3300 mV reference.
    CHECK_EQ(adc_scale::to_millivolts(1023u, 3300u), 3297u);

    // Explicit 12-bit full scale, 3300 mV reference.
    CHECK_EQ(adc_scale::to_millivolts(2048u, 3300u, 4096u), 1650u);

    // Degenerate full scale must not divide by zero.
    CHECK_EQ(adc_scale::to_millivolts(100u, 5000u, 0u), 0u);

    // constexpr-usable.
    static_assert(adc_scale::to_millivolts(512u, 5000u) == 2500u);
}

TEST_MAIN()
