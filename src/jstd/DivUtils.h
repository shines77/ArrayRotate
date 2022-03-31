
#ifndef JSTD_DIV_UTILS_H
#define JSTD_DIV_UTILS_H

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <math.h>
#include <assert.h>

#include <cstdint>
#include <cstddef>
#include <cstdbool>

#ifdef _MSC_VER
#include <intrin.h>     // For __umulh(), available only in x64 mode
#endif

#if defined(__SIZEOF_INT128__)
#define HAS_INT128_T
// clang-cl on Windows does not yet support 128-bit division
#if !(defined(__clang__) && defined(_MSC_VER))
#define HAS_INT128_DIV
#endif
#endif // __SIZEOF_INT128__

namespace jstd {

template <typename Integal>
static inline
std::uint32_t floorLog2(Integal val)
{
    std::uint32_t kMaxByteLen = sizeof(Integal) * 8;
    std::uint32_t log2_i = 0;

    if ((val & (val - 1)) == 0) {
        while (val != 0) {
            val >>= 1;
            log2_i++;
        }
        return ((log2_i > 0) ? (log2_i - 1) : 0);
    }

    std::size_t power2 = 1;
    do {
        if (val > power2 && val < power2 * 2) {
            return log2_i;
        }
        power2 <<= 1;
        log2_i++;
    } while (log2_i < kMaxByteLen);

    return log2_i;
}

#if defined(WIN64) || defined(_WIN64) || defined(_M_X64) || defined(_M_AMD64) \
 || defined(_M_IA64) || defined(__amd64__) || defined(__x86_64__)

#if defined(_MSC_VER)

//
// See: https://github.com/lemire/fastmod
//
// __umulh() is only available in x64 mode under Visual Studio: don't compile to 32-bit!
//           but, I write a version for x86 (32bit) mode: see below.
//
static inline
uint64_t mul128_high_u32(uint64_t low64_bits, uint32_t divisor) {
    return __umulh(low64_bits, divisor);
}

static inline
uint64_t mul128_high_u64(uint64_t low64_bits, uint64_t divisor) {
    return __umulh(low64_bits, divisor);
}

#else // !_MSC_VER

static inline
uint64_t mul128_high_u32(uint64_t low64_bits, uint32_t divisor) {
    return (((__uint128_t)low64_bits * divisor) >> 64u);
}

static inline
uint64_t mul128_high_u64(uint64_t low64_bits, uint64_t divisor) {
    return (((__uint128_t)low64_bits * divisor) >> 64u);
}

#endif // _MSC_VER

static inline
uint64_t mul128_high_u64_ex(uint64_t low64_bits, uint64_t divisor) {
    return mul128_high_u64(low64_bits, divisor);
}

#else // !__amd64__

/*****************************************************************

   low64_bits = low0, high0
   divisor32  = low1, 0

   low64_bits * divisor32 =

 |           |             |            |           |
 |           |             |      high0 * 0         |  product03
 |           |       low0  * 0          |           |  product02
 |           |       high0 * low1       |           |  product01
 |      low0 * low1        |            |           |  product00
 |           |             |            |           |

*****************************************************************/

static inline
uint32_t mul128_high_u32(uint64_t low64_bits, uint32_t divisor) {
    uint32_t low0  = (uint32_t)(low64_bits & 0xFFFFFFFFull);
    uint32_t high0 = (uint32_t)(low64_bits >> 32u);
    if (high0 == 0)
        return 0;

    uint64_t product00 = (uint64_t)divisor * low0;
    uint64_t product01 = (uint64_t)divisor * high0;

    uint32_t product00_high = (uint32_t)(product00 >> 32u);
    uint32_t product01_low  = (uint32_t)(product01 & 0xFFFFFFFFull);
    uint32_t product01_high = (uint32_t)(product01 >> 32u);

    uint32_t carry32 = (product00_high > ~product01_low) ? 1 : 0;
    uint32_t result = product01_high + carry32;
    return result;
}

/*****************************************************************

   low64_bits = low0, high0
   divisor64  = low1, high1

   low64_bits * divisor64 =

 |           |             |            |           |
 |           |             |      high0 * high1     |  product03
 |           |       low0  * high1      |           |  product02
 |           |       high0 * low1       |           |  product01
 |      low0 * low1        |            |           |  product00
 |           |             |            |           |

*****************************************************************/

static inline
uint64_t mul128_high_u64(uint64_t low64_bits, uint64_t divisor) {
    uint32_t low1  = (uint32_t)(divisor & 0xFFFFFFFFull);
    uint32_t high1 = (uint32_t)(divisor >> 32u);
    if (high1 == 0) {
        return mul128_high_u32(low64_bits, low1);
    }

    uint32_t low0  = (uint32_t)(low64_bits & 0xFFFFFFFFull);
    uint32_t high0 = (uint32_t)(low64_bits >> 32u);

    if (high0 == 0) {
        uint64_t product00 = (uint64_t)low0 * low1;
        uint64_t product02 = (uint64_t)low0 * high1;

        uint32_t product00_high = (uint32_t)(product00 >> 32u);
        uint32_t product02_low  = (uint32_t)(product02 & 0xFFFFFFFFull);
        uint32_t product02_high = (uint32_t)(product02 >> 32u);

        uint64_t product_mid64 = product00_high + product02_low;
        uint32_t carry32 = (uint32_t)(product_mid64 >> 32u);

        uint64_t result = product02_high + carry32;
        return result;
    }
    else {
        uint64_t product00 = (uint64_t)low0 * low1;
        uint64_t product01 = (uint64_t)high0 * low1;
        uint64_t product02 = (uint64_t)low0 * high1;
        uint64_t product03 = (uint64_t)high0 * high1;

        uint32_t product00_high = (uint32_t)(product00 >> 32u);
        uint32_t product01_low  = (uint32_t)(product01 & 0xFFFFFFFFull);
        uint32_t product02_low  = (uint32_t)(product02 & 0xFFFFFFFFull);
        uint32_t product01_high = (uint32_t)(product01 >> 32u);
        uint32_t product02_high = (uint32_t)(product02 >> 32u);

        uint64_t product_mid64 = product00_high + product01_low + product02_low;
        uint32_t carry32 = (uint32_t)(product_mid64 >> 32u);

        uint64_t result = product01_high + product02_high + carry32 + product03;
        return result;
    }
}

static inline
uint64_t mul128_high_u64_ex(uint64_t low64_bits, uint64_t divisor) {
    uint32_t low0  = (uint32_t)(low64_bits & 0xFFFFFFFFull);
    uint32_t high0 = (uint32_t)(low64_bits >> 32u);
    uint32_t low1  = (uint32_t)(divisor & 0xFFFFFFFFull);
    uint32_t high1 = (uint32_t)(divisor >> 32u);

    if (high0 == 0) {
        uint64_t product00 = (uint64_t)low0 * low1;
        uint64_t product02 = (uint64_t)low0 * high1;

        uint32_t product00_high = (uint32_t)(product00 >> 32u);
        uint32_t product02_low  = (uint32_t)(product02 & 0xFFFFFFFFull);
        uint32_t product02_high = (uint32_t)(product02 >> 32u);

        uint64_t product_mid64 = product00_high + product02_low;
        uint32_t carry32 = (uint32_t)(product_mid64 >> 32u);

        uint64_t result = product02_high + carry32;
        return result;
    }
    else {
        uint64_t product00 = (uint64_t)low0 * low1;
        uint64_t product01 = (uint64_t)high0 * low1;
        uint64_t product02 = (uint64_t)low0 * high1;
        uint64_t product03 = (uint64_t)high0 * high1;

        uint32_t product00_high = (uint32_t)(product00 >> 32u);
        uint32_t product01_low  = (uint32_t)(product01 & 0xFFFFFFFFull);
        uint32_t product02_low  = (uint32_t)(product02 & 0xFFFFFFFFull);
        uint32_t product01_high = (uint32_t)(product01 >> 32u);
        uint32_t product02_high = (uint32_t)(product02 >> 32u);

        uint64_t product_mid64 = product00_high + product01_low + product02_low;
        uint32_t carry32 = (uint32_t)(product_mid64 >> 32u);

        uint64_t result = product01_high + product02_high + carry32 + product03;
        return result;
    }
}

#endif // __amd64__

} // namespace jstd

#endif // JSTD_DIV_UTILS_H