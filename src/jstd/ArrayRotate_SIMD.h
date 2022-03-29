
#ifndef JSTD_ARRAY_ROTATE_SIMD_H
#define JSTD_ARRAY_ROTATE_SIMD_H

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

#include "jstd/BitVec.h"
#include "jstd/stddef.h"

#include <assert.h>
#include <cstdint>
#include <cstddef>
#include <cstdbool>
#include <algorithm>
#include <type_traits>

namespace jstd {
namespace simd {

static const std::size_t kSSERegBytes = 16;
static const std::size_t kAVXRegBytes = 32;

static const std::size_t kSSERegCount = 8;
static const std::size_t kAVXRegCount = 16;

static const std::size_t kSSEAlignment = kSSERegBytes;
static const std::size_t kAVXAlignment = kAVXRegBytes;

static const std::size_t kSSEAlignMask = kSSEAlignment - 1;
static const std::size_t kAVXAlignMask = kAVXAlignment - 1;

static const std::size_t kRotateThresholdLength = 32;
static const std::size_t kMaxAVXStashBytes = (kAVXRegCount - 4) * kAVXRegBytes;

template <typename T>
static inline
T * left_rotate_simple_impl(T * first, T * mid, T * last,
                            std::size_t left_len, std::size_t right_len)
{
    typedef T           value_type;
    typedef T *         pointer;
    typedef const T *   const_pointer;

    pointer result = first + right_len;

    do {
        if (left_len <= right_len) {
            pointer read = mid;
            pointer write = first;
            if (left_len != 1) {
                while (read != last) {
                    std::swap(*write, *read);
                    ++write;
                    ++read;
                }
                right_len %= left_len;
                first = write;
                left_len -= right_len;
                mid = last - right_len;
                if (right_len == 0 || left_len == 0)
                    break;
            } else {
                value_type tmp(std::move(*write));
                while (read != last) {
                    *write = *read;
                    ++write;
                    ++read;
                }
                *write = std::move(tmp);
                break;
            }
        } else {
            pointer read = mid;
            pointer write = last;
            if (right_len != 1) {
                while (read != first) {
                    --write;
                    --read;
                    std::swap(*read, *write);
                }
                left_len %= right_len;
                last = write;
                right_len -= left_len;
                mid = first + left_len;
                if (left_len == 0 || right_len == 0)
                    break;
            } else {
                value_type tmp(std::move(*read));
                while (read != first) {
                    --write;
                    --read;
                    *write = *read;
                }
                *read = std::move(tmp);
                break;
            }
        }
    } while (1);

    return result;
}

template <typename T>
T * left_rotate_simple(T * first, T * mid, T * last)
{
    typedef T           value_type;
    typedef T *         pointer;
    typedef const T *   const_pointer;

    std::size_t left_len = std::size_t(mid - first);
    if (left_len == 0) return first;

    std::size_t right_len = std::size_t(last - mid);
    if (right_len == 0) return last;

    return left_rotate_simple_impl(first, mid, last, left_len, right_len);
}

template <typename T>
T * left_rotate_simple(T * data, std::size_t length, std::size_t offset)
{
    typedef T           value_type;
    typedef T *         pointer;
    typedef const T *   const_pointer;

    pointer first = data;
    pointer mid   = data + offset;
    pointer last  = data + length;

    std::size_t left_len = offset;
    if (left_len == 0) return first;

    std::size_t right_len = (offset <= length) ? (length - offset) : 0;
    if (right_len == 0) return last;

    return left_rotate_simple_impl(first, mid, last, left_len, right_len);
}

template <typename T>
inline
T * rotate_simple(T * data, std::size_t length, std::size_t offset)
{
    JSTD_ASSERT((offset <= length), "simd::rotate_simple(): (offset > length)");
    return left_rotate_simple(data, length, offset);
}

template <typename T>
inline
T * rotate_simple(T * first, T * mid, T * last)
{
    JSTD_ASSERT((last >= mid), "simd::rotate_simple(): (last < mid)");
    JSTD_ASSERT((mid >= first), "simd::rotate_simple(): (mid < first)");
    return left_rotate_simple(first, std::size_t(last - first), std::size_t(mid - first));
}

template <typename T>
static inline
void avx_forward_move_6(T * first, T * mid, T * last)
{
    typedef T           value_type;
    typedef T *         pointer;
    typedef const T *   const_pointer;

    static const std::size_t kValueSize = sizeof(value_type);
    static const bool kValueSizeIsPower2 = ((kValueSize & (kValueSize - 1)) == 0);

    static const std::size_t kPerStepBytes = 6 * kAVXRegBytes;

    std::size_t unAlignedBytes = (std::size_t)mid & kAVXAlignMask;
    bool sourceAddrCanAlign = (((unAlignedBytes / kValueSize) * kValueSize) == unAlignedBytes);
    if (kValueSizeIsPower2 && sourceAddrCanAlign) {
        while (unAlignedBytes != 0) {
            *first++ = *mid++;
            unAlignedBytes -= kValueSize;
        }

        char * target = (char *)first;
        char * source = (char *)mid;
        char * end = (char *)last;

        std::size_t lastUnalignedBytes = (std::size_t)last % kPerStepBytes;
        std::size_t totalBytes = (last - first) * kValueSize;
        const char * limit = (totalBytes >= kPerStepBytes) ? (end - lastUnalignedBytes) : source;

        while (source < limit) {
            __m256i ymm0 = _mm256_load_si256((const __m256i *)(source + 32 * 0));
            __m256i ymm1 = _mm256_load_si256((const __m256i *)(source + 32 * 1));
            __m256i ymm2 = _mm256_load_si256((const __m256i *)(source + 32 * 2));
            __m256i ymm3 = _mm256_load_si256((const __m256i *)(source + 32 * 3));
            __m256i ymm4 = _mm256_load_si256((const __m256i *)(source + 32 * 4));
            __m256i ymm5 = _mm256_load_si256((const __m256i *)(source + 32 * 5));

            _mm256_storeu_si256((__m256i *)(target + 32 * 0), ymm0);
            _mm256_storeu_si256((__m256i *)(target + 32 * 1), ymm1);
            _mm256_storeu_si256((__m256i *)(target + 32 * 2), ymm2);
            _mm256_storeu_si256((__m256i *)(target + 32 * 3), ymm3);
            _mm256_storeu_si256((__m256i *)(target + 32 * 4), ymm4);
            _mm256_storeu_si256((__m256i *)(target + 32 * 5), ymm5);

            source += kPerStepBytes;
            target += kPerStepBytes;
        }

        lastUnalignedBytes = (std::size_t)end & kAVXAlignMask;
        limit = end - lastUnalignedBytes;

        if (source + (4 * kAVXRegBytes) <= limit) {
            __m256i ymm0 = _mm256_load_si256((const __m256i *)(source + 32 * 0));
            __m256i ymm1 = _mm256_load_si256((const __m256i *)(source + 32 * 1));
            __m256i ymm2 = _mm256_load_si256((const __m256i *)(source + 32 * 2));
            __m256i ymm3 = _mm256_load_si256((const __m256i *)(source + 32 * 3));

            _mm256_storeu_si256((__m256i *)(target + 32 * 0), ymm0);
            _mm256_storeu_si256((__m256i *)(target + 32 * 1), ymm1);
            _mm256_storeu_si256((__m256i *)(target + 32 * 2), ymm2);
            _mm256_storeu_si256((__m256i *)(target + 32 * 3), ymm3);

            source += 4 * kAVXRegBytes;
            target += 4 * kAVXRegBytes;
        }

        if (source + (2 * kAVXRegBytes) <= limit) {
            __m256i ymm0 = _mm256_load_si256((const __m256i *)(source + 32 * 0));
            __m256i ymm1 = _mm256_load_si256((const __m256i *)(source + 32 * 1));

            _mm256_storeu_si256((__m256i *)(target + 32 * 0), ymm0);
            _mm256_storeu_si256((__m256i *)(target + 32 * 1), ymm1);

            source += 2 * kAVXRegBytes;
            target += 2 * kAVXRegBytes;
        }

        if (source + (1 * kAVXRegBytes) <= limit) {
            __m256i ymm0 = _mm256_load_si256((const __m256i *)(source + 32 * 0));

            _mm256_storeu_si256((__m256i *)(target + 32 * 0), ymm0);

            source += 1 * kAVXRegBytes;
            target += 1 * kAVXRegBytes;
        }

        while (source < end) {
            *target = *source;
            source += kValueSize;
            target += kValueSize;
        }
    }
    else {
        char * target = (char *)first;
        char * source = (char *)mid;
        char * end = (char *)last;

        std::size_t lastUnalignedBytes = (std::size_t)last % kPerStepBytes;
        std::size_t totalBytes = (last - first) * kValueSize;
        const char * limit = (totalBytes >= kPerStepBytes) ? (end - lastUnalignedBytes) : source;

        while (source < limit) {
            __m256i ymm0 = _mm256_loadu_si256((const __m256i *)(source + 32 * 0));
            __m256i ymm1 = _mm256_loadu_si256((const __m256i *)(source + 32 * 1));
            __m256i ymm2 = _mm256_loadu_si256((const __m256i *)(source + 32 * 2));
            __m256i ymm3 = _mm256_loadu_si256((const __m256i *)(source + 32 * 3));
            __m256i ymm4 = _mm256_loadu_si256((const __m256i *)(source + 32 * 4));
            __m256i ymm5 = _mm256_loadu_si256((const __m256i *)(source + 32 * 5));
            __m256i ymm6 = _mm256_loadu_si256((const __m256i *)(source + 32 * 6));
            __m256i ymm7 = _mm256_loadu_si256((const __m256i *)(source + 32 * 7));

            _mm256_storeu_si256((__m256i *)(target + 32 * 0), ymm0);
            _mm256_storeu_si256((__m256i *)(target + 32 * 1), ymm1);
            _mm256_storeu_si256((__m256i *)(target + 32 * 2), ymm2);
            _mm256_storeu_si256((__m256i *)(target + 32 * 3), ymm3);
            _mm256_storeu_si256((__m256i *)(target + 32 * 4), ymm4);
            _mm256_storeu_si256((__m256i *)(target + 32 * 5), ymm5);
            _mm256_storeu_si256((__m256i *)(target + 32 * 6), ymm6);
            _mm256_storeu_si256((__m256i *)(target + 32 * 7), ymm7);

            source += kPerStepBytes;
            target += kPerStepBytes;
        }

        lastUnalignedBytes = (std::size_t)end & kAVXAlignMask;
        limit = end - lastUnalignedBytes;

        if (source + (4 * kAVXRegBytes) <= limit) {
            __m256i ymm0 = _mm256_loadu_si256((const __m256i *)(source + 32 * 0));
            __m256i ymm1 = _mm256_loadu_si256((const __m256i *)(source + 32 * 1));
            __m256i ymm2 = _mm256_loadu_si256((const __m256i *)(source + 32 * 2));
            __m256i ymm3 = _mm256_loadu_si256((const __m256i *)(source + 32 * 3));

            _mm256_storeu_si256((__m256i *)(target + 32 * 0), ymm0);
            _mm256_storeu_si256((__m256i *)(target + 32 * 1), ymm1);
            _mm256_storeu_si256((__m256i *)(target + 32 * 2), ymm2);
            _mm256_storeu_si256((__m256i *)(target + 32 * 3), ymm3);

            source += 4 * kAVXRegBytes;
            target += 4 * kAVXRegBytes;
        }

        if (source + (2 * kAVXRegBytes) <= limit) {
            __m256i ymm0 = _mm256_loadu_si256((const __m256i *)(source + 32 * 0));
            __m256i ymm1 = _mm256_loadu_si256((const __m256i *)(source + 32 * 1));

            _mm256_storeu_si256((__m256i *)(target + 32 * 0), ymm0);
            _mm256_storeu_si256((__m256i *)(target + 32 * 1), ymm1);

            source += 2 * kAVXRegBytes;
            target += 2 * kAVXRegBytes;
        }

        if (source + (1 * kAVXRegBytes) <= limit) {
            __m256i ymm0 = _mm256_loadu_si256((const __m256i *)(source + 32 * 0));

            _mm256_storeu_si256((__m256i *)(target + 32 * 0), ymm0);

            source += 1 * kAVXRegBytes;
            target += 1 * kAVXRegBytes;
        }

        while (lastUnalignedBytes != 0) {
            *target = *source;
            source += kValueSize;
            target += kValueSize;
            lastUnalignedBytes -= kValueSize;
        }
    }
}

template <typename T>
static inline
void avx_forward_move_8(T * first, T * mid, T * last)
{
    typedef T           value_type;
    typedef T *         pointer;
    typedef const T *   const_pointer;

    static const std::size_t kValueSize = sizeof(value_type);
    static const bool kValueSizeIsPower2 = ((kValueSize & (kValueSize - 1)) == 0);

    static const std::size_t kPerStepBytes = 8 * kAVXRegBytes;

    std::size_t unAlignedBytes = (std::size_t)mid & kAVXAlignMask;
    bool sourceAddrCanAlign = (((unAlignedBytes / kValueSize) * kValueSize) == unAlignedBytes);
    if (kValueSizeIsPower2 && sourceAddrCanAlign) {
        while (unAlignedBytes != 0) {
            *first++ = *mid++;
            unAlignedBytes -= kValueSize;
        }

        char * target = (char *)first;
        char * source = (char *)mid;
        char * end = (char *)last;

        std::size_t lastUnalignedBytes = (std::size_t)last % kPerStepBytes;
        std::size_t totalBytes = (last - first) * kValueSize;
        const char * limit = (totalBytes >= kPerStepBytes) ? (end - lastUnalignedBytes) : source;

        while (source < limit) {
            __m256i ymm0 = _mm256_load_si256((const __m256i *)(source + 32 * 0));
            __m256i ymm1 = _mm256_load_si256((const __m256i *)(source + 32 * 1));
            __m256i ymm2 = _mm256_load_si256((const __m256i *)(source + 32 * 2));
            __m256i ymm3 = _mm256_load_si256((const __m256i *)(source + 32 * 3));
            __m256i ymm4 = _mm256_load_si256((const __m256i *)(source + 32 * 4));
            __m256i ymm5 = _mm256_load_si256((const __m256i *)(source + 32 * 5));
            __m256i ymm6 = _mm256_load_si256((const __m256i *)(source + 32 * 6));
            __m256i ymm7 = _mm256_load_si256((const __m256i *)(source + 32 * 7));

            _mm256_storeu_si256((__m256i *)(target + 32 * 0), ymm0);
            _mm256_storeu_si256((__m256i *)(target + 32 * 1), ymm1);
            _mm256_storeu_si256((__m256i *)(target + 32 * 2), ymm2);
            _mm256_storeu_si256((__m256i *)(target + 32 * 3), ymm3);
            _mm256_storeu_si256((__m256i *)(target + 32 * 4), ymm4);
            _mm256_storeu_si256((__m256i *)(target + 32 * 5), ymm5);
            _mm256_storeu_si256((__m256i *)(target + 32 * 6), ymm6);
            _mm256_storeu_si256((__m256i *)(target + 32 * 7), ymm7);

            source += kPerStepBytes;
            target += kPerStepBytes;
        }

        lastUnalignedBytes = (std::size_t)end & kAVXAlignMask;
        limit = end - lastUnalignedBytes;

        if (source + (4 * kAVXRegBytes) <= limit) {
            __m256i ymm0 = _mm256_load_si256((const __m256i *)(source + 32 * 0));
            __m256i ymm1 = _mm256_load_si256((const __m256i *)(source + 32 * 1));
            __m256i ymm2 = _mm256_load_si256((const __m256i *)(source + 32 * 2));
            __m256i ymm3 = _mm256_load_si256((const __m256i *)(source + 32 * 3));

            _mm256_storeu_si256((__m256i *)(target + 32 * 0), ymm0);
            _mm256_storeu_si256((__m256i *)(target + 32 * 1), ymm1);
            _mm256_storeu_si256((__m256i *)(target + 32 * 2), ymm2);
            _mm256_storeu_si256((__m256i *)(target + 32 * 3), ymm3);

            source += 4 * kAVXRegBytes;
            target += 4 * kAVXRegBytes;
        }

        if (source + (2 * kAVXRegBytes) <= limit) {
            __m256i ymm0 = _mm256_load_si256((const __m256i *)(source + 32 * 0));
            __m256i ymm1 = _mm256_load_si256((const __m256i *)(source + 32 * 1));

            _mm256_storeu_si256((__m256i *)(target + 32 * 0), ymm0);
            _mm256_storeu_si256((__m256i *)(target + 32 * 1), ymm1);

            source += 2 * kAVXRegBytes;
            target += 2 * kAVXRegBytes;
        }

        if (source + (1 * kAVXRegBytes) <= limit) {
            __m256i ymm0 = _mm256_load_si256((const __m256i *)(source + 32 * 0));

            _mm256_storeu_si256((__m256i *)(target + 32 * 0), ymm0);

            source += 1 * kAVXRegBytes;
            target += 1 * kAVXRegBytes;
        }

        while (source < end) {
            *target = *source;
            source += kValueSize;
            target += kValueSize;
        }
    }
    else {
        char * target = (char *)first;
        char * source = (char *)mid;
        char * end = (char *)last;

        std::size_t lastUnalignedBytes = (std::size_t)last % kPerStepBytes;
        std::size_t totalBytes = (last - first) * kValueSize;
        const char * limit = (totalBytes >= kPerStepBytes) ? (end - lastUnalignedBytes) : source;

        while (source < limit) {
            __m256i ymm0 = _mm256_loadu_si256((const __m256i *)(source + 32 * 0));
            __m256i ymm1 = _mm256_loadu_si256((const __m256i *)(source + 32 * 1));
            __m256i ymm2 = _mm256_loadu_si256((const __m256i *)(source + 32 * 2));
            __m256i ymm3 = _mm256_loadu_si256((const __m256i *)(source + 32 * 3));
            __m256i ymm4 = _mm256_loadu_si256((const __m256i *)(source + 32 * 4));
            __m256i ymm5 = _mm256_loadu_si256((const __m256i *)(source + 32 * 5));
            __m256i ymm6 = _mm256_loadu_si256((const __m256i *)(source + 32 * 6));
            __m256i ymm7 = _mm256_loadu_si256((const __m256i *)(source + 32 * 7));

            _mm256_storeu_si256((__m256i *)(target + 32 * 0), ymm0);
            _mm256_storeu_si256((__m256i *)(target + 32 * 1), ymm1);
            _mm256_storeu_si256((__m256i *)(target + 32 * 2), ymm2);
            _mm256_storeu_si256((__m256i *)(target + 32 * 3), ymm3);
            _mm256_storeu_si256((__m256i *)(target + 32 * 4), ymm4);
            _mm256_storeu_si256((__m256i *)(target + 32 * 5), ymm5);
            _mm256_storeu_si256((__m256i *)(target + 32 * 6), ymm6);
            _mm256_storeu_si256((__m256i *)(target + 32 * 7), ymm7);

            source += kPerStepBytes;
            target += kPerStepBytes;
        }

        lastUnalignedBytes = (std::size_t)end & kAVXAlignMask;
        limit = end - lastUnalignedBytes;

        if (source + (4 * kAVXRegBytes) <= limit) {
            __m256i ymm0 = _mm256_loadu_si256((const __m256i *)(source + 32 * 0));
            __m256i ymm1 = _mm256_loadu_si256((const __m256i *)(source + 32 * 1));
            __m256i ymm2 = _mm256_loadu_si256((const __m256i *)(source + 32 * 2));
            __m256i ymm3 = _mm256_loadu_si256((const __m256i *)(source + 32 * 3));

            _mm256_storeu_si256((__m256i *)(target + 32 * 0), ymm0);
            _mm256_storeu_si256((__m256i *)(target + 32 * 1), ymm1);
            _mm256_storeu_si256((__m256i *)(target + 32 * 2), ymm2);
            _mm256_storeu_si256((__m256i *)(target + 32 * 3), ymm3);

            source += 4 * kAVXRegBytes;
            target += 4 * kAVXRegBytes;
        }

        if (source + (2 * kAVXRegBytes) <= limit) {
            __m256i ymm0 = _mm256_loadu_si256((const __m256i *)(source + 32 * 0));
            __m256i ymm1 = _mm256_loadu_si256((const __m256i *)(source + 32 * 1));

            _mm256_storeu_si256((__m256i *)(target + 32 * 0), ymm0);
            _mm256_storeu_si256((__m256i *)(target + 32 * 1), ymm1);

            source += 2 * kAVXRegBytes;
            target += 2 * kAVXRegBytes;
        }

        if (source + (1 * kAVXRegBytes) <= limit) {
            __m256i ymm0 = _mm256_loadu_si256((const __m256i *)(source + 32 * 0));

            _mm256_storeu_si256((__m256i *)(target + 32 * 0), ymm0);

            source += 1 * kAVXRegBytes;
            target += 1 * kAVXRegBytes;
        }

        while (lastUnalignedBytes != 0) {
            *target = *source;
            source += kValueSize;
            target += kValueSize;
            lastUnalignedBytes -= kValueSize;
        }
    }
}

template <typename T>
void left_rotate_sse_1_regs(T * first, T * mid, T * last,
                            std::size_t left_len, std::size_t right_len)
{
    __m128i stash0 = _mm_loadu_si128((const __m128i *)first);

    avx_forward_move_8(first, mid, last);

    _mm_storeu_si128((__m128i *)(last - left_len), stash0);
}

template <typename T>
void left_rotate_avx_1_regs(T * first, T * mid, T * last,
                            std::size_t left_len, std::size_t right_len)
{
    typedef T           value_type;
    typedef T *         pointer;
    typedef const T *   const_pointer;

    __m256i stash0 = _mm256_loadu_si256((const __m256i *)first);

    avx_forward_move_8(first, mid, last);

    _mm256_storeu_si256((__m256i *)(last - left_len), stash0);
}

template <typename T>
void left_rotate_avx_2_regs(T * first, T * mid, T * last,
                            std::size_t left_len, std::size_t right_len)
{
    const __m256i * stash_start = (const __m256i *)first;
    __m256i stash0 = _mm256_loadu_si256(stash_start + 0);
    __m256i stash1 = _mm256_loadu_si256(stash_start + 1);

    avx_forward_move_8(first, mid, last);

    __m256i * store_start = (__m256i *)(last - left_len);
    _mm256_storeu_si256(store_start + 0, stash0);
    _mm256_storeu_si256(store_start + 1, stash1);
}

template <typename T>
void left_rotate_avx_3_regs(T * first, T * mid, T * last,
                            std::size_t left_len, std::size_t right_len)
{
    const __m256i * stash_start = (const __m256i *)first;
    __m256i stash0 = _mm256_loadu_si256(stash_start + 0);
    __m256i stash1 = _mm256_loadu_si256(stash_start + 1);
    __m256i stash2 = _mm256_loadu_si256(stash_start + 2);

    avx_forward_move_8(first, mid, last);

    __m256i * store_start = (__m256i *)(last - left_len);
    _mm256_storeu_si256(store_start + 0, stash0);
    _mm256_storeu_si256(store_start + 1, stash1);
    _mm256_storeu_si256(store_start + 2, stash2);
}

template <typename T>
void left_rotate_avx_4_regs(T * first, T * mid, T * last,
                            std::size_t left_len, std::size_t right_len)
{
    const __m256i * stash_start = (const __m256i *)first;
    __m256i stash0 = _mm256_loadu_si256(stash_start + 0);
    __m256i stash1 = _mm256_loadu_si256(stash_start + 1);
    __m256i stash2 = _mm256_loadu_si256(stash_start + 2);
    __m256i stash3 = _mm256_loadu_si256(stash_start + 3);

    avx_forward_move_8(first, mid, last);

    __m256i * store_start = (__m256i *)(last - left_len);
    _mm256_storeu_si256(store_start + 0, stash0);
    _mm256_storeu_si256(store_start + 1, stash1);
    _mm256_storeu_si256(store_start + 2, stash2);
    _mm256_storeu_si256(store_start + 3, stash3);
}

template <typename T>
void _mm256_storeu_last(__m256i * addr, __m256i src, std::size_t left_bytes)
{
    //
}

template <typename T>
void left_rotate_avx_5_regs(T * first, T * mid, T * last,
                            std::size_t left_len, std::size_t right_len)
{
    const __m256i * stash_start = (const __m256i *)first;
    __m256i stash0 = _mm256_loadu_si256(stash_start + 0);
    __m256i stash1 = _mm256_loadu_si256(stash_start + 1);
    __m256i stash2 = _mm256_loadu_si256(stash_start + 2);
    __m256i stash3 = _mm256_loadu_si256(stash_start + 3);
    __m256i stash4 = _mm256_loadu_si256(stash_start + 4);

    avx_forward_move_6(first, mid, last);

    __m256i * store_start = (__m256i *)(last - left_len);
    _mm256_storeu_si256(store_start + 0, stash0);
    _mm256_storeu_si256(store_start + 1, stash1);
    _mm256_storeu_si256(store_start + 2, stash2);
    _mm256_storeu_si256(store_start + 3, stash3);
    _mm256_storeu_si256(store_start + 4, stash4);
    std::size_t leftBytes = left_len * sizeof(T) - 4 * kAVXRegBytes;
    _mm256_storeu_last<T>(store_start + 4, stash4, leftBytes);
}

template <typename T>
T * left_rotate_avx(T * data, std::size_t length, std::size_t offset)
{
    typedef T           value_type;
    typedef T *         pointer;
    typedef const T *   const_pointer;

    pointer first = data;
    pointer mid   = data + offset;
    pointer last  = data + length;

    std::size_t left_len = offset;
    if (left_len == 0) return first;

    std::size_t right_len = (offset <= length) ? (length - offset) : 0;
    if (right_len == 0) return last;

    if (length <= kRotateThresholdLength) {
        return left_rotate_simple_impl(first, mid, last, left_len, right_len);
    }

    pointer result = first + right_len;

    do {
        if (left_len <= right_len) {
            std::size_t left_bytes = left_len * sizeof(T);
            if (left_bytes <= kMaxAVXStashBytes) {
                std::size_t avx_needs = (left_bytes - 1) / kAVXRegBytes;
                switch (avx_needs) {
                    case 0:
                        if (left_bytes <= kSSERegBytes)
                            left_rotate_sse_1_regs(first, mid, last, left_len, right_len);
                        else
                            left_rotate_avx_1_regs(first, mid, last, left_len, right_len);
                        break;
                    case 1:
                        left_rotate_avx_2_regs(first, mid, last, left_len, right_len);
                        break;
                    case 2:
                        left_rotate_avx_3_regs(first, mid, last, left_len, right_len);
                        break;
                    case 3:
                        left_rotate_avx_4_regs(first, mid, last, left_len, right_len);
                        break;
                    case 4:
                        left_rotate_avx_5_regs(first, mid, last, left_len, right_len);
                        break;
                    case 5:
                        break;
                    case 6:
                        break;
                    case 7:
                        break;
                    case 8:
                        break;
                    case 9:
                        break;
                    case 10:
                        break;
                    default:
                        break;
                }
            }
            else {
                //
            }
        } else {
            std::size_t right_bytes = right_len * sizeof(T);
            if (right_bytes <= kMaxAVXStashBytes) {
                //
            }
            else {
                //
            }
        }
        break;
    } while (1);

    return result;
}

template <typename T>
inline
T * rotate(T * data, std::size_t length, std::size_t offset)
{
    JSTD_ASSERT((offset <= length), "simd::rotate(): (offset > length)");
    return left_rotate_avx(data, length, offset);
}

template <typename T>
inline
T * rotate(T * first, T * mid, T * last, void * void_ptr)
{
    JSTD_ASSERT((last >= mid), "simd::rotate(): (last < mid)");
    JSTD_ASSERT((mid >= first), "simd::rotate(): (mid < first)");
    return left_rotate_avx((T *)first, std::size_t(last - first), std::size_t(mid - first));
}

} // namespace simd
} // namespace jstd

#endif // JSTD_ARRAY_ROTATE_SIMD_H
