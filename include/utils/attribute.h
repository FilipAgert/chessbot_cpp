#pragma once
#pragma once

#if defined(__cpp_attribute_assume) && __cpp_attribute_assume >= 202207L
#define ASSUME(expr) [[assume(expr)]]
#elif defined(__clang__)
#define ASSUME(expr) __builtin_assume(expr)
#elif defined(_MSC_VER)
#define ASSUME(expr) __assume(expr)
#elif defined(__GNUC__)
#define ASSUME(expr) __attribute__((assume(expr)))
#else
#define ASSUME(expr)                                                                                                                                           \
    do {                                                                                                                                                       \
        if (!(expr))                                                                                                                                           \
            __builtin_unreachable();                                                                                                                           \
    } while (0)
#endif