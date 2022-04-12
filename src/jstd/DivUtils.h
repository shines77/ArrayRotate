
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

namespace jstd {

template <typename Integal>
static inline
std::uint32_t floorLog2(Integal val)
{
    std::uint32_t kMaxByteLen = sizeof(Integal) * 8;
    std::uint32_t log2_i = 0;

    if ((val & (val - 1)) == 0) {
        while (val > 1) {
            val >>= 1;
            log2_i++;
        }
        return log2_i;
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
// __umulh() is only available in x64 mode under Visual Studio.
//           But, I have written a version (see below) for x86 (32bit) mode.
//
static inline
uint64_t mul_u64x32_high(uint64_t n1, uint32_t n2) {
    return __umulh(n1, (uint64_t)n2);
}

static inline
uint64_t mul_u64x64_high(uint64_t n1, uint64_t n2) {
    return __umulh(n1, n2);
}

#else // !_MSC_VER

static inline
uint64_t mul_u64x32_high(uint64_t n1, uint32_t n2) {
    return (((__uint128_t)n1 * n2) >> 64u);
}

static inline
uint64_t mul_u64x64_high(uint64_t n1, uint64_t n2) {
    return (((__uint128_t)n1 * n2) >> 64u);
}

#endif // _MSC_VER

static inline
uint64_t mul_u64x64_high_ex(uint64_t n1, uint64_t n2) {
    return mul_u64x64_high(n1, n2);
}

#else // !__amd64__

/*****************************************************************

   low64_bits = low0, high0
   divisor32  = low1, 0

   low64_bits * divisor32 =

 |           |             |            |           |
 |           |             |      high0 * 0         |  product_03
 |           |       low0  * 0          |           |  product_02
 |           |       high0 * low1       |           |  product_01
 |      low0 * low1        |            |           |  product_00
 |           |             |            |           |
 0          32            64           96          128

*****************************************************************/

static inline
uint32_t mul_u64x32_high(uint64_t n1, uint32_t n2) {
    uint32_t low0  = (uint32_t)(n1 & 0xFFFFFFFFull);
    uint32_t high0 = (uint32_t)(n1 >> 32u);
    if (high0 == 0)
        return 0;

    uint64_t product00 = (uint64_t)n2 * low0;
    uint64_t product01 = (uint64_t)n2 * high0;

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
 |           |             |      high0 * high1     |  product_03
 |           |       low0  * high1      |           |  product_02
 |           |       high0 * low1       |           |  product_01
 |      low0 * low1        |            |           |  product_00
 |           |             |            |           |
 0          32            64           96          128

*****************************************************************/

static inline
uint64_t mul_u64x64_high(uint64_t n1, uint64_t n2) {
    uint32_t low1  = (uint32_t)(n2 & 0xFFFFFFFFull);
    uint32_t high1 = (uint32_t)(n2 >> 32u);
    if (high1 == 0) {
        return mul_u64x32_high(n1, low1);
    }

    uint32_t low0  = (uint32_t)(n1 & 0xFFFFFFFFull);
    uint32_t high0 = (uint32_t)(n1 >> 32u);

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
uint64_t mul_u64x64_high_ex(uint64_t n1, uint64_t n2) {
    uint32_t low0  = (uint32_t)(n1 & 0xFFFFFFFFull);
    uint32_t high0 = (uint32_t)(n1 >> 32u);
    uint32_t low1  = (uint32_t)(n2 & 0xFFFFFFFFull);
    uint32_t high1 = (uint32_t)(n2 >> 32u);

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
