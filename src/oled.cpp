#include "oled.hpp"

#include <avr/pgmspace.h>

#include "twi0.hpp"

namespace {

// SSD1306 control bytes. 0x00 selects command stream, 0x40 selects display RAM.
constexpr uint8_t control_command = 0x00u;
constexpr uint8_t control_data = 0x40u;

// Text rendering uses 5 bitmap columns plus one blank spacer column.
constexpr uint8_t glyph_width = 5u;
constexpr uint8_t glyph_stride = glyph_width + 1u;

// Minimal 5x7 glyph set for the status strings used by this firmware.
enum class Glyph : uint8_t {
    space,
    dash,
    zero,
    one,
    two,
    three,
    four,
    five,
    six,
    seven,
    eight,
    nine,
    A,
    C,
    D,
    E,
    I,
    K,
    L,
    M,
    O,
    R,
    T,
    V,
    W,
    X,
    unknown,
};

constexpr uint8_t glyph_count = static_cast<uint8_t>(Glyph::unknown) + 1u;

// Each byte is one vertical column, least-significant bit at the top.
const uint8_t glyph_table[glyph_count][glyph_width] PROGMEM = {
    {0x00, 0x00, 0x00, 0x00, 0x00},  // space
    {0x00, 0x08, 0x08, 0x08, 0x00},  // -
    {0x3E, 0x51, 0x49, 0x45, 0x3E},  // 0
    {0x00, 0x42, 0x7F, 0x40, 0x00},  // 1
    {0x42, 0x61, 0x51, 0x49, 0x46},  // 2
    {0x21, 0x41, 0x45, 0x4B, 0x31},  // 3
    {0x18, 0x14, 0x12, 0x7F, 0x10},  // 4
    {0x27, 0x45, 0x45, 0x45, 0x39},  // 5
    {0x3C, 0x4A, 0x49, 0x49, 0x30},  // 6
    {0x01, 0x71, 0x09, 0x05, 0x03},  // 7
    {0x36, 0x49, 0x49, 0x49, 0x36},  // 8
    {0x06, 0x49, 0x49, 0x29, 0x1E},  // 9
    {0x7E, 0x09, 0x09, 0x09, 0x7E},  // A
    {0x3E, 0x41, 0x41, 0x41, 0x22},  // C
    {0x7F, 0x41, 0x41, 0x22, 0x1C},  // D
    {0x7F, 0x49, 0x49, 0x49, 0x41},  // E
    {0x00, 0x41, 0x7F, 0x41, 0x00},  // I
    {0x7F, 0x08, 0x14, 0x22, 0x41},  // K
    {0x7F, 0x40, 0x40, 0x40, 0x40},  // L
    {0x7F, 0x02, 0x04, 0x02, 0x7F},  // M
    {0x3E, 0x41, 0x41, 0x41, 0x3E},  // O
    {0x7F, 0x09, 0x19, 0x29, 0x46},  // R
    {0x01, 0x01, 0x7F, 0x01, 0x01},  // T
    {0x1F, 0x20, 0x40, 0x20, 0x1F},  // V
    {0x7F, 0x20, 0x18, 0x20, 0x7F},  // W
    {0x63, 0x14, 0x08, 0x14, 0x63},  // X
    {0x7F, 0x09, 0x09, 0x09, 0x06},  // unknown -> P-like marker
};

bool send_control(uint8_t control, const uint8_t* bytes, uint8_t size)
{
    if (twi0::start_write(oled::address) != twi0::Status::ok) {
        twi0::stop();
        return false;
    }

    if (twi0::write_byte(control) != twi0::Status::ok) {
        twi0::stop();
        return false;
    }

    if (twi0::write_bytes(bytes, size) != twi0::Status::ok) {
        twi0::stop();
        return false;
    }

    twi0::stop();
    return true;
}

bool send_commands(const uint8_t* commands, uint8_t size)
{
    return send_control(control_command, commands, size);
}

bool set_window(uint8_t first_column, uint8_t last_column, uint8_t first_page, uint8_t last_page)
{
    // clang-format off
    const uint8_t commands[] = {
        0x21u, first_column, last_column,  // column address range
        0x22u, first_page,   last_page,    // page address range
    };
    // clang-format on

    return send_commands(commands, static_cast<uint8_t>(sizeof(commands)));
}

Glyph glyph_for(char c)
{
    if (c >= 'a' && c <= 'z') {
        c = static_cast<char>(c - ('a' - 'A'));
    }

    if (c >= '0' && c <= '9') {
        return static_cast<Glyph>(static_cast<uint8_t>(Glyph::zero) + static_cast<uint8_t>(c - '0'));
    }

    switch (c) {
        case ' ': return Glyph::space;
        case '-': return Glyph::dash;
        case 'A': return Glyph::A;
        case 'C': return Glyph::C;
        case 'D': return Glyph::D;
        case 'E': return Glyph::E;
        case 'I': return Glyph::I;
        case 'K': return Glyph::K;
        case 'L': return Glyph::L;
        case 'M': return Glyph::M;
        case 'O': return Glyph::O;
        case 'R': return Glyph::R;
        case 'T': return Glyph::T;
        case 'V': return Glyph::V;
        case 'W': return Glyph::W;
        case 'X': return Glyph::X;
        default: return Glyph::unknown;
    }
}

uint8_t* append_glyph(uint8_t* out, Glyph glyph)
{
    const uint8_t index = static_cast<uint8_t>(glyph);

    for (uint8_t column = 0u; column < glyph_width; ++column) {
        *out++ = pgm_read_byte(&glyph_table[index][column]);
    }

    *out++ = 0x00u;
    return out;
}

}  // namespace

namespace oled {

bool init()
{
    // Sequence follows the Explorer assembly bring-up: 128x64 panel, remapped
    // columns, COM scan down, horizontal addressing, charge pump on.
    // clang-format off
    const uint8_t init_commands[] = {
        0xAEu,         // display off
        0xD5u, 0x80u,  // display clock divide
        0xA8u, 0x3Fu,  // multiplex ratio for 64 rows
        0xD3u, 0x00u,  // display offset
        0x40u,         // display start line
        0xA1u,         // segment remap
        0xC8u,         // COM scan direction remap
        0xDAu, 0x12u,  // COM pins for 128x64
        0x81u, 0x7Fu,  // contrast
        0xA4u,         // resume display from RAM
        0xA6u,         // normal display
        0x20u, 0x00u,  // horizontal addressing mode
        0x8Du, 0x14u,  // charge pump enable
        0xAFu,         // display on
    };
    // clang-format on

    return send_commands(init_commands, static_cast<uint8_t>(sizeof(init_commands))) && clear();
}

bool clear()
{
    const uint8_t zeros[width] = {};

    for (uint8_t page = 0u; page < page_count; ++page) {
        if (!set_window(0u, static_cast<uint8_t>(width - 1u), page, page)
            || !send_control(control_data, zeros, width)) {
            return false;
        }
    }

    return true;
}

bool write_text_page(uint8_t page, const char* text)
{
    if (page >= page_count) {
        return false;
    }

    uint8_t row[width] = {};
    uint8_t* out = row;
    uint8_t columns_left = width;

    while (*text != '\0' && columns_left >= glyph_stride) {
        out = append_glyph(out, glyph_for(*text++));
        columns_left = static_cast<uint8_t>(columns_left - glyph_stride);
    }

    return set_window(0u, static_cast<uint8_t>(width - 1u), page, page)
           && send_control(control_data, row, width);
}

}  // namespace oled
