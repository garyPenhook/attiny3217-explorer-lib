#pragma once

#include <cstdint>
#include <cstdio>

// Tiny dependency-free check harness for the host unit tests. Each test file
// defines run_tests() and ends with TEST_MAIN().
namespace test {
inline int failures = 0;
}

#define CHECK(cond)                                                     \
    do {                                                                \
        if (!(cond)) {                                                  \
            std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); \
            ++test::failures;                                           \
        }                                                               \
    } while (0)

#define CHECK_EQ(a, b)                                         \
    do {                                                       \
        const long _a = static_cast<long>(a);                  \
        const long _b = static_cast<long>(b);                  \
        if (_a != _b) {                                        \
            std::printf("FAIL %s:%d: %s == %s (%ld != %ld)\n", \
                __FILE__, __LINE__, #a, #b, _a, _b);           \
            ++test::failures;                                  \
        }                                                      \
    } while (0)

#define TEST_MAIN()                                         \
    int main()                                              \
    {                                                       \
        run_tests();                                        \
        if (test::failures != 0) {                          \
            std::printf("%d failure(s)\n", test::failures); \
            return 1;                                       \
        }                                                   \
        std::printf("OK\n");                                \
        return 0;                                           \
    }
