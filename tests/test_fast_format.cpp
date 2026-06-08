#include "fast_format.hpp"

#include <cstring>

#include "test_assert.hpp"

namespace {

// Render through a formatter into a fixed buffer and NUL-terminate.
template<typename Fn>
const char* render(char* buffer, Fn&& fn)
{
    char* end = fn(buffer);
    *end = '\0';
    return buffer;
}

}  // namespace

void run_tests()
{
    // hex_nibble maps 0..15 to uppercase ASCII.
    CHECK_EQ(fast_format::hex_nibble(0x0u), '0');
    CHECK_EQ(fast_format::hex_nibble(0x9u), '9');
    CHECK_EQ(fast_format::hex_nibble(0xAu), 'A');
    CHECK_EQ(fast_format::hex_nibble(0xFu), 'F');
    CHECK_EQ(fast_format::hex_nibble(0x1Fu), 'F');  // high nibble ignored

    char buf[8];
    CHECK(std::strcmp(render(buf, [](char* o) { return fast_format::append_hex8(o, 0xABu); }), "AB") == 0);
    CHECK(std::strcmp(render(buf, [](char* o) { return fast_format::append_hex8(o, 0x07u); }), "07") == 0);
    CHECK(std::strcmp(render(buf, [](char* o) { return fast_format::append_hex16(o, 0x1234u); }), "1234") == 0);
    CHECK(std::strcmp(render(buf, [](char* o) { return fast_format::append_hex16(o, 0x00FFu); }), "00FF") == 0);

    // xor_checksum over [first, last).
    const uint8_t abc[] = {'A', 'B', 'C'};
    CHECK_EQ(fast_format::xor_checksum(abc, abc + 3), 0x40u);  // 0x41^0x42^0x43
    CHECK_EQ(fast_format::xor_checksum(abc, abc), 0x00u);      // empty range

    // append_decimal_u16 with no leading zeros, lone-zero handling.
    CHECK(std::strcmp(render(buf, [](char* o) { return fast_format::append_decimal_u16(o, 0u); }), "0") == 0);
    CHECK(std::strcmp(render(buf, [](char* o) { return fast_format::append_decimal_u16(o, 5u); }), "5") == 0);
    CHECK(std::strcmp(render(buf, [](char* o) { return fast_format::append_decimal_u16(o, 42u); }), "42") == 0);
    CHECK(std::strcmp(render(buf, [](char* o) { return fast_format::append_decimal_u16(o, 1000u); }), "1000") == 0);
    CHECK(std::strcmp(render(buf, [](char* o) { return fast_format::append_decimal_u16(o, 65535u); }), "65535") == 0);
}

TEST_MAIN()
