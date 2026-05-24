#pragma once

#include <avr/io.h>
#include <stdint.h>

// Compile-time GPIO pin abstraction. Each Pin specialization resolves to a
// concrete PORTx register and bit mask with no object storage.
namespace gpio {

// Logical port identifiers available on the ATtiny3217 package.
enum class PortId : uint8_t {
    A,
    B,
    C,
};

template<PortId Port, uint8_t Bit>
struct Pin {
    // AVR PORT registers expose eight bits per port.
    static_assert(Bit < 8, "AVR port pins are 0..7");
    static constexpr uint8_t bit = Bit;
    static constexpr uint8_t mask = static_cast<uint8_t>(1u << Bit);

    static inline volatile PORT_t& port()
    {
        // if constexpr removes the unused branches for each Pin specialization.
        if constexpr (Port == PortId::A) {
            return PORTA;
        } else if constexpr (Port == PortId::B) {
            return PORTB;
        } else {
            return PORTC;
        }
    }

    static inline void configure_output()
    {
        // DIRSET avoids a read-modify-write sequence on the direction register.
        port().DIRSET = mask;
    }

    static inline void configure_input()
    {
        // DIRCLR avoids a read-modify-write sequence on the direction register.
        port().DIRCLR = mask;
    }

    static inline void set_high()
    {
        // OUTSET changes only this pin's output latch.
        port().OUTSET = mask;
    }

    static inline void set_low()
    {
        // OUTCLR changes only this pin's output latch.
        port().OUTCLR = mask;
    }

    static inline void toggle()
    {
        // OUTTGL toggles only this pin's output latch.
        port().OUTTGL = mask;
    }
};

}  // namespace gpio
