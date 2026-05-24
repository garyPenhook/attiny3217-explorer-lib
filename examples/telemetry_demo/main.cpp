/**
 * @file main.cpp
 * @author gary
 * @date 2026-05-24
 * @brief Firmware entry point for the ATtiny3217 application.
 */

#include "app.hpp"
#include "platform_init.hpp"

int main()
{
    // Bring the chip, GPIO, serial ports, timer, ADC, and interrupts into a
    // known state before starting the foreground application loop.
    platform_init::run();

    // app::run() is marked noreturn and owns the cooperative foreground loop.
    app::run();
}
