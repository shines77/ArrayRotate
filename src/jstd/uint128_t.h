
#ifndef JSTD_UINT128_T_H
#define JSTD_UINT128_T_H

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

#include "jstd/stddef.h"
#include "jstd/BitUtils.h"

//
// See: https://github.com/ridiculousfish/libdivide/blob/master/libdivide.h
//
#if defined(__SIZEOF_INT128__)
#define HAS_INT128_T
// clang-cl on Windows does not yet support 128-bit division
#if !(defined(__clang__) && defined(_MSC_VER))
#define HAS_INT128_DIV
#endif
#endif // __SIZEOF_INT128__

namespace jstd {

struct _uint128_t {
    typedef _uint128_t  this_type;

    typedef uint64_t    integral_t;
    typedef uint64_t    unsigned_t;
    typedef int64_t     signed_t;

    typedef uint64_t    high_t;
    typedef uint64_t    low_t;

    low_t  low;
    high_t high;

    _uint128_t() noexcept
        : low((low_t)0ull), high((high_t)0ull) {}

    _uint128_t(uint64_t _high, uint64_t _low) noexcept
        : low((low_t)_low), high((high_t)_high) {}

    _uint128_t(uint64_t _high, int64_t _low) noexcept
        : low((low_t)_low), high((high_t)_high) {}

    _uint128_t(int64_t _high, int64_t _low) noexcept
        : low((low_t)_low), high((high_t)_high) {}

    _uint128_t(uint32_t _high, uint32_t _low) noexcept
        : low((low_t)_low), high((high_t)_high) {}

    _uint128_t(uint32_t _high, int32_t _low) noexcept
        : low((low_t)_low), high((high_t)_high) {}

    _uint128_t(int32_t _high, int32_t _low) noexcept
        : low((low_t)_low), high((high_t)_high) {}

    _uint128_t(uint64_t _low) noexcept
        : low((low_t)_low), high((high_t)0ull) {}

    _uint128_t(int64_t _low) noexcept
        : low((low_t)_low), high((high_t)0ull) {}

    _uint128_t(uint32_t _low) noexcept
        : low((low_t)_low), high((high_t)0ull) {}

    _uint128_t(int32_t _low) noexcept
        : low((low_t)_low), high((high_t)0ull) {}

#ifdef HAS_INT128_T
    _uint128_t(const __uint128_t & src) noexcept
        : low((low_t)(src >> 64u)), high((high_t)(src & 0xFFFFFFFFFFFFFFFFull)) {}

    _uint128_t(const __int128_t & src) noexcept
        : low((low_t)((__uint128_t)src >> 64u)), high((high_t)(src & 0xFFFFFFFFFFFFFFFFull)) {}
#endif

    _uint128_t(const this_type & src) noexcept
        : low(src.low), high(src.high) {}

    this_type & operator = (const this_type & rhs) {
        this->low = rhs.low;
        this->high = rhs.high;
        return *this;
    }

    template <typename Integral>
    this_type & operator = (Integral rhs) {
        this->low = reinterpret_cast<low_t>(rhs);
        this->high = reinterpret_cast<high_t>(0);
        return *this;
    }

    bool is_zero() const {
        return (this->low == (low_t)0 && this->high == (high_t)0);
    }

    bool operator && (const this_type & rhs) const {
        return (!this->is_zero() && !rhs.is_zero());
    }

    bool operator || (const this_type & rhs) const {
        return (!this->is_zero() || !rhs.is_zero());
    }

    bool operator ! () const {
        return this->is_zero();
    }

    operator bool () const {
        return !this->is_zero();
    }

    this_type operator & (const this_type & rhs) const {
        return this_type(this->low & rhs.low, this->high & rhs.high);
    }

    this_type operator | (const this_type & rhs) const {
        return this_type(this->low | rhs.low, this->high | rhs.high);
    }

    this_type operator ^ (const this_type & rhs) const {
        return this_type(this->low ^ rhs.low, this->high ^ rhs.high);
    }

    this_type operator ~ () const {
        return this_type((low_t)(~(this->low)), (high_t)(~(this->high)));
    }

    this_type operator / (const this_type & rhs) const {
        return unsigned_128_div_128_to_128(*this, rhs);
    }

    this_type & operator &= (const this_type & rhs) {
        this->low &= rhs.low;
        this->high &= rhs.high;
        return *this;
    }

    this_type & operator |= (const this_type & rhs) {
        this->low |= rhs.low;
        this->high |= rhs.high;
        return *this;
    }

    this_type & operator ^= (const this_type & rhs) {
        this->low ^= rhs.low;
        this->high ^= rhs.high;
        return *this;
    }

    this_type & operator /= (const this_type & rhs) {
        *this = *this / rhs;
        return *this;
    }

    static inline
    uint32_t count_tail_zeros(integral_t val) {
        return BitUtils::bsf64(val);
    }

    static inline
    uint32_t count_leading_zeros(integral_t val) {
        return (63u - BitUtils::bsr64(val));
    }

    static inline
    uint32_t count_tail_zeros(const this_type & u128) {
        uint32_t tail_zeros;
        if (u128.low == 0)
            tail_zeros = (u128.high == 0) ? 0u : count_tail_zeros(u128.high);
        else
            tail_zeros = count_tail_zeros(u128.low);
        return tail_zeros;
    }

    static inline
    uint32_t count_leading_zeros(const this_type & u128) {
        uint32_t leading_zeros;
        if (u128.high == 0)
            leading_zeros = (u128.low == 0) ? 0u : count_leading_zeros(u128.low);
        else
            leading_zeros = count_leading_zeros(u128.high);
        return leading_zeros;
    }

    static
    JSTD_FORCE_INLINE
    integral_t unsigned_64_div_64_to_64(integral_t dividend, integral_t divisor) {
        return (dividend / divisor);
    }

    static
    JSTD_FORCE_INLINE
    integral_t unsigned_64_div_128_to_64(integral_t dividend, const this_type & divisor) {
        if (divisor.high == 0) {
            return (dividend / divisor.low);
        } else {
            return (integral_t)0;
        }
    }

    static
    JSTD_FORCE_INLINE
    integral_t unsigned_128_div_64_to_64(const this_type & dividend, integral_t divisor) {
        if (dividend.high != 0) {
            return 0ull;
        } else {
            return (dividend.low / divisor);
        }
    }

    //
    // q(64) = n(128) / d(128)
    //
    static inline
    integral_t unsigned_128_div_128_to_64(const this_type & dividend, const this_type & divisor) {
        if (dividend.high != 0) {
            if (divisor.high == 0) {
                // dividend.high != 0 && divisor.high == 0
                integral_t result_low = unsigned_128_div_64_to_64(dividend, divisor.low);
                return result_low;
            } else {
                // dividend.high != 0 && divisor.high != 0

                // Here divisor >= 2^64
                // We know that divisor.high != 0, so count leading zeros is OK
                // We have 0 <= d_leading_zeros <= 63
                uint32_t d_leading_zeros = count_leading_zeros(divisor.high);
                return 0;
            }
        } else {
            if (divisor.high == 0) {
                // dividend.high == 0 && divisor.high == 0
                return (dividend.low / divisor.low);
            } else {
                // dividend.high == 0 && divisor.high != 0
                return 0ull;
            }
        }
    }

    static
    JSTD_FORCE_INLINE
    this_type unsigned_128_div_64_to_128(const this_type & dividend, integral_t divisor) {
        if (dividend.high != 0) {
            // TODO: xxxxxx
            return this_type(0ull, 0ull);
        } else {
            return this_type(dividend.low / divisor, 0ull);
        }
    }

    //
    // q(128) = n(128) / d(128)
    //
    static inline
    this_type unsigned_128_div_128_to_128(const this_type & dividend, const this_type & divisor) {
        if (divisor.high == 0) {
            // dividend.high != 0 && divisor.high == 0
            return unsigned_128_div_64_to_128(dividend, divisor.low);
        } else {
            if (dividend.high != 0) {
                // dividend.high != 0 && divisor.high != 0
                // TODO: xxxxxx
                return this_type(0ull, 00ull);
            } else {
                if (divisor.high == 0) {
                    // dividend.high == 0 && divisor.high == 0
                    return this_type(0ull, dividend.low / divisor.low);
                } else {
                    // dividend.high == 0 && divisor.high != 0
                    return this_type(0, 0);
                }
            }
        }
    }
}; // class _uint128_t

} // namespace jstd

#endif // JSTD_UINT128_T_H
