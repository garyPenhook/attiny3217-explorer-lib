#include "fuses.hpp"

#include <avr/io.h>

// avr-libc places this object in the ELF .fuse section. Programming tools can
// extract the bytes from the ELF/HEX and program the ATtiny3217 fuse memory.
FUSES = {
    fuses::wdtcfg,    // byte 0: WDTCFG
    fuses::bodcfg,    // byte 1: BODCFG
    fuses::osccfg,    // byte 2: OSCCFG
    fuses::reserved,  // byte 3: reserved
    fuses::tcd0cfg,   // byte 4: TCD0CFG
    fuses::syscfg0,   // byte 5: SYSCFG0
    fuses::syscfg1,   // byte 6: SYSCFG1
    fuses::append,    // byte 7: APPEND
    fuses::bootend,   // byte 8: BOOTEND
};
