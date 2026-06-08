#include "bit_ops.hpp"

#include "test_assert.hpp"

void run_tests()
{
    // mask8 is compile-time; verify a few positions.
    static_assert(bit_ops::mask8<0>() == 0x01u);
    static_assert(bit_ops::mask8<7>() == 0x80u);
    CHECK_EQ(bit_ops::mask8<3>(), 0x08u);

    // test() reports whether any masked bit is set.
    CHECK(bit_ops::test(0b1010u, 0b0010u));
    CHECK(!bit_ops::test(0b1010u, 0b0001u));

    // set()/clear() force bits high/low.
    CHECK_EQ(bit_ops::set(0x00u, 0x0Fu), 0x0Fu);
    CHECK_EQ(bit_ops::set(0xF0u, 0x0Fu), 0xFFu);
    CHECK_EQ(bit_ops::clear(0xFFu, 0x0Fu), 0xF0u);
    CHECK_EQ(bit_ops::clear(0xAAu, 0xFFu), 0x00u);

    // byte splitting.
    CHECK_EQ(bit_ops::low_byte(0x1234u), 0x34u);
    CHECK_EQ(bit_ops::high_byte(0x1234u), 0x12u);
    CHECK_EQ(bit_ops::low_byte(0x00FFu), 0xFFu);
    CHECK_EQ(bit_ops::high_byte(0xFF00u), 0xFFu);
}

TEST_MAIN()
