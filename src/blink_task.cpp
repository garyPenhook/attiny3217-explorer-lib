#include "blink_task.hpp"

#include "board.hpp"

namespace blink_task {

// Board alias keeps this module independent of the physical port/bit choice.
using LedPin = board::StatusLed;

void init()
{
    // Drive the status LED pin as an output and start from the off/low state.
    LedPin::configure_output();
    LedPin::set_low();
}

void tick()
{
    // Toggle directly through OUTTGL via the GPIO wrapper.
    LedPin::toggle();
}

}  // namespace blink_task
