#pragma once

// Minimal status LED task. The implementation maps this to board::StatusLed.
namespace blink_task {

// Configure the LED pin and drive it to the idle state.
void init();

// Toggle the LED once; callers decide the timing.
void tick();

}  // namespace blink_task
