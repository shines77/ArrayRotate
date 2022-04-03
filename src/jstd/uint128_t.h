
#ifndef JSTD_UINT128_T_H
#define JSTD_UINT128_T_H

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

#include <stdint.h>     // For __uint128_t
#include <stddef.h>
#include <stdbool.h>
#include <math.h>
#include <assert.h>

#include <cstdint>
#include <cstddef>
#include <cstdbool>

#include "jstd/stddef.h"
#include "jstd/BitUtils.h"
#include "jstd/DivUtils.h"

#ifdef _MSC_VER
#include <intrin.h>     // For _addcarry_u64(), available only in x64 mode
#endif

#if defined(JSTD_X86_64)
#include <immintrin.h>  // For _udiv128(), _div128(), _addcarry_u64()
#endif

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

#if defined(JSTD_X86_64) && (defined(_MSC_VER) || defined(__ICL))
#ifdef __cplusplus
extern "C"
#endif
uint64_t __udiv128(uint64_t low, uint64_t high, uint64_t divisor, uint64_t * remainder);
#endif // JSTD_X86_64 && (_MSC_VER || __ICL)

namespace jstd {

#if defined(__ICL)
//
// See: https://stackoverflow.com/questions/8453146/128-bit-division-intrinsic-in-visual-c
// See: https://www.thinbug.com/q/8453146
//

static
//__declspec(naked)
__declspec(regcall, naked)  // only for Intel C++ Compiler
inline uint64_t __fastcall _udiv128_fast(uint64_t dividend_low, uint64_t dividend_high, uint64_t divisor, uint64_t * remainder) {
    __asm {
        mov rax, rcx    ; Put the low digit in place (high is already there)
        div r8          ; 128 bit divide rdx-rax/r8 = rdx remainder, rax quotient
        mov [r9], rdx   ; Save the reminder
        ret             ; Return the quotient
    }
}

static
//__declspec(naked)
__declspec(regcall, naked)  // only for Intel C++ Compiler
inline uint64_t __fastcall _udiv128_icc(uint64_t dividend_high, uint64_t dividend_low, uint64_t divisor, uint64_t * remainder) {
    __asm {
        mov rax, rdx    ; Put the low digit in place
        mov rdx, rcx    ; Put the high digit in place
        div r8          ; 128 bit divide rdx-rax/r8 = rdx remainder, rax quotient
        mov [r9], rdx   ; Save the reminder
        ret             ; Return the quotient
    }
}

#endif // __ICL

struct _uint128_t {
    typedef _uint128_t  this_type;

    typedef uint64_t    integral_t;
    typedef uint64_t    unsigned_t;
    typedef int64_t     signed_t;

    typedef uint64_t    high_t;
    typedef uint64_t    low_t;

    low_t  low;
    high_t high;

    static const bool kIsSigned = false;

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

    this_type & operator = (int32_t rhs) {
        this->low = (low_t)rhs;
        this->high = 0;
        return *this;
    }

    this_type & operator = (uint32_t rhs) {
        this->low = (low_t)rhs;
        this->high = 0;
        return *this;
    }

    this_type & operator = (int64_t rhs) {
        this->low = (low_t)rhs;
        this->high = 0;
        return *this;
    }

    this_type & operator = (uint64_t rhs) {
        this->low = (low_t)rhs;
        this->high = 0;
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

    bool operator > (const this_type & rhs) const {
        return ((this->high > rhs.high) || ((this->high == rhs.high) && (this->low > rhs.low)));
    }

    bool operator >= (const this_type & rhs) const {
        return ((this->high > rhs.high) || ((this->high == rhs.high) && (this->low >= rhs.low)));
    }

    bool operator < (const this_type & rhs) const {
        return ((this->high < rhs.high) || ((this->high == rhs.high) && (this->low < rhs.low)));
    }

    bool operator <= (const this_type & rhs) const {
        return ((this->high < rhs.high) || ((this->high == rhs.high) && (this->low <= rhs.low)));
    }

    bool operator == (const this_type & rhs) const {
        return ((this->high == rhs.high) && (this->low == rhs.low));
    }

    bool operator != (const this_type & rhs) const {
        return ((this->high != rhs.high) || (this->low != rhs.low));
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

    operator uint64_t () const {
        return (uint64_t)this->low;
    }

    operator int64_t () const {
        return (int64_t)((this->high & 0x800000000000000ull) | (uint64_t)this->low);
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

    this_type operator + (const this_type & rhs) const {
        return bigint_128_add(*this, rhs);
    }

    this_type operator - (const this_type & rhs) const {
        return bigint_128_sub(*this, rhs);
    }

    this_type operator + (int rhs) const {
        return bigint_128_add(*this, (int64_t)rhs);
    }

    this_type operator - (int rhs) const {
        return bigint_128_sub(*this, (int64_t)rhs);
    }

    this_type operator + (uint32_t rhs) const {
        return bigint_128_add(*this, (uint64_t)rhs);
    }

    this_type operator - (uint32_t rhs) const {
        return bigint_128_sub(*this, (uint64_t)rhs);
    }

    this_type operator + (int64_t rhs) const {
        return bigint_128_add(*this, rhs);
    }

    this_type operator - (int64_t rhs) const {
        return bigint_128_sub(*this, rhs);
    }

    this_type operator + (uint64_t rhs) const {
        return bigint_128_add(*this, rhs);
    }

    this_type operator - (uint64_t rhs) const {
        return bigint_128_sub(*this, rhs);
    }

    this_type operator * (const this_type & rhs) const {
        return unsigned_128_mul_128_to_128(*this, rhs);
    }

    this_type operator / (const this_type & rhs) const {
        return unsigned_128_div_128_to_128(*this, rhs);
    }

    this_type operator >> (const int shift) const {
        return right_shift(*this, shift);
    }

    this_type operator << (const int shift) const {
        return left_shift(*this, shift);
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

    this_type & operator += (const this_type & rhs) {
        *this = *this + rhs;
        return *this;
    }

    this_type & operator -= (const this_type & rhs) {
        *this = *this - rhs;
        return *this;
    }

    this_type & operator *= (const this_type & rhs) {
        *this = *this * rhs;
        return *this;
    }

    this_type & operator /= (const this_type & rhs) {
        *this = *this / rhs;
        return *this;
    }

    this_type &  operator >>= (const int shift) {
        *this = *this >> shift;
        return *this;
    }

    this_type &  operator <<= (const int shift) {
        *this = *this << shift;
        return *this;
    }

    static inline
    int count_tail_zeros(integral_t val) {
        return (int)BitUtils::bsf64(val);
    }

    static inline
    int count_leading_zeros(integral_t val) {
        return (63 - (int)BitUtils::bsr64(val));
    }

    static inline
    int count_tail_zeros(const this_type & u128) {
        int tail_zeros;
        if (u128.low == 0)
            tail_zeros = (u128.high == 0) ? 0 : (count_tail_zeros(u128.high) + 64);
        else
            tail_zeros = count_tail_zeros(u128.low);
        return tail_zeros;
    }

    static inline
    int count_leading_zeros(const this_type & u128) {
        int leading_zeros;
        if (u128.high == 0)
            leading_zeros = (u128.low == 0) ? 0 : (count_leading_zeros(u128.low) + 64);
        else
            leading_zeros = count_leading_zeros(u128.high);
        return leading_zeros;
    }

    static inline
    this_type logic_left_shift(const this_type & src, const int shift) {
        assert(shift >= 0 && shift < 128);
        this_type result = src;
        if (shift <= 64) {
            int low_rshift = (64 - shift);
            result.high = ((unsigned_t)result.high << shift) | ((unsigned_t)result.low >> low_rshift);
            result.low <<= shift;
        } else {
            int low_lshift = (shift - 64);
            result.high = (unsigned_t)result.low << low_lshift;
            result.low = 0;
        }
        return result;
    }

    static inline
    this_type logic_right_shift(const this_type & src, const int shift) {
        assert(shift >= 0 && shift < 128);
        this_type result = src;
        if (shift <= 64) {
            int high_lshift = (64 - shift);
            result.low = ((unsigned_t)result.low >> shift) | ((unsigned_t)result.high << high_lshift);
            result.high >>= shift;
        } else {
            int high_rshift = (shift - 64);
            result.low = (unsigned_t)result.high >> high_rshift;
            result.high = 0;
        }
        return result;
    }

    static inline
    this_type sign_extend_left_shift(const this_type & src, const int shift) {
        assert(shift >= 0 && shift <= 128);
        this_type result = src;
        if (shift <= 64) {
            int low_rshift = (64 - shift);
            result.high = ((signed_t)result.high << shift) | ((unsigned_t)result.low >> low_rshift);
            result.low <<= shift;
        } else {
            int low_lshift = (shift - 64);
            result.high = (result.high & 0x800000000000000ull) | ((unsigned_t)result.low << low_lshift);
            result.low = 0;
        }
        return result;
    }

    static inline
    this_type sign_extend_right_shift(const this_type & src, const int shift) {
        assert(shift >= 0 && shift <= 128);
        this_type result = src;
        if (shift <= 64) {
            int high_lshift = (64 - shift);
            result.low = ((unsigned_t)result.low >> shift) | ((signed_t)result.high << high_lshift);
            result.high >>= shift;
        } else {
            int high_rshift = (shift - 64);
            result.low = (unsigned_t)(result.high & 0x7FFFFFFFFFFFFFFFull) >> high_rshift;
            result.high = result.high & 0x800000000000000ull;
        }
        return result;
    }

    static inline
    this_type left_shift(const this_type & src, const int shift) {
        if (kIsSigned)
            return sign_extend_left_shift(src, shift);
        else
            return logic_left_shift(src, shift);
    }

    static inline
    this_type right_shift(const this_type & src, const int shift) {
        if (kIsSigned)
            return sign_extend_right_shift(src, shift);
        else
            return logic_right_shift(src, shift);
    }

#if defined(JSTD_X86_I386)
    static inline
    unsigned char _addcarry_u64(unsigned char c_in, unsigned __int64 a, unsigned __int64 b, unsigned __int64 * out) {
        unsigned __int64 total_a = a + (c_in > 0 ? 1 : 0);
        unsigned __int64 max_n = (total_a >= b) ? total_a : b;
        unsigned __int64 sum = total_a + b;
        unsigned char carry = (sum >= max_n) ? 0 : 1;
        *out = sum;
        return carry;
    }

    static inline
    unsigned char _subborrow_u64(unsigned char c_in, unsigned __int64 a, unsigned __int64 b, unsigned __int64 * out) {
        unsigned __int64 min_n = (a <= b) ? a : b;
        unsigned __int64 total_b = b + (c_in > 0 ? 1 : 0);
        unsigned char borrow = (a >= total_b) ? 0 : 1;
        *out = a - total_b;
        return borrow;
    }
#endif // JSTD_X86_I386

    static inline
    this_type bigint_128_add(const this_type & a, const this_type & b) {
        this_type result;
        unsigned char carry = _addcarry_u64(0, a.low, b.low, (uint64_t *)&result.low);
        result.high = a.high + b.high + carry;
        return result;
    }

    static inline
    this_type bigint_128_sub(const this_type & a, const this_type & b) {
        this_type result;
        unsigned char borrow = _subborrow_u64(0, a.low, b.low, (uint64_t *)&result.low);
        result.high = a.high - b.high - borrow;
        return result;
    }

    static inline
    this_type bigint_128_add(const this_type & a, uint64_t b) {
        this_type result;
        unsigned char carry = _addcarry_u64(0, a.low, b, (uint64_t *)&result.low);
        result.high = a.high + carry;
        return result;
    }

    static inline
    this_type bigint_128_sub(const this_type & a, uint64_t b) {
        this_type result;
        unsigned char borrow = _subborrow_u64(0, a.low, b, (uint64_t *)&result.low);
        result.high = a.high - borrow;
        return result;
    }

    static inline
    this_type bigint_128_add(const this_type & a, int64_t b) {
        this_type result;
        unsigned char carry = _addcarry_u64(0, a.low, b, (uint64_t *)&result.low);
        result.high = a.high + (b >= 0) ? carry : -carry;
        return result;
    }

    static inline
    this_type bigint_128_sub(const this_type & a, int64_t b) {
        this_type result;
        unsigned char borrow = _subborrow_u64(0, a.low, b, (uint64_t *)&result.low);
        result.high = a.high - (b >= 0) ? borrow : -borrow;
        return result;
    }

#if defined(_MSC_VER) || defined(__ICL)

    static inline
    int u64_distance(uint64_t dividend, const uint64_t divisor) {
        int n_leading_zeros = count_leading_zeros(dividend);
        int d_leading_zeros = count_leading_zeros(divisor);
        return (d_leading_zeros - n_leading_zeros);
    }

    static inline
    int u128_distance(const _uint128_t & dividend, const _uint128_t & divisor) {
        int n_leading_zeros = count_leading_zeros(dividend);
        int d_leading_zeros = count_leading_zeros(divisor);
        return (d_leading_zeros - n_leading_zeros);
    }

    static inline
    uint64_t __udivmodti4_64(uint64_t dividend, uint64_t divisor, uint64_t * remainder) {
        if (divisor > dividend) {
            if (remainder != nullptr) {
                *remainder = dividend;
            }
            return 0;
        }

        // Calculate the distance between most significant bits, 128 > shift >= 0.
        int shift = u64_distance(dividend, divisor);
        divisor <<= shift;

        uint64_t quotient = 0;
        for (; shift >= 0; --shift) {
            quotient <<= 1;
            if (dividend >= divisor) {
                dividend -= divisor;
                quotient |= 1;
            }
            divisor >>= 1;
        }

        if (remainder != nullptr) {
            *remainder = dividend;
        }
        return quotient;
    }

    //
    // See: https://danlark.org/2020/06/14/128-bit-division/
    //
    // clang compiler-rt:
    //      https://github.com/llvm-mirror/compiler-rt/blob/release_90/lib/builtins/udivmodti4.c#L20
    //
    static inline
    _uint128_t __udivmodti4(_uint128_t dividend, _uint128_t divisor, _uint128_t * remainder) {
        if (divisor > dividend) {
            if (remainder != nullptr) {
                *remainder = dividend;
            }
            return 0;
        }

        // Calculate the distance between most significant bits, 128 > shift >= 0.
        int shift = u128_distance(dividend, divisor);
        divisor <<= shift;

        _uint128_t quotient = 0;
        for (; shift >= 0; --shift) {
            quotient <<= 1;
            if (dividend >= divisor) {
                dividend -= divisor;
                quotient |= 1;
            }
            divisor >>= 1;
        }

        if (remainder != nullptr) {
            *remainder = dividend;
        }
        return quotient;
    }

    static inline
    _uint128_t __umodti3(_uint128_t a, _uint128_t b) {
        _uint128_t r;
        __udivmodti4(a, b, &r);
        return r;
    }

    // Returns: a % b

    static inline
    int64_t __modti3(int64_t a, int64_t b) {
        const int bits_in_tword_m1 = (int)(sizeof(int64_t) * CHAR_BIT) - 1;
        int64_t s = b >> bits_in_tword_m1;  // s = b < 0 ? -1 : 0
        b = (b ^ s) - s;                    // negate if s == -1
        s = a >> bits_in_tword_m1;          // s = a < 0 ? -1 : 0
        a = (a ^ s) - s;                    // negate if s == -1
        uint64_t r;
        __udivmodti4_64(a, b, &r);
        return ((int64_t)r ^ s) - s;        // negate if s == -1
    }

    // Returns: a / b

    static inline
    _uint128_t __udivti3(_uint128_t a, _uint128_t b) {
        return __udivmodti4(a, b, nullptr);
    }

    static inline
    int64_t __divti3(int64_t a, int64_t b) {
        const int bits_in_tword_m1 = (int)(sizeof(int64_t) * CHAR_BIT) - 1;
        int64_t s_a = a >> bits_in_tword_m1;                    // s_a = (a < 0) ? -1 : 0
        int64_t s_b = b >> bits_in_tword_m1;                    // s_b = (b < 0) ? -1 : 0
        a = (a ^ s_a) - s_a;                                    // negate if s_a == -1
        b = (b ^ s_b) - s_b;                                    // negate if s_b == -1
        s_a ^= s_b;                                             // sign of quotient
        return (__udivmodti4_64(a, b, nullptr) ^ s_a) - s_a;       // negate if s_a == -1
    }

#endif // _MSC_VER || __ICL

    static
    JSTD_FORCE_INLINE
    integral_t unsigned_64_div_64_to_64(integral_t dividend, integral_t divisor) {
        return (dividend / divisor);
    }

    static
    JSTD_FORCE_INLINE
    integral_t unsigned_64_div_128_to_64(integral_t dividend, const this_type & divisor) {
        return ((divisor.high != 0) ? 0 : (dividend / divisor.low));
    }

    static
    JSTD_FORCE_INLINE
    integral_t unsigned_128_div_64_to_64(const this_type & dividend, integral_t divisor) {
        // N.B. resist the temptation to use __uint128_t here.
        // In LLVM compiler-rt, it performs a 128/128 -> 128 division which is many times slower than
        // necessary. In gcc it's better but still slower than the divlu implementation, perhaps because
        // it's not JSTD_FORCE_INLINE.
#if defined(JSTD_X86_64) && defined(JSTD_GCC_STYLE_ASM)
        uint64_t result, remainder;
        UNUSED_VARIANT(remainder);
        __asm__("divq %[v]" : "=a"(result), "=d"(remainder) : [v] "r"(divisor), "a"(dividend.low), "d"(dividend.high));
        return (integral_t)result;
#elif defined(JSTD_X86_64) && defined(_MSC_VER) && (_MSC_VER >= 1920)
        /////////////////////////////////////////////////////////
        //
        // See: https://stackoverflow.com/questions/1870158/unsigned-128-bit-division-on-64-bit-machine/60013652#60013652
        // See: https://docs.microsoft.com/en-us/cpp/intrinsics/udiv64?view=msvc-170
        //
        // Since Visual Studio 2019 RTM (_MSC_VER = 1920)
        //
        // Defined in <immintrin.h>
        //
        // unsigned __int64 _udiv128(
        //    unsigned __int64 highDividend,
        //    unsigned __int64 lowDividend,
        //    unsigned __int64 divisor,
        //    unsigned __int64 *remainder
        // );
        //
        /////////////////////////////////////////////////////////
        //
        // Defined in <stdlib.h> /* C++ only */
        //
        // ldiv_t div(
        //    long numer,
        //    long denom
        // );
        //
        // lldiv_t div(
        //    long long numer,
        //    long long denom
        // );
        //
        // See: https://docs.microsoft.com/zh-cn/cpp/c-runtime-library/reference/imaxdiv?view=msvc-160
        //
        // Defined in <inttypes.h>
        //
        // imaxdiv_t imaxdiv(
        //     intmax_t numer,
        //     intmax_t denom
        // );
        //
        // See: https://stackoverflow.com/questions/29229371/addcarry-u64-and-addcarryx-u64-with-msvc-and-icc
        // See: https://docs.microsoft.com/en-us/previous-versions/hh977022(v=vs.140)?redirectedfrom=MSDN
        //
        // #include <immintrin.h>
        //
        // uint8_t _addcarry_u64(uint8_t c_in, uint64_t a, uint64_t b);
        // uint8_t _subborrow_u64(uint8_t c_in, uint64_t a, uint64_t b);
        //
        // _addcarryx_u64() for AVX2 since Broadwell
        //
        /////////////////////////////////////////////////////////
        uint64_t remainder;
        uint64_t quotient = _udiv128(dividend.high, dividend.low, divisor, &remainder);
        return (integral_t)quotient;
#elif defined(JSTD_X86_64) && (defined(_MSC_VER) || defined(__ICL))
#if 1
        uint64_t remainder;
        uint64_t quotient = __udiv128(dividend.low, dividend.high, divisor, &remainder);
        return (integral_t)quotient;
#else
        this_type quotient = __udivti3(dividend, divisor);
        return (integral_t)quotient.low;
#endif
#else
        if (dividend.high != 0) {
            // Check for overflow and divide by 0.
            if (dividend.high < divisor) {
                // TODO: xxxxxx
                if (divisor != 0) {
                    int gcd_pow2 = count_leading_zeros(divisor);
                } else {
                    throw std::runtime_error("_uint128_t::unsigned_128_div_64_to_64(): divisor is zero.");
                }
            } else {
                return integral_t(~0ull);
            }
        } else {
            return (dividend.low / divisor);
        }
#endif
    }

    //
    // q(64) = n(128) / d(128)
    //
    static inline
    integral_t unsigned_128_div_128_to_64(const this_type & dividend, const this_type & divisor) {
        if (divisor.high == 0) {
            // dividend.high != 0 && divisor.high == 0
            return unsigned_128_div_64_to_64(dividend, divisor.low);
        } else {
            if (dividend.high != 0) {
                // dividend.high != 0 && divisor.high != 0
                // TODO: xxxxxx

                // Here divisor >= 2^64
                // We know that divisor.high != 0, so count leading zeros is OK
                // We have 0 <= d_leading_zeros <= 63
                uint32_t d_leading_zeros = count_leading_zeros(divisor.high);

                return this_type(UINT64_C(0), UINT64_C(0));
            } else {
                if (divisor.high == 0) {
                    // dividend.high == 0 && divisor.high == 0
                    return this_type(UINT64_C(0), dividend.low / divisor.low);
                } else {
                    // dividend.high == 0 && divisor.high != 0
                    return this_type(0, 0);
                }
            }
        }
    }

    static
    JSTD_FORCE_INLINE
    this_type unsigned_128_div_64_to_128(const this_type & dividend, integral_t divisor) {
        if (dividend.high != 0) {
            // TODO: xxxxxx
#if defined(JSTD_X86_64) && (defined(_MSC_VER) || defined(__ICL))
            this_type quotient = __udivti3(dividend, divisor);
            return quotient;
#else
            return this_type(UINT64_C(0), UINT64_C(0));
#endif
        } else {
            return this_type(dividend.low / divisor, UINT64_C(0));
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
                return this_type(UINT64_C(0), UINT64_C(0));
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

    static inline
    this_type unsigned_128_mul_128_to_128(const this_type & multiplicand, const this_type & multiplier) {
        this_type product;
        product.low  = multiplicand.low * multiplier.low;
        product.high = mul128_high_u64(multiplicand.low, multiplier.low);
        return product;
    }
}; // class _uint128_t

} // namespace jstd

#endif // JSTD_UINT128_T_H
