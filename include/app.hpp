#pragma once

// Foreground application loop. Hardware setup belongs in platform_init; this
// layer coordinates sampling, communication, and periodic status output.
namespace app {

// Runs forever after platform_init::run() completes.
[[noreturn]] void run();

}  // namespace app
