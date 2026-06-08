#include "app.hpp"

#include <stdint.h>

#include "analog_sensor.hpp"
#include "bit_ops.hpp"
#include "blink_task.hpp"
#include "button_irq.hpp"
#include "fast_format.hpp"
#include "oled.hpp"
#include "spi0.hpp"
#include "timer0.hpp"
#include "twi0.hpp"
#include "uart0.hpp"
#include "uart0_rx.hpp"

namespace {

// A received UART byte with bit 0 set triggers an extra LED toggle.
constexpr uint8_t command_echo_blink = bit_ops::mask8<0>();

// Probe byte clocked through SPI0 once per telemetry frame.
constexpr uint8_t spi_probe_byte = 0x55u;

// Explorer MCP9808 7-bit address; address-phase ACK is enough for a bus probe.
constexpr uint8_t twi_target_address = 0x1cu;

// Text and field markers for the UART status protocol.
constexpr char crlf[] = "\r\n";
constexpr char button_text[] = "button\r\n";
constexpr char frame_sample_key = 'S';
constexpr char frame_millivolt_key = 'M';
constexpr char frame_echo_key = 'E';
constexpr char frame_twi_key = 'T';
constexpr char frame_checksum_key = 'C';
constexpr char frame_separator = ' ';
constexpr char frame_assign = '=';

// Snapshot of all values reported in one status frame.
struct Telemetry {
    uint16_t sample;
    uint16_t sample_mv;
    uint8_t echo;
    bool twi_ok;
};

char* append_text(char* out, const char* text)
{
    while (*text != '\0') {
        *out++ = *text++;
    }

    return out;
}

char* append_decimal_u16(char* out, uint16_t value)
{
    constexpr uint16_t divisors[] = {10000u, 1000u, 100u, 10u, 1u};
    bool started = false;

    for (uint8_t i = 0u; i < static_cast<uint8_t>(sizeof(divisors) / sizeof(divisors[0])); ++i) {
        const uint16_t divisor = divisors[i];
        const uint8_t digit = static_cast<uint8_t>(value / divisor);
        if (digit != 0u || started) {
            *out++ = static_cast<char>('0' + digit);
            started = true;
        }
        value = static_cast<uint16_t>(value % divisor);
    }

    if (!started) {
        *out++ = '0';
    }

    return out;
}

void write_status_display(const Telemetry& telemetry)
{
    char text[22] = {};
    char* out = text;

    out = append_text(out, "ADC ");
    out = append_decimal_u16(out, telemetry.sample_mv);
    out = append_text(out, "MV ");
    out = append_text(out, telemetry.twi_ok ? "TWI OK" : "TWI ERR");
    *out = '\0';

    oled::write_text_page(0u, text);
}

inline void write_checked_char(char value, uint8_t& checksum)
{
    // The checksum covers every emitted character before the checksum field.
    checksum ^= static_cast<uint8_t>(value);
    uart0::write_byte(static_cast<uint8_t>(value));
}

inline void write_checked_hex8(uint8_t value, uint8_t& checksum)
{
    // Emit two ASCII hex digits while feeding the same bytes into the checksum.
    write_checked_char(fast_format::hex_nibble(static_cast<uint8_t>(value >> 4)), checksum);
    write_checked_char(fast_format::hex_nibble(value), checksum);
}

inline void write_checked_hex16(uint16_t value, uint8_t& checksum)
{
    // Emit high byte first so multi-byte values are human-readable.
    write_checked_hex8(static_cast<uint8_t>(value >> 8), checksum);
    write_checked_hex8(static_cast<uint8_t>(value), checksum);
}

inline void write_hex8(uint8_t value)
{
    // Checksum digits are not folded into the checksum itself.
    uart0::write_byte(static_cast<uint8_t>(fast_format::hex_nibble(static_cast<uint8_t>(value >> 4))));
    uart0::write_byte(static_cast<uint8_t>(fast_format::hex_nibble(value)));
}

[[gnu::noinline]] Telemetry sample_and_transfer()
{
    // Sample the analog input first so the frame's raw and scaled values come
    // from the same ADC conversion.
    const uint16_t sample = analog_sensor::sample();
    const uint16_t sample_mv = analog_sensor::to_millivolts(sample);

    // SPI is full-duplex; the returned byte is what the bus presented while
    // 0x55 was shifted out.
    const uint8_t echo = spi0::transfer(spi_probe_byte);

    // Probe the Explorer I2C bus with an address-only write to the MCP9808.
    const bool twi_ok = twi0::start_write(twi_target_address) == twi0::Status::ok;
    twi0::stop();

    return Telemetry{
        .sample = sample,
        .sample_mv = sample_mv,
        .echo = echo,
        .twi_ok = twi_ok,
    };
}

[[gnu::noinline]] void service_events()
{
    uint8_t received = 0u;

    // Toggle once per foreground cycle, then service asynchronous events.
    blink_task::tick();

    // Button press work stays in foreground; the ISR only latches the event.
    if (button_irq::consume_press_debounced(timer0::tick_count())) {
        uart0::write_cstr(button_text);
    }

    // Echo one received byte and let bit 0 request an extra LED tick.
    if (uart0_rx::read_byte(received)) {
        uart0::write_byte(received);
        if (bit_ops::test(received, command_echo_blink)) {
            blink_task::tick();
        }
    }
}

[[gnu::noinline]] void send_status_frame(const Telemetry& telemetry)
{
    // Frame format:
    // S=<raw hex16> M=<millivolts hex16> E=<spi echo hex8> T=<0|1> C=<xor hex8>
    // Checksum is XOR of every character through "C="; the two hex digits are not included.
    uint8_t checksum = 0u;

    write_checked_char(frame_sample_key, checksum);
    write_checked_char(frame_assign, checksum);
    write_checked_hex16(telemetry.sample, checksum);
    write_checked_char(frame_separator, checksum);
    write_checked_char(frame_millivolt_key, checksum);
    write_checked_char(frame_assign, checksum);
    write_checked_hex16(telemetry.sample_mv, checksum);
    write_checked_char(frame_separator, checksum);
    write_checked_char(frame_echo_key, checksum);
    write_checked_char(frame_assign, checksum);
    write_checked_hex8(telemetry.echo, checksum);
    write_checked_char(frame_separator, checksum);
    write_checked_char(frame_twi_key, checksum);
    write_checked_char(frame_assign, checksum);
    write_checked_char(telemetry.twi_ok ? '1' : '0', checksum);
    write_checked_char(frame_separator, checksum);
    write_checked_char(frame_checksum_key, checksum);
    write_checked_char(frame_assign, checksum);
    write_hex8(checksum);
    uart0::write_cstr(crlf);
}

}  // namespace

namespace app {

[[noreturn]] void run()
    {
        for (;;) {
            // Each iteration samples peripherals, services input events, prints one
            // status frame, then waits for the periodic TCA0 tick.
            const Telemetry telemetry = sample_and_transfer();
            service_events();
            write_status_display(telemetry);
            send_status_frame(telemetry);
            timer0::wait_tick();
        }
    }

}  // namespace app
