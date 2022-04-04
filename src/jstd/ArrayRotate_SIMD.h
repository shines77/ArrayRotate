
#ifndef JSTD_ARRAY_ROTATE_SIMD_H
#define JSTD_ARRAY_ROTATE_SIMD_H

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

#include <assert.h>
#include <cstdint>
#include <cstddef>
#include <cstdbool>
#include <algorithm>
#include <type_traits>

#include "jstd/stddef.h"
#include "jstd/BitVec.h"

//
// _mm_prefetch()
//
// See: https://stackoverflow.com/questions/46521694/what-are-mm-prefetch-locality-hints
// See: https://gist.github.com/simonhf/caaa33ccb87c0bf0775a863c0d6843c2
//
// See: https://gcc.gnu.org/onlinedocs/gcc/Other-Builtins.html
//
//   void __builtin_prefetch(const void * addr, int rw, int locality);
//
//   rw: (1) is preparing for a write, or (0) is preparing for a read;
//   locality: prefetch hint level, default value is 3 (_MM_HINT_T2).
//

#ifndef PREFETCH_HINT_LEVEL
#define PREFETCH_HINT_LEVEL     _MM_HINT_T0
#endif

namespace jstd {
namespace simd {

static const bool kUsePrefetchHint = true;

#if defined(_MSC_VER)
static const int  kPrefetchHintLevel = PREFETCH_HINT_LEVEL;
#else
static const enum _mm_hint kPrefetchHintLevel = PREFETCH_HINT_LEVEL;
#endif

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

static const bool kLoadIsAligned = true;
static const bool kStoreIsAligned = true;

static const bool kLoadIsNotAligned = false;
static const bool kStoreIsNotAligned = false;

template <typename T>
static inline
T * left_rotate_simple_impl(T * first, T * mid, T * last,
                            std::size_t left_len, std::size_t right_len)
{
    typedef T   value_type;
    typedef T * pointer;

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
    std::size_t left_len = std::size_t(mid - first);
    if (left_len == 0) return first;

    std::size_t right_len = std::size_t(last - mid);
    if (right_len == 0) return last;

    return left_rotate_simple_impl(first, mid, last, left_len, right_len);
}

template <typename T>
T * left_rotate_simple(T * data, std::size_t length, std::size_t offset)
{
    typedef T * pointer;

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

template <typename T, bool loadIsAligned, bool storeIsAligned>
static
JSTD_FORCE_INLINE
void avx_forward_move_8_tailing(char * target, char * source, char * end)
{
    static const int LeftLen = 4;
    static const std::size_t kValueSize = sizeof(T);
    std::size_t lastUnalignedBytes = (std::size_t)end & kAVXAlignMask;
    const char * limit = end - lastUnalignedBytes;

    if (loadIsAligned && storeIsAligned) {
        if (((source + (4 * kAVXRegBytes)) <= limit) && (LeftLen >= 4)) {
            __m256i ymm0 = _mm256_load_si256((const __m256i *)(source + 32 * 0));
            __m256i ymm1 = _mm256_load_si256((const __m256i *)(source + 32 * 1));
            __m256i ymm2 = _mm256_load_si256((const __m256i *)(source + 32 * 2));
            __m256i ymm3 = _mm256_load_si256((const __m256i *)(source + 32 * 3));

            _mm256_store_si256((__m256i *)(target + 32 * 0), ymm0);
            _mm256_store_si256((__m256i *)(target + 32 * 1), ymm1);
            _mm256_store_si256((__m256i *)(target + 32 * 2), ymm2);
            _mm256_store_si256((__m256i *)(target + 32 * 3), ymm3);

            source += 4 * kAVXRegBytes;
            target += 4 * kAVXRegBytes;
        }

        if (((source + (2 * kAVXRegBytes)) <= limit) && (LeftLen >= 2)) {
            __m256i ymm0 = _mm256_load_si256((const __m256i *)(source + 32 * 0));
            __m256i ymm1 = _mm256_load_si256((const __m256i *)(source + 32 * 1));

            _mm256_store_si256((__m256i *)(target + 32 * 0), ymm0);
            _mm256_store_si256((__m256i *)(target + 32 * 1), ymm1);

            source += 2 * kAVXRegBytes;
            target += 2 * kAVXRegBytes;
        }

        if (((source + (1 * kAVXRegBytes)) <= limit) && (LeftLen >= 1)) {
            __m256i ymm0 = _mm256_load_si256((const __m256i *)(source + 32 * 0));

            _mm256_store_si256((__m256i *)(target + 32 * 0), ymm0);

            source += 1 * kAVXRegBytes;
            target += 1 * kAVXRegBytes;
        }
    }
    else if (loadIsAligned && !storeIsAligned) {
        if (((source + (4 * kAVXRegBytes)) <= limit) && (LeftLen >= 4)) {
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

        if (((source + (2 * kAVXRegBytes)) <= limit) && (LeftLen >= 2)) {
            __m256i ymm0 = _mm256_load_si256((const __m256i *)(source + 32 * 0));
            __m256i ymm1 = _mm256_load_si256((const __m256i *)(source + 32 * 1));

            _mm256_storeu_si256((__m256i *)(target + 32 * 0), ymm0);
            _mm256_storeu_si256((__m256i *)(target + 32 * 1), ymm1);

            source += 2 * kAVXRegBytes;
            target += 2 * kAVXRegBytes;
        }

        if (((source + (1 * kAVXRegBytes)) <= limit) && (LeftLen >= 1)) {
            __m256i ymm0 = _mm256_load_si256((const __m256i *)(source + 32 * 0));

            _mm256_storeu_si256((__m256i *)(target + 32 * 0), ymm0);

            source += 1 * kAVXRegBytes;
            target += 1 * kAVXRegBytes;
        }
    }
    else if (!loadIsAligned && storeIsAligned) {
        if (((source + (4 * kAVXRegBytes)) <= limit) && (LeftLen >= 4)) {
            __m256i ymm0 = _mm256_loadu_si256((const __m256i *)(source + 32 * 0));
            __m256i ymm1 = _mm256_loadu_si256((const __m256i *)(source + 32 * 1));
            __m256i ymm2 = _mm256_loadu_si256((const __m256i *)(source + 32 * 2));
            __m256i ymm3 = _mm256_loadu_si256((const __m256i *)(source + 32 * 3));

            _mm256_store_si256((__m256i *)(target + 32 * 0), ymm0);
            _mm256_store_si256((__m256i *)(target + 32 * 1), ymm1);
            _mm256_store_si256((__m256i *)(target + 32 * 2), ymm2);
            _mm256_store_si256((__m256i *)(target + 32 * 3), ymm3);

            source += 4 * kAVXRegBytes;
            target += 4 * kAVXRegBytes;
        }

        if (((source + (2 * kAVXRegBytes)) <= limit) && (LeftLen >= 2)) {
            __m256i ymm0 = _mm256_loadu_si256((const __m256i *)(source + 32 * 0));
            __m256i ymm1 = _mm256_loadu_si256((const __m256i *)(source + 32 * 1));

            _mm256_store_si256((__m256i *)(target + 32 * 0), ymm0);
            _mm256_store_si256((__m256i *)(target + 32 * 1), ymm1);

            source += 2 * kAVXRegBytes;
            target += 2 * kAVXRegBytes;
        }

        if (((source + (1 * kAVXRegBytes)) <= limit) && (LeftLen >= 1)) {
            __m256i ymm0 = _mm256_loadu_si256((const __m256i *)(source + 32 * 0));

            _mm256_store_si256((__m256i *)(target + 32 * 0), ymm0);

            source += 1 * kAVXRegBytes;
            target += 1 * kAVXRegBytes;
        }
    }
    else {
        if (((source + (4 * kAVXRegBytes)) <= limit) && (LeftLen >= 4)) {
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

        if (((source + (2 * kAVXRegBytes)) <= limit) && (LeftLen >= 2)) {
            __m256i ymm0 = _mm256_loadu_si256((const __m256i *)(source + 32 * 0));
            __m256i ymm1 = _mm256_loadu_si256((const __m256i *)(source + 32 * 1));

            _mm256_storeu_si256((__m256i *)(target + 32 * 0), ymm0);
            _mm256_storeu_si256((__m256i *)(target + 32 * 1), ymm1);

            source += 2 * kAVXRegBytes;
            target += 2 * kAVXRegBytes;
        }

        if (((source + (1 * kAVXRegBytes)) <= limit) && (LeftLen >= 1)) {
            __m256i ymm0 = _mm256_loadu_si256((const __m256i *)(source + 32 * 0));

            _mm256_storeu_si256((__m256i *)(target + 32 * 0), ymm0);

            source += 1 * kAVXRegBytes;
            target += 1 * kAVXRegBytes;
        }
    }

    while (source < end) {
        *(T *)target = *(T *)source;
        source += kValueSize;
        target += kValueSize;
    }
}

template <typename T>
static
void avx_forward_move_4(T * first, T * mid, T * last)
{
    typedef T value_type;
    static const std::size_t kValueSize = sizeof(value_type);
    static const bool kValueSizeIsPower2 = ((kValueSize & (kValueSize - 1)) == 0);

    static const std::size_t kPerStepBytes = 4 * kAVXRegBytes;

    std::size_t unAlignedBytes = (std::size_t)mid & kAVXAlignMask;
    bool sourceAddrCanAlign = (((unAlignedBytes / kValueSize) * kValueSize) == unAlignedBytes);
    if (kValueSizeIsPower2 && sourceAddrCanAlign) {
        unAlignedBytes = (unAlignedBytes != 0) ? (kAVXRegBytes - unAlignedBytes) : 0;
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
            if (kUsePrefetchHint) {
                _mm_prefetch((const char *)(source + kPerStepBytes * 4), kPrefetchHintLevel);
            }

            __m256i ymm0 = _mm256_load_si256((const __m256i *)(source + 32 * 0));
            __m256i ymm1 = _mm256_load_si256((const __m256i *)(source + 32 * 1));
            __m256i ymm2 = _mm256_load_si256((const __m256i *)(source + 32 * 2));
            __m256i ymm3 = _mm256_load_si256((const __m256i *)(source + 32 * 3));

            _mm256_storeu_si256((__m256i *)(target + 32 * 0), ymm0);
            _mm256_storeu_si256((__m256i *)(target + 32 * 1), ymm1);
            _mm256_storeu_si256((__m256i *)(target + 32 * 2), ymm2);
            _mm256_storeu_si256((__m256i *)(target + 32 * 3), ymm3);

            source += kPerStepBytes;
            target += kPerStepBytes;
        }

        lastUnalignedBytes = (std::size_t)end & kAVXAlignMask;
        limit = end - lastUnalignedBytes;

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
            if (kUsePrefetchHint) {
                _mm_prefetch((const char *)(source + kPerStepBytes * 4), kPrefetchHintLevel);
            }

            __m256i ymm0 = _mm256_loadu_si256((const __m256i *)(source + 32 * 0));
            __m256i ymm1 = _mm256_loadu_si256((const __m256i *)(source + 32 * 1));
            __m256i ymm2 = _mm256_loadu_si256((const __m256i *)(source + 32 * 2));
            __m256i ymm3 = _mm256_loadu_si256((const __m256i *)(source + 32 * 3));

            _mm256_storeu_si256((__m256i *)(target + 32 * 0), ymm0);
            _mm256_storeu_si256((__m256i *)(target + 32 * 1), ymm1);
            _mm256_storeu_si256((__m256i *)(target + 32 * 2), ymm2);
            _mm256_storeu_si256((__m256i *)(target + 32 * 3), ymm3);

            source += kPerStepBytes;
            target += kPerStepBytes;
        }

        lastUnalignedBytes = (std::size_t)end & kAVXAlignMask;
        limit = end - lastUnalignedBytes;

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
static
void avx_forward_move_6(T * first, T * mid, T * last)
{
    typedef T value_type;
    static const std::size_t kValueSize = sizeof(value_type);
    static const bool kValueSizeIsPower2 = ((kValueSize & (kValueSize - 1)) == 0);

    static const std::size_t kPerStepBytes = 6 * kAVXRegBytes;

    std::size_t unAlignedBytes = (std::size_t)mid & kAVXAlignMask;
    bool sourceAddrCanAlign = (((unAlignedBytes / kValueSize) * kValueSize) == unAlignedBytes);
    if (kValueSizeIsPower2 && sourceAddrCanAlign) {
        unAlignedBytes = (unAlignedBytes != 0) ? (kAVXRegBytes - unAlignedBytes) : 0;
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
            if (kUsePrefetchHint) {
                _mm_prefetch((const char *)(source + kPerStepBytes * 3), kPrefetchHintLevel);
            }

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
            if (kUsePrefetchHint) {
                _mm_prefetch((const char *)(source + kPerStepBytes * 3), kPrefetchHintLevel);
            }

            __m256i ymm0 = _mm256_loadu_si256((const __m256i *)(source + 32 * 0));
            __m256i ymm1 = _mm256_loadu_si256((const __m256i *)(source + 32 * 1));
            __m256i ymm2 = _mm256_loadu_si256((const __m256i *)(source + 32 * 2));
            __m256i ymm3 = _mm256_loadu_si256((const __m256i *)(source + 32 * 3));
            __m256i ymm4 = _mm256_loadu_si256((const __m256i *)(source + 32 * 4));
            __m256i ymm5 = _mm256_loadu_si256((const __m256i *)(source + 32 * 5));

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
static
void avx_forward_move_8_load_aligned(T * first, T * mid, T * last)
{
    static const std::size_t kValueSize = sizeof(T);
    static const bool kValueSizeIsPower2 = ((kValueSize & (kValueSize - 1)) == 0);
    static const bool kValueSizeIsDivisible =  (kValueSize < kAVXRegBytes) ?
                                              ((kAVXRegBytes % kValueSize) == 0) :
                                              ((kValueSize % kAVXRegBytes) == 0);

    static const std::size_t kPerStepBytes = 8 * kAVXRegBytes;

    std::size_t unAlignedBytes = (std::size_t)mid & kAVXAlignMask;
    bool loadAddrCanAlign;
    if (kValueSize < kAVXRegBytes)
        loadAddrCanAlign = (kValueSizeIsDivisible && ((unAlignedBytes % kValueSize) == 0));
    else
        loadAddrCanAlign = (kValueSizeIsDivisible && (unAlignedBytes == 0));

    if (kValueSizeIsDivisible && loadAddrCanAlign) {
        //unAlignedBytes = (unAlignedBytes != 0) ? (kAVXRegBytes - unAlignedBytes) : 0;
        unAlignedBytes = (kAVXRegBytes - unAlignedBytes) & kAVXAlignMask;
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

        bool storeAddrIsAligned = (((std::size_t)target & kAVXAlignMask) == 0);
        if (!storeAddrIsAligned) {
            while (source < limit) {
                //
                // See: https://blog.csdn.net/qq_43401808/article/details/87360789
                //
                if (kUsePrefetchHint) {
                    _mm_prefetch((const char *)(source + kPerStepBytes * 2), kPrefetchHintLevel);
                }

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

            avx_forward_move_8_tailing<T, kLoadIsAligned, kStoreIsNotAligned>(target, source, end);
        } else {
            while (source < limit) {
                //
                // See: https://blog.csdn.net/qq_43401808/article/details/87360789
                //
                if (kUsePrefetchHint) {
                    _mm_prefetch((const char *)(source + kPerStepBytes * 2), kPrefetchHintLevel);
                }

                __m256i ymm0 = _mm256_load_si256((const __m256i *)(source + 32 * 0));
                __m256i ymm1 = _mm256_load_si256((const __m256i *)(source + 32 * 1));
                __m256i ymm2 = _mm256_load_si256((const __m256i *)(source + 32 * 2));
                __m256i ymm3 = _mm256_load_si256((const __m256i *)(source + 32 * 3));
                __m256i ymm4 = _mm256_load_si256((const __m256i *)(source + 32 * 4));
                __m256i ymm5 = _mm256_load_si256((const __m256i *)(source + 32 * 5));
                __m256i ymm6 = _mm256_load_si256((const __m256i *)(source + 32 * 6));
                __m256i ymm7 = _mm256_load_si256((const __m256i *)(source + 32 * 7));

                _mm256_store_si256((__m256i *)(target + 32 * 0), ymm0);
                _mm256_store_si256((__m256i *)(target + 32 * 1), ymm1);
                _mm256_store_si256((__m256i *)(target + 32 * 2), ymm2);
                _mm256_store_si256((__m256i *)(target + 32 * 3), ymm3);
                _mm256_store_si256((__m256i *)(target + 32 * 4), ymm4);
                _mm256_store_si256((__m256i *)(target + 32 * 5), ymm5);
                _mm256_store_si256((__m256i *)(target + 32 * 6), ymm6);
                _mm256_store_si256((__m256i *)(target + 32 * 7), ymm7);

                source += kPerStepBytes;
                target += kPerStepBytes;
            }

            avx_forward_move_8_tailing<T, kLoadIsAligned, kStoreIsAligned>(target, source, end);
        }
    }
    else {
        bool storeAddrCanAlign = false;
        if (kValueSizeIsDivisible) {
            unAlignedBytes = (std::size_t)first & kAVXAlignMask;
            if (kValueSize < kAVXRegBytes)
                storeAddrCanAlign = ((unAlignedBytes % kValueSize) == 0);
            else 
                storeAddrCanAlign = (unAlignedBytes == 0);

            if (storeAddrCanAlign) {
                unAlignedBytes = (kAVXRegBytes - unAlignedBytes) & kAVXAlignMask;
                while (unAlignedBytes != 0) {
                    *first++ = *mid++;
                    unAlignedBytes -= kValueSize;
                }
            }
        }

        char * target = (char *)first;
        char * source = (char *)mid;
        char * end = (char *)last;

        std::size_t lastUnalignedBytes = (std::size_t)last % kPerStepBytes;
        std::size_t totalBytes = (last - first) * kValueSize;
        const char * limit = (totalBytes >= kPerStepBytes) ? (end - lastUnalignedBytes) : source;

        if (storeAddrCanAlign) {
            while (source < limit) {
                if (kUsePrefetchHint) {
                    _mm_prefetch((const char *)(source + kPerStepBytes * 2), kPrefetchHintLevel);
                }

                __m256i ymm0 = _mm256_loadu_si256((const __m256i *)(source + 32 * 0));
                __m256i ymm1 = _mm256_loadu_si256((const __m256i *)(source + 32 * 1));
                __m256i ymm2 = _mm256_loadu_si256((const __m256i *)(source + 32 * 2));
                __m256i ymm3 = _mm256_loadu_si256((const __m256i *)(source + 32 * 3));
                __m256i ymm4 = _mm256_loadu_si256((const __m256i *)(source + 32 * 4));
                __m256i ymm5 = _mm256_loadu_si256((const __m256i *)(source + 32 * 5));
                __m256i ymm6 = _mm256_loadu_si256((const __m256i *)(source + 32 * 6));
                __m256i ymm7 = _mm256_loadu_si256((const __m256i *)(source + 32 * 7));

                _mm256_store_si256((__m256i *)(target + 32 * 0), ymm0);
                _mm256_store_si256((__m256i *)(target + 32 * 1), ymm1);
                _mm256_store_si256((__m256i *)(target + 32 * 2), ymm2);
                _mm256_store_si256((__m256i *)(target + 32 * 3), ymm3);
                _mm256_store_si256((__m256i *)(target + 32 * 4), ymm4);
                _mm256_store_si256((__m256i *)(target + 32 * 5), ymm5);
                _mm256_store_si256((__m256i *)(target + 32 * 6), ymm6);
                _mm256_store_si256((__m256i *)(target + 32 * 7), ymm7);

                source += kPerStepBytes;
                target += kPerStepBytes;
            }

            avx_forward_move_8_tailing<T, kLoadIsNotAligned, kStoreIsAligned>(target, source, end);
        } else {
            while (source < limit) {
                if (kUsePrefetchHint) {
                    _mm_prefetch((const char *)(source + kPerStepBytes * 2), kPrefetchHintLevel);
                }

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

            avx_forward_move_8_tailing<T, kLoadIsNotAligned, kStoreIsNotAligned>(target, source, end);
        }
    }
}

template <typename T>
static
void avx_forward_move_8_store_aligned(T * first, T * mid, T * last)
{
    static const std::size_t kValueSize = sizeof(T);
    static const bool kValueSizeIsPower2 = ((kValueSize & (kValueSize - 1)) == 0);
    static const bool kValueSizeIsDivisible =  (kValueSize < kAVXRegBytes) ?
                                              ((kAVXRegBytes % kValueSize) == 0) :
                                              ((kValueSize % kAVXRegBytes) == 0);

    static const std::size_t kPerStepBytes = 8 * kAVXRegBytes;

    std::size_t unAlignedBytes = (std::size_t)first & kAVXAlignMask;
    bool storeAddrCanAlign;
    if (kValueSize < kAVXRegBytes)
        storeAddrCanAlign = (kValueSizeIsDivisible && ((unAlignedBytes % kValueSize) == 0));
    else
        storeAddrCanAlign = (kValueSizeIsDivisible && (unAlignedBytes == 0));

    if (kValueSizeIsDivisible && storeAddrCanAlign) {
        //unAlignedBytes = (unAlignedBytes != 0) ? (kAVXRegBytes - unAlignedBytes) : 0;
        unAlignedBytes = (kAVXRegBytes - unAlignedBytes) & kAVXAlignMask;
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

        bool loadAddrIsAligned = (((std::size_t)source & kAVXAlignMask) == 0);
        if (!loadAddrIsAligned) {
            while (source < limit) {
                //
                // See: https://blog.csdn.net/qq_43401808/article/details/87360789
                //
                if (kUsePrefetchHint) {
                    _mm_prefetch((const char *)(source + kPerStepBytes * 2), kPrefetchHintLevel);
                }

                __m256i ymm0 = _mm256_loadu_si256((const __m256i *)(source + 32 * 0));
                __m256i ymm1 = _mm256_loadu_si256((const __m256i *)(source + 32 * 1));
                __m256i ymm2 = _mm256_loadu_si256((const __m256i *)(source + 32 * 2));
                __m256i ymm3 = _mm256_loadu_si256((const __m256i *)(source + 32 * 3));
                __m256i ymm4 = _mm256_loadu_si256((const __m256i *)(source + 32 * 4));
                __m256i ymm5 = _mm256_loadu_si256((const __m256i *)(source + 32 * 5));
                __m256i ymm6 = _mm256_loadu_si256((const __m256i *)(source + 32 * 6));
                __m256i ymm7 = _mm256_loadu_si256((const __m256i *)(source + 32 * 7));

                _mm256_store_si256((__m256i *)(target + 32 * 0), ymm0);
                _mm256_store_si256((__m256i *)(target + 32 * 1), ymm1);
                _mm256_store_si256((__m256i *)(target + 32 * 2), ymm2);
                _mm256_store_si256((__m256i *)(target + 32 * 3), ymm3);
                _mm256_store_si256((__m256i *)(target + 32 * 4), ymm4);
                _mm256_store_si256((__m256i *)(target + 32 * 5), ymm5);
                _mm256_store_si256((__m256i *)(target + 32 * 6), ymm6);
                _mm256_store_si256((__m256i *)(target + 32 * 7), ymm7);

                source += kPerStepBytes;
                target += kPerStepBytes;
            }

            avx_forward_move_8_tailing<T, kLoadIsNotAligned, kStoreIsAligned>(target, source, end);
        } else {
            while (source < limit) {
                //
                // See: https://blog.csdn.net/qq_43401808/article/details/87360789
                //
                if (kUsePrefetchHint) {
                    _mm_prefetch((const char *)(source + kPerStepBytes * 2), kPrefetchHintLevel);
                }

                __m256i ymm0 = _mm256_load_si256((const __m256i *)(source + 32 * 0));
                __m256i ymm1 = _mm256_load_si256((const __m256i *)(source + 32 * 1));
                __m256i ymm2 = _mm256_load_si256((const __m256i *)(source + 32 * 2));
                __m256i ymm3 = _mm256_load_si256((const __m256i *)(source + 32 * 3));
                __m256i ymm4 = _mm256_load_si256((const __m256i *)(source + 32 * 4));
                __m256i ymm5 = _mm256_load_si256((const __m256i *)(source + 32 * 5));
                __m256i ymm6 = _mm256_load_si256((const __m256i *)(source + 32 * 6));
                __m256i ymm7 = _mm256_load_si256((const __m256i *)(source + 32 * 7));

                _mm256_store_si256((__m256i *)(target + 32 * 0), ymm0);
                _mm256_store_si256((__m256i *)(target + 32 * 1), ymm1);
                _mm256_store_si256((__m256i *)(target + 32 * 2), ymm2);
                _mm256_store_si256((__m256i *)(target + 32 * 3), ymm3);
                _mm256_store_si256((__m256i *)(target + 32 * 4), ymm4);
                _mm256_store_si256((__m256i *)(target + 32 * 5), ymm5);
                _mm256_store_si256((__m256i *)(target + 32 * 6), ymm6);
                _mm256_store_si256((__m256i *)(target + 32 * 7), ymm7);

                source += kPerStepBytes;
                target += kPerStepBytes;
            }

            avx_forward_move_8_tailing<T, kLoadIsAligned, kStoreIsAligned>(target, source, end);
        }
    }
    else {
        bool loadAddrCanAlign = false;
        if (kValueSizeIsDivisible) {
            unAlignedBytes = (std::size_t)mid & kAVXAlignMask;
            if (kValueSize < kAVXRegBytes)
                loadAddrCanAlign = ((unAlignedBytes % kValueSize) == 0);
            else 
                loadAddrCanAlign = (unAlignedBytes == 0);

            if (loadAddrCanAlign) {
                unAlignedBytes = (kAVXRegBytes - unAlignedBytes) & kAVXAlignMask;
                while (unAlignedBytes != 0) {
                    *first++ = *mid++;
                    unAlignedBytes -= kValueSize;
                }
            }
        }

        char * target = (char *)first;
        char * source = (char *)mid;
        char * end = (char *)last;

        std::size_t lastUnalignedBytes = (std::size_t)last % kPerStepBytes;
        std::size_t totalBytes = (last - first) * kValueSize;
        const char * limit = (totalBytes >= kPerStepBytes) ? (end - lastUnalignedBytes) : source;

        if (loadAddrCanAlign) {
            while (source < limit) {
                if (kUsePrefetchHint) {
                    _mm_prefetch((const char *)(source + kPerStepBytes * 2), kPrefetchHintLevel);
                }

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

            avx_forward_move_8_tailing<T, kLoadIsAligned, kStoreIsNotAligned>(target, source, end);
        } else {
            while (source < limit) {
                if (kUsePrefetchHint) {
                    _mm_prefetch((const char *)(source + kPerStepBytes * 2), kPrefetchHintLevel);
                }

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

            avx_forward_move_8_tailing<T, kLoadIsNotAligned, kStoreIsNotAligned>(target, source, end);
        }
    }
}

template <typename T>
static
void avx_forward_move_8x2(T * first, T * mid, T * last)
{
    typedef T value_type;
    static const std::size_t kValueSize = sizeof(value_type);
    static const bool kValueSizeIsPower2 = ((kValueSize & (kValueSize - 1)) == 0);

    static const std::size_t kPerStepBytes = 16 * kAVXRegBytes;

    std::size_t unAlignedBytes = (std::size_t)mid & kAVXAlignMask;
    bool sourceAddrCanAlign = (((unAlignedBytes / kValueSize) * kValueSize) == unAlignedBytes);
    if (kValueSizeIsPower2 && sourceAddrCanAlign) {
        unAlignedBytes = (unAlignedBytes != 0) ? (kAVXRegBytes - unAlignedBytes) : 0;
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
            //
            // See: https://blog.csdn.net/qq_43401808/article/details/87360789
            //
            if (kUsePrefetchHint) {
                _mm_prefetch((const char *)(source + kPerStepBytes * 2), kPrefetchHintLevel);
            }

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

            if (kUsePrefetchHint) {
                _mm_prefetch((const char *)(source + kPerStepBytes * 2), kPrefetchHintLevel);
            }

            ymm0 = _mm256_load_si256((const __m256i *)(source + 32 * 8));
            ymm1 = _mm256_load_si256((const __m256i *)(source + 32 * 9));
            ymm2 = _mm256_load_si256((const __m256i *)(source + 32 * 10));
            ymm3 = _mm256_load_si256((const __m256i *)(source + 32 * 11));
            ymm4 = _mm256_load_si256((const __m256i *)(source + 32 * 12));
            ymm5 = _mm256_load_si256((const __m256i *)(source + 32 * 13));
            ymm6 = _mm256_load_si256((const __m256i *)(source + 32 * 14));
            ymm7 = _mm256_load_si256((const __m256i *)(source + 32 * 15));

            _mm256_storeu_si256((__m256i *)(target + 32 * 8),  ymm0);
            _mm256_storeu_si256((__m256i *)(target + 32 * 9),  ymm1);
            _mm256_storeu_si256((__m256i *)(target + 32 * 10), ymm2);
            _mm256_storeu_si256((__m256i *)(target + 32 * 11), ymm3);
            _mm256_storeu_si256((__m256i *)(target + 32 * 12), ymm4);
            _mm256_storeu_si256((__m256i *)(target + 32 * 13), ymm5);
            _mm256_storeu_si256((__m256i *)(target + 32 * 14), ymm6);
            _mm256_storeu_si256((__m256i *)(target + 32 * 15), ymm7);

            source += kPerStepBytes;
            target += kPerStepBytes;
        }

        lastUnalignedBytes = (std::size_t)end & kAVXAlignMask;
        limit = end - lastUnalignedBytes;

        if (source + (8 * kAVXRegBytes) <= limit) {
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

            source += 8 * kAVXRegBytes;
            target += 8 * kAVXRegBytes;
        }

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
            if (kUsePrefetchHint) {
                _mm_prefetch((const char *)(source + kPerStepBytes * 2), kPrefetchHintLevel);
            }

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

            if (kUsePrefetchHint) {
                _mm_prefetch((const char *)(source + kPerStepBytes * 2), kPrefetchHintLevel);
            }

            ymm0 = _mm256_loadu_si256((const __m256i *)(source + 32 * 8));
            ymm1 = _mm256_loadu_si256((const __m256i *)(source + 32 * 9));
            ymm2 = _mm256_loadu_si256((const __m256i *)(source + 32 * 10));
            ymm3 = _mm256_loadu_si256((const __m256i *)(source + 32 * 11));
            ymm4 = _mm256_loadu_si256((const __m256i *)(source + 32 * 12));
            ymm5 = _mm256_loadu_si256((const __m256i *)(source + 32 * 13));
            ymm6 = _mm256_loadu_si256((const __m256i *)(source + 32 * 14));
            ymm7 = _mm256_loadu_si256((const __m256i *)(source + 32 * 15));

            _mm256_storeu_si256((__m256i *)(target + 32 * 8),  ymm0);
            _mm256_storeu_si256((__m256i *)(target + 32 * 9),  ymm1);
            _mm256_storeu_si256((__m256i *)(target + 32 * 10), ymm2);
            _mm256_storeu_si256((__m256i *)(target + 32 * 11), ymm3);
            _mm256_storeu_si256((__m256i *)(target + 32 * 12), ymm4);
            _mm256_storeu_si256((__m256i *)(target + 32 * 13), ymm5);
            _mm256_storeu_si256((__m256i *)(target + 32 * 14), ymm6);
            _mm256_storeu_si256((__m256i *)(target + 32 * 15), ymm7);

            source += kPerStepBytes;
            target += kPerStepBytes;
        }

        lastUnalignedBytes = (std::size_t)end & kAVXAlignMask;
        limit = end - lastUnalignedBytes;

        if (source + (8 * kAVXRegBytes) <= limit) {
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

            source += 8 * kAVXRegBytes;
            target += 8 * kAVXRegBytes;
        }

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

template <typename T, std::size_t index>
void _mm_storeu_last(__m128i * addr, __m128i src, std::size_t left_len)
{
    uint8_t * target = (uint8_t *)addr;
    std::intptr_t left_bytes = left_len * sizeof(T) - index * kSSERegBytes;
    assert(left_bytes >= 0 && left_bytes <= kSSERegBytes);
    uint32_t value32_0, value32_1;
    uint64_t value64_0, value64_1;
    switch (left_bytes) {
        case 0:
            assert(false);
            break;
        case 1:
            value32_0 = (uint32_t)_mm_extract_epi32(src, 0);
            *target = uint8_t(value32_0 & 0xFFu);
            break;
        case 2:
            value32_0 = (uint32_t)_mm_extract_epi16(src, 0);
            *(uint16_t *)(target + 0) = value32_0;
            break;
        case 3:
            value32_0 = (uint32_t)_mm_extract_epi32(src, 0);
            *(uint16_t *)(target + 0) = uint16_t(value32_0 & 0xFFFFu);
            *(uint8_t  *)(target + 2) = uint8_t(value32_0 >> 16u);
            break;
        case 4:
            value32_0 = (uint32_t)_mm_extract_epi32(src, 0);
            *(uint32_t *)(target + 0) = value32_0;
            break;
        case 5:
            value32_0 = (uint32_t)_mm_extract_epi32(src, 0);
            value32_1 = (uint32_t)_mm_extract_epi32(src, 1);
            *(uint32_t *)(target + 0) = value32_0;
            *(uint8_t  *)(target + 4) = uint8_t(value32_1 & 0xFFu);
            break;
        case 6:
            value32_0 = (uint32_t)_mm_extract_epi32(src, 0);
            value32_1 = (uint32_t)_mm_extract_epi16(src, 2);
            *(uint32_t *)(target + 0) = value32_0;
            *(uint16_t *)(target + 4) = value32_1;
            break;
        case 7:
            value32_0 = (uint32_t)_mm_extract_epi32(src, 0);
            value32_1 = (uint32_t)_mm_extract_epi32(src, 1);
            *(uint32_t *)(target + 0) = value32_0;
            *(uint16_t *)(target + 4) = uint16_t(value32_1 & 0xFFFFu);
            *(uint8_t  *)(target + 6) = uint8_t(value32_1 >> 16u);
            break;
        case 8:
            value64_0 = (uint64_t)_mm_extract_epi64(src, 0);
            *(uint64_t *)(target + 0) = value64_0;
            break;
        case 9:
            value64_0 = (uint64_t)_mm_extract_epi64(src, 0);
            value32_0 = (uint32_t)_mm_extract_epi32(src, 2);
            *(uint64_t *)(target + 0) = value64_0;
            *(uint8_t  *)(target + 8) = uint8_t(value32_0 & 0xFFu);
            break;
        case 10:
            value64_0 = (uint64_t)_mm_extract_epi64(src, 0);
            value32_0 = (uint32_t)_mm_extract_epi16(src, 2);
            *(uint64_t *)(target + 0) = value64_0;
            *(uint16_t *)(target + 8) = uint16_t(value32_0 & 0xFFFFu);
            break;
        case 11:
            value64_0 = (uint64_t)_mm_extract_epi64(src, 0);
            value32_0 = (uint32_t)_mm_extract_epi32(src, 2);
            *(uint64_t *)(target + 0) = value64_0;
            *(uint16_t *)(target + 8) = uint16_t(value32_0 & 0xFFFFu);
            *(uint8_t  *)(target + 10) = uint8_t(value32_0 >> 16u);
            break;
        case 12:
            value64_0 = (uint64_t)_mm_extract_epi64(src, 0);
            value32_0 = (uint32_t)_mm_extract_epi32(src, 2);
            *(uint64_t *)(target + 0) = value64_0;
            *(uint32_t *)(target + 8) = value32_0;
            break;
        case 13:
            value64_0 = (uint64_t)_mm_extract_epi64(src, 0);
            value64_1 = (uint64_t)_mm_extract_epi64(src, 1);
            *(uint64_t *)(target + 0) = value64_0;
            *(uint32_t *)(target + 8) = (uint32_t)(value64_1 & 0xFFFFFFFFu);
            *(uint8_t  *)(target + 12) = (uint8_t)((value64_1 >> 32u) & 0xFFu);
            break;
        case 14:
            value64_0 = (uint64_t)_mm_extract_epi64(src, 0);
            value64_1 = (uint64_t)_mm_extract_epi64(src, 1);
            *(uint64_t *)(target + 0) = value64_0;
            *(uint32_t *)(target + 8) = (uint32_t)(value64_1 & 0xFFFFFFFFu);
            *(uint16_t *)(target + 12) = (uint16_t)((value64_1 >> 32u) & 0xFFFFu);
            break;
        case 15:
            value64_0 = (uint64_t)_mm_extract_epi64(src, 0);
            value64_1 = (uint64_t)_mm_extract_epi64(src, 1);
            *(uint64_t *)(target + 0) = value64_0;
            *(uint32_t *)(target + 8) = (uint32_t)(value64_1 & 0xFFFFFFFFu);
            *(uint16_t *)(target + 12) = (uint16_t)((value64_1 >> 32u) & 0xFFFFu);
            *(uint8_t  *)(target + 14) = (uint8_t)((value64_1 >> 48u) & 0xFFu);
            break;
        case 16:
            _mm_storeu_si128(addr, src);
            break;
        default:
            break;
    }
}

template <typename T, std::size_t index>
void _mm256_storeu_last(__m256i * addr, __m256i src, std::size_t left_len)
{
    uint8_t * target = (uint8_t *)addr;
    std::intptr_t left_bytes = left_len * sizeof(T) - index * kAVXRegBytes;
    assert(left_bytes >= 0 && left_bytes <= kAVXRegBytes);
    uint32_t value32_0, value32_1;
    uint64_t value64_0, value64_1, value64_2;
    switch (left_bytes) {
        case 0:
            assert(false);
            break;
        case 1:
            value32_0 = (uint32_t)AVX::mm256_extract_epi32<0>(src);
            *target = uint8_t(value32_0 & 0xFFu);
            break;
        case 2:
            value32_0 = (uint32_t)AVX::mm256_extract_epi16<0>(src);
            *(uint16_t *)(target + 0) = value32_0;
            break;
        case 3:
            value32_0 = (uint32_t)AVX::mm256_extract_epi32<0>(src);
            *(uint16_t *)(target + 0) = uint16_t(value32_0 & 0xFFFFu);
            *(uint8_t  *)(target + 2) = uint8_t(value32_0 >> 16u);
            break;
        case 4:
            value32_0 = (uint32_t)AVX::mm256_extract_epi32<0>(src);
            *(uint32_t *)(target + 0) = value32_0;
            break;
        case 5:
            value32_0 = (uint32_t)AVX::mm256_extract_epi32<0>(src);
            value32_1 = (uint32_t)AVX::mm256_extract_epi32<1>(src);
            *(uint32_t *)(target + 0) = value32_0;
            *(uint8_t  *)(target + 4) = uint8_t(value32_1 & 0xFFu);
            break;
        case 6:
            value32_0 = (uint32_t)AVX::mm256_extract_epi32<0>(src);
            value32_1 = (uint32_t)AVX::mm256_extract_epi16<2>(src);
            *(uint32_t *)(target + 0) = value32_0;
            *(uint16_t *)(target + 4) = value32_1;
            break;
        case 7:
            value32_0 = (uint32_t)AVX::mm256_extract_epi32<0>(src);
            value32_1 = (uint32_t)AVX::mm256_extract_epi32<1>(src);
            *(uint32_t *)(target + 0) = value32_0;
            *(uint16_t *)(target + 4) = uint16_t(value32_1 & 0xFFFFu);
            *(uint8_t  *)(target + 6) = uint8_t(value32_1 >> 16u);
            break;
        case 8:
            value64_0 = (uint64_t)AVX::mm256_extract_epi64<0>(src);
            *(uint64_t *)(target + 0) = value64_0;
            break;
        case 9:
            value64_0 = (uint64_t)AVX::mm256_extract_epi64<0>(src);
            value32_0 = (uint32_t)AVX::mm256_extract_epi32<2>(src);
            *(uint64_t *)(target + 0) = value64_0;
            *(uint8_t  *)(target + 8) = uint8_t(value32_0 & 0xFFu);
            break;
        case 10:
            value64_0 = (uint64_t)AVX::mm256_extract_epi64<0>(src);
            value32_0 = (uint32_t)AVX::mm256_extract_epi16<4>(src);
            *(uint64_t *)(target + 0) = value64_0;
            *(uint16_t *)(target + 8) = uint16_t(value32_0 & 0xFFFFu);
            break;
        case 11:
            value64_0 = (uint64_t)AVX::mm256_extract_epi64<0>(src);
            value32_0 = (uint32_t)AVX::mm256_extract_epi32<2>(src);
            *(uint64_t *)(target + 0) = value64_0;
            *(uint16_t *)(target + 8) = uint16_t(value32_0 & 0xFFFFu);
            *(uint8_t  *)(target + 10) = uint8_t(value32_0 >> 16u);
            break;
        case 12:
            value64_0 = (uint64_t)AVX::mm256_extract_epi64<0>(src);
            value32_0 = (uint32_t)AVX::mm256_extract_epi32<2>(src);
            *(uint64_t *)(target + 0) = value64_0;
            *(uint32_t *)(target + 8) = value32_0;
            break;
        case 13:
            value64_0 = (uint64_t)AVX::mm256_extract_epi64<0>(src);
            value64_1 = (uint64_t)AVX::mm256_extract_epi64<1>(src);
            *(uint64_t *)(target + 0) = value64_0;
            *(uint32_t *)(target + 8) = (uint32_t)(value64_1 & 0xFFFFFFFFu);
            *(uint8_t  *)(target + 12) = (uint8_t)((value64_1 >> 32u) & 0xFFu);
            break;
        case 14:
            value64_0 = (uint64_t)AVX::mm256_extract_epi64<0>(src);
            value64_1 = (uint64_t)AVX::mm256_extract_epi64<1>(src);
            *(uint64_t *)(target + 0) = value64_0;
            *(uint32_t *)(target + 8) = (uint32_t)(value64_1 & 0xFFFFFFFFu);
            *(uint16_t *)(target + 12) = (uint16_t)((value64_1 >> 32u) & 0xFFFFu);
            break;
        case 15:
            value64_0 = (uint64_t)AVX::mm256_extract_epi64<0>(src);
            value64_1 = (uint64_t)AVX::mm256_extract_epi64<1>(src);
            *(uint64_t *)(target + 0) = value64_0;
            *(uint32_t *)(target + 8) = (uint32_t)(value64_1 & 0xFFFFFFFFu);
            *(uint16_t *)(target + 12) = (uint16_t)((value64_1 >> 32u) & 0xFFFFu);
            *(uint8_t  *)(target + 14) = (uint8_t)((value64_1 >> 48u) & 0xFFu);
            break;
        case 16:
            value64_0 = (uint64_t)AVX::mm256_extract_epi64<0>(src);
            value64_1 = (uint64_t)AVX::mm256_extract_epi64<1>(src);
            *(uint64_t *)(target + 0) = value64_0;
            *(uint64_t *)(target + 8) = value64_1;
            break;
        case 17:
            value64_0 = (uint64_t)AVX::mm256_extract_epi64<0>(src);
            value64_1 = (uint64_t)AVX::mm256_extract_epi64<1>(src);
            value32_0 = (uint32_t)AVX::mm256_extract_epi32<4>(src);
            
            *(uint64_t *)(target + 0) = value64_0;
            *(uint64_t *)(target + 8) = value64_1;
            *(uint8_t  *)(target + 16) = uint8_t(value32_0 & 0xFFu);
            break;
        case 18:
            value64_0 = (uint64_t)AVX::mm256_extract_epi64<0>(src);
            value64_1 = (uint64_t)AVX::mm256_extract_epi64<1>(src);
            value32_0 = (uint32_t)AVX::mm256_extract_epi16<8>(src);
            
            *(uint64_t *)(target + 0) = value64_0;
            *(uint64_t *)(target + 8) = value64_1;
            *(uint16_t *)(target + 16) = uint16_t(value32_0);
            break;
        case 19:
            value64_0 = (uint64_t)AVX::mm256_extract_epi64<0>(src);
            value64_1 = (uint64_t)AVX::mm256_extract_epi64<1>(src);
            value32_0 = (uint32_t)AVX::mm256_extract_epi32<4>(src);
            
            *(uint64_t *)(target + 0) = value64_0;
            *(uint64_t *)(target + 8) = value64_1;
            *(uint16_t *)(target + 16) = uint16_t(value32_0 & 0xFFFFu);
            *(uint8_t  *)(target + 18) = uint8_t((value32_0 >> 16u) & 0xFFu);
            break;
        case 20:
            value64_0 = (uint64_t)AVX::mm256_extract_epi64<0>(src);
            value64_1 = (uint64_t)AVX::mm256_extract_epi64<1>(src);
            value32_0 = (uint32_t)AVX::mm256_extract_epi32<4>(src);
            
            *(uint64_t *)(target + 0) = value64_0;
            *(uint64_t *)(target + 8) = value64_1;
            *(uint32_t *)(target + 16) = value32_0;
            break;
        case 21:
            value64_0 = (uint64_t)AVX::mm256_extract_epi64<0>(src);
            value64_1 = (uint64_t)AVX::mm256_extract_epi64<1>(src);
            value32_0 = (uint32_t)AVX::mm256_extract_epi32<4>(src);
            value32_1 = (uint32_t)AVX::mm256_extract_epi32<5>(src);
            
            *(uint64_t *)(target + 0) = value64_0;
            *(uint64_t *)(target + 8) = value64_1;
            *(uint32_t *)(target + 16) = value32_0;
            *(uint8_t  *)(target + 20) = uint8_t(value32_1 & 0xFFu);
            break;
        case 22:
            value64_0 = (uint64_t)AVX::mm256_extract_epi64<0>(src);
            value64_1 = (uint64_t)AVX::mm256_extract_epi64<1>(src);
            value32_0 = (uint32_t)AVX::mm256_extract_epi32<4>(src);
            value32_1 = (uint32_t)AVX::mm256_extract_epi16<10>(src);
            
            *(uint64_t *)(target + 0) = value64_0;
            *(uint64_t *)(target + 8) = value64_1;
            *(uint32_t *)(target + 16) = value32_0;
            *(uint16_t *)(target + 20) = uint16_t(value32_1);
            break;
        case 23:
            value64_0 = (uint64_t)AVX::mm256_extract_epi64<0>(src);
            value64_1 = (uint64_t)AVX::mm256_extract_epi64<1>(src);
            value32_0 = (uint32_t)AVX::mm256_extract_epi32<4>(src);
            value32_1 = (uint32_t)AVX::mm256_extract_epi32<5>(src);
            
            *(uint64_t *)(target + 0) = value64_0;
            *(uint64_t *)(target + 8) = value64_1;
            *(uint32_t *)(target + 16) = value32_0;
            *(uint16_t *)(target + 20) = uint16_t(value32_1 & 0xFFFFu);
            *(uint8_t  *)(target + 22) = uint8_t((value32_1 >> 16u) & 0xFFu);
            break;
        case 24:
            value64_0 = (uint64_t)AVX::mm256_extract_epi64<0>(src);
            value64_1 = (uint64_t)AVX::mm256_extract_epi64<1>(src);
            value64_2 = (uint64_t)AVX::mm256_extract_epi64<2>(src);
            
            *(uint64_t *)(target + 0)  = value64_0;
            *(uint64_t *)(target + 8)  = value64_1;
            *(uint64_t *)(target + 16) = value64_2;
            break;
        case 25:
            value64_0 = (uint64_t)AVX::mm256_extract_epi64<0>(src);
            value64_1 = (uint64_t)AVX::mm256_extract_epi64<1>(src);
            value64_2 = (uint64_t)AVX::mm256_extract_epi64<2>(src);
            value32_0 = (uint32_t)AVX::mm256_extract_epi32<6>(src);
            
            *(uint64_t *)(target + 0)  = value64_0;
            *(uint64_t *)(target + 8)  = value64_1;
            *(uint64_t *)(target + 16) = value64_2;
            *(uint8_t  *)(target + 24) = uint8_t(value32_0 & 0xFFu);
            break;
        case 26:
            value64_0 = (uint64_t)AVX::mm256_extract_epi64<0>(src);
            value64_1 = (uint64_t)AVX::mm256_extract_epi64<1>(src);
            value64_2 = (uint64_t)AVX::mm256_extract_epi64<2>(src);
            value32_0 = (uint32_t)AVX::mm256_extract_epi16<12>(src);
            
            *(uint64_t *)(target + 0)  = value64_0;
            *(uint64_t *)(target + 8)  = value64_1;
            *(uint64_t *)(target + 16) = value64_2;
            *(uint16_t *)(target + 24) = uint16_t(value32_0);
            break;
        case 27:
            value64_0 = (uint64_t)AVX::mm256_extract_epi64<0>(src);
            value64_1 = (uint64_t)AVX::mm256_extract_epi64<1>(src);
            value64_2 = (uint64_t)AVX::mm256_extract_epi64<2>(src);
            value32_0 = (uint32_t)AVX::mm256_extract_epi32<6>(src);
            
            *(uint64_t *)(target + 0)  = value64_0;
            *(uint64_t *)(target + 8)  = value64_1;
            *(uint64_t *)(target + 16) = value64_2;
            *(uint16_t *)(target + 24) = uint16_t(value32_0 & 0xFFFFu);
            *(uint8_t  *)(target + 26) = uint8_t((value32_0 >> 16u) & 0xFFu);
            break;
        case 28:
            value64_0 = (uint64_t)AVX::mm256_extract_epi64<0>(src);
            value64_1 = (uint64_t)AVX::mm256_extract_epi64<1>(src);
            value64_2 = (uint64_t)AVX::mm256_extract_epi64<2>(src);
            value32_0 = (uint32_t)AVX::mm256_extract_epi32<6>(src);
            
            *(uint64_t *)(target + 0)  = value64_0;
            *(uint64_t *)(target + 8)  = value64_1;
            *(uint64_t *)(target + 16) = value64_2;
            *(uint32_t *)(target + 24) = value32_0;
            break;
        case 29:
            value64_0 = (uint64_t)AVX::mm256_extract_epi64<0>(src);
            value64_1 = (uint64_t)AVX::mm256_extract_epi64<1>(src);
            value64_2 = (uint64_t)AVX::mm256_extract_epi64<2>(src);
            value32_0 = (uint32_t)AVX::mm256_extract_epi32<6>(src);
            value32_1 = (uint32_t)AVX::mm256_extract_epi32<7>(src);
            
            *(uint64_t *)(target + 0)  = value64_0;
            *(uint64_t *)(target + 8)  = value64_1;
            *(uint64_t *)(target + 16) = value64_2;
            *(uint32_t *)(target + 24) = value32_0;
            *(uint8_t  *)(target + 28) = uint8_t(value32_1 & 0xFFu);
            break;
        case 30:
            value64_0 = (uint64_t)AVX::mm256_extract_epi64<0>(src);
            value64_1 = (uint64_t)AVX::mm256_extract_epi64<1>(src);
            value64_2 = (uint64_t)AVX::mm256_extract_epi64<2>(src);
            value32_0 = (uint32_t)AVX::mm256_extract_epi32<6>(src);
            value32_1 = (uint32_t)AVX::mm256_extract_epi16<14>(src);
            
            *(uint64_t *)(target + 0)  = value64_0;
            *(uint64_t *)(target + 8)  = value64_1;
            *(uint64_t *)(target + 16) = value64_2;
            *(uint32_t *)(target + 24) = value32_0;
            *(uint16_t *)(target + 28) = uint16_t(value32_1);
            break;
        case 31:
            value64_0 = (uint64_t)AVX::mm256_extract_epi64<0>(src);
            value64_1 = (uint64_t)AVX::mm256_extract_epi64<1>(src);
            value64_2 = (uint64_t)AVX::mm256_extract_epi64<2>(src);
            value32_0 = (uint32_t)AVX::mm256_extract_epi32<6>(src);
            value32_1 = (uint32_t)AVX::mm256_extract_epi32<7>(src);
            
            *(uint64_t *)(target + 0)  = value64_0;
            *(uint64_t *)(target + 8)  = value64_1;
            *(uint64_t *)(target + 16) = value64_2;
            *(uint32_t *)(target + 24) = value32_0;
            *(uint16_t *)(target + 28) = uint16_t(value32_1 & 0xFFFFu);
            *(uint8_t  *)(target + 30) = uint8_t((value32_1 >> 16u) & 0xFFu);
            break;
        case 32:
            _mm256_storeu_si256(addr, src);
            break;
        default:
            break;
    }
}

template <typename T>
void left_rotate_sse_1_regs(T * first, T * mid, T * last,
                            std::size_t left_len, std::size_t right_len)
{
    const __m128i * stash_start = (const __m128i *)first;
    __m128i stash0 = _mm_loadu_si128(stash_start);

    avx_forward_move_8_load_aligned<T>(first, mid, last);

    __m128i * store_start = (__m128i *)(last - left_len);
    _mm_storeu_last<T, 0>(store_start, stash0, left_len);
}

template <typename T>
void left_rotate_avx_1_regs(T * first, T * mid, T * last,
                            std::size_t left_len, std::size_t right_len)
{
    const __m256i * stash_start = (const __m256i *)first;
    __m256i stash0 = _mm256_loadu_si256(stash_start);

    avx_forward_move_8_load_aligned<T>(first, mid, last);

    __m256i * store_start = (__m256i *)(last - left_len);
    _mm256_storeu_last<T, 0>(store_start + 0, stash0, left_len);
}

template <typename T>
void left_rotate_avx_2_regs(T * first, T * mid, T * last,
                            std::size_t left_len, std::size_t right_len)
{
    const __m256i * stash_start = (const __m256i *)first;
    __m256i stash0 = _mm256_loadu_si256(stash_start + 0);
    __m256i stash1 = _mm256_loadu_si256(stash_start + 1);

    avx_forward_move_8_load_aligned<T>(first, mid, last);

    __m256i * store_start = (__m256i *)(last - left_len);
    _mm256_storeu_si256(store_start + 0, stash0);
    _mm256_storeu_last<T, 1>(store_start + 1, stash1, left_len);
}

template <typename T>
void left_rotate_avx_3_regs(T * first, T * mid, T * last,
                            std::size_t left_len, std::size_t right_len)
{
    const __m256i * stash_start = (const __m256i *)first;
    __m256i stash0 = _mm256_loadu_si256(stash_start + 0);
    __m256i stash1 = _mm256_loadu_si256(stash_start + 1);
    __m256i stash2 = _mm256_loadu_si256(stash_start + 2);

    avx_forward_move_8_load_aligned<T>(first, mid, last);

    __m256i * store_start = (__m256i *)(last - left_len);
    _mm256_storeu_si256(store_start + 0, stash0);
    _mm256_storeu_si256(store_start + 1, stash1);
    _mm256_storeu_last<T, 2>(store_start + 2, stash2, left_len);
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

    //avx_forward_move_8_load_aligned<T>(first, mid, last);
    avx_forward_move_8_store_aligned<T>(first, mid, last);

    __m256i * store_start = (__m256i *)(last - left_len);
    _mm256_storeu_si256(store_start + 0, stash0);
    _mm256_storeu_si256(store_start + 1, stash1);
    _mm256_storeu_si256(store_start + 2, stash2);
    _mm256_storeu_last<T, 3>(store_start + 3, stash3, left_len);
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

    //avx_forward_move_8_load_aligned<T>(first, mid, last);
    avx_forward_move_8_store_aligned<T>(first, mid, last);

    __m256i * store_start = (__m256i *)(last - left_len);
    _mm256_storeu_si256(store_start + 0, stash0);
    _mm256_storeu_si256(store_start + 1, stash1);
    _mm256_storeu_si256(store_start + 2, stash2);
    _mm256_storeu_si256(store_start + 3, stash3);
    _mm256_storeu_last<T, 4>(store_start + 4, stash4, left_len);
}

template <typename T>
void left_rotate_avx_6_regs(T * first, T * mid, T * last,
                            std::size_t left_len, std::size_t right_len)
{
    const __m256i * stash_start = (const __m256i *)first;
    __m256i stash0 = _mm256_loadu_si256(stash_start + 0);
    __m256i stash1 = _mm256_loadu_si256(stash_start + 1);
    __m256i stash2 = _mm256_loadu_si256(stash_start + 2);
    __m256i stash3 = _mm256_loadu_si256(stash_start + 3);
    __m256i stash4 = _mm256_loadu_si256(stash_start + 4);
    __m256i stash5 = _mm256_loadu_si256(stash_start + 5);

    avx_forward_move_8_load_aligned<T>(first, mid, last);

    __m256i * store_start = (__m256i *)(last - left_len);
    _mm256_storeu_si256(store_start + 0, stash0);
    _mm256_storeu_si256(store_start + 1, stash1);
    _mm256_storeu_si256(store_start + 2, stash2);
    _mm256_storeu_si256(store_start + 3, stash3);
    _mm256_storeu_si256(store_start + 4, stash4);
    _mm256_storeu_last<T, 5>(store_start + 5, stash5, left_len);
}

template <typename T>
void left_rotate_avx_7_regs(T * first, T * mid, T * last,
                            std::size_t left_len, std::size_t right_len)
{
    const __m256i * stash_start = (const __m256i *)first;
    __m256i stash0 = _mm256_loadu_si256(stash_start + 0);
    __m256i stash1 = _mm256_loadu_si256(stash_start + 1);
    __m256i stash2 = _mm256_loadu_si256(stash_start + 2);
    __m256i stash3 = _mm256_loadu_si256(stash_start + 3);
    __m256i stash4 = _mm256_loadu_si256(stash_start + 4);
    __m256i stash5 = _mm256_loadu_si256(stash_start + 5);
    __m256i stash6 = _mm256_loadu_si256(stash_start + 6);

    avx_forward_move_6(first, mid, last);

    __m256i * store_start = (__m256i *)(last - left_len);
    _mm256_storeu_si256(store_start + 0, stash0);
    _mm256_storeu_si256(store_start + 1, stash1);
    _mm256_storeu_si256(store_start + 2, stash2);
    _mm256_storeu_si256(store_start + 3, stash3);
    _mm256_storeu_si256(store_start + 4, stash4);
    _mm256_storeu_si256(store_start + 5, stash5);
    _mm256_storeu_last<T, 6>(store_start + 6, stash6, left_len);
}

template <typename T>
void left_rotate_avx_8_regs(T * first, T * mid, T * last,
                            std::size_t left_len, std::size_t right_len)
{
    const __m256i * stash_start = (const __m256i *)first;
    __m256i stash0 = _mm256_loadu_si256(stash_start + 0);
    __m256i stash1 = _mm256_loadu_si256(stash_start + 1);
    __m256i stash2 = _mm256_loadu_si256(stash_start + 2);
    __m256i stash3 = _mm256_loadu_si256(stash_start + 3);
    __m256i stash4 = _mm256_loadu_si256(stash_start + 4);
    __m256i stash5 = _mm256_loadu_si256(stash_start + 5);
    __m256i stash6 = _mm256_loadu_si256(stash_start + 6);
    __m256i stash7 = _mm256_loadu_si256(stash_start + 7);

    avx_forward_move_6(first, mid, last);

    __m256i * store_start = (__m256i *)(last - left_len);
    _mm256_storeu_si256(store_start + 0, stash0);
    _mm256_storeu_si256(store_start + 1, stash1);
    _mm256_storeu_si256(store_start + 2, stash2);
    _mm256_storeu_si256(store_start + 3, stash3);
    _mm256_storeu_si256(store_start + 4, stash4);
    _mm256_storeu_si256(store_start + 5, stash5);
    _mm256_storeu_si256(store_start + 6, stash6);
    _mm256_storeu_last<T, 7>(store_start + 7, stash7, left_len);
}

template <typename T>
void left_rotate_avx_9_regs(T * first, T * mid, T * last,
                            std::size_t left_len, std::size_t right_len)
{
    const __m256i * stash_start = (const __m256i *)first;
    __m256i stash0 = _mm256_loadu_si256(stash_start + 0);
    __m256i stash1 = _mm256_loadu_si256(stash_start + 1);
    __m256i stash2 = _mm256_loadu_si256(stash_start + 2);
    __m256i stash3 = _mm256_loadu_si256(stash_start + 3);
    __m256i stash4 = _mm256_loadu_si256(stash_start + 4);
    __m256i stash5 = _mm256_loadu_si256(stash_start + 5);
    __m256i stash6 = _mm256_loadu_si256(stash_start + 6);
    __m256i stash7 = _mm256_loadu_si256(stash_start + 7);
    __m256i stash8 = _mm256_loadu_si256(stash_start + 8);

    avx_forward_move_4(first, mid, last);

    __m256i * store_start = (__m256i *)(last - left_len);
    _mm256_storeu_si256(store_start + 0, stash0);
    _mm256_storeu_si256(store_start + 1, stash1);
    _mm256_storeu_si256(store_start + 2, stash2);
    _mm256_storeu_si256(store_start + 3, stash3);
    _mm256_storeu_si256(store_start + 4, stash4);
    _mm256_storeu_si256(store_start + 5, stash5);
    _mm256_storeu_si256(store_start + 6, stash6);
    _mm256_storeu_si256(store_start + 7, stash7);
    _mm256_storeu_last<T, 8>(store_start + 8, stash8, left_len);
}

template <typename T>
void left_rotate_avx_10_regs(T * first, T * mid, T * last,
                            std::size_t left_len, std::size_t right_len)
{
    const __m256i * stash_start = (const __m256i *)first;
    __m256i stash0 = _mm256_loadu_si256(stash_start + 0);
    __m256i stash1 = _mm256_loadu_si256(stash_start + 1);
    __m256i stash2 = _mm256_loadu_si256(stash_start + 2);
    __m256i stash3 = _mm256_loadu_si256(stash_start + 3);
    __m256i stash4 = _mm256_loadu_si256(stash_start + 4);
    __m256i stash5 = _mm256_loadu_si256(stash_start + 5);
    __m256i stash6 = _mm256_loadu_si256(stash_start + 6);
    __m256i stash7 = _mm256_loadu_si256(stash_start + 7);
    __m256i stash8 = _mm256_loadu_si256(stash_start + 8);
    __m256i stash9 = _mm256_loadu_si256(stash_start + 9);

    avx_forward_move_4(first, mid, last);

    __m256i * store_start = (__m256i *)(last - left_len);
    _mm256_storeu_si256(store_start + 0, stash0);
    _mm256_storeu_si256(store_start + 1, stash1);
    _mm256_storeu_si256(store_start + 2, stash2);
    _mm256_storeu_si256(store_start + 3, stash3);
    _mm256_storeu_si256(store_start + 4, stash4);
    _mm256_storeu_si256(store_start + 5, stash5);
    _mm256_storeu_si256(store_start + 6, stash6);
    _mm256_storeu_si256(store_start + 7, stash7);
    _mm256_storeu_si256(store_start + 8, stash8);
    _mm256_storeu_last<T, 9>(store_start + 9, stash9, left_len);
}

template <typename T>
void left_rotate_avx_11_regs(T * first, T * mid, T * last,
                            std::size_t left_len, std::size_t right_len)
{
    const __m256i * stash_start = (const __m256i *)first;
    __m256i stash0 = _mm256_loadu_si256(stash_start + 0);
    __m256i stash1 = _mm256_loadu_si256(stash_start + 1);
    __m256i stash2 = _mm256_loadu_si256(stash_start + 2);
    __m256i stash3 = _mm256_loadu_si256(stash_start + 3);
    __m256i stash4 = _mm256_loadu_si256(stash_start + 4);
    __m256i stash5 = _mm256_loadu_si256(stash_start + 5);
    __m256i stash6 = _mm256_loadu_si256(stash_start + 6);
    __m256i stash7 = _mm256_loadu_si256(stash_start + 7);
    __m256i stash8 = _mm256_loadu_si256(stash_start + 8);
    __m256i stash9 = _mm256_loadu_si256(stash_start + 9);
    __m256i stash10 = _mm256_loadu_si256(stash_start + 10);

    avx_forward_move_4(first, mid, last);

    __m256i * store_start = (__m256i *)(last - left_len);
    _mm256_storeu_si256(store_start + 0, stash0);
    _mm256_storeu_si256(store_start + 1, stash1);
    _mm256_storeu_si256(store_start + 2, stash2);
    _mm256_storeu_si256(store_start + 3, stash3);
    _mm256_storeu_si256(store_start + 4, stash4);
    _mm256_storeu_si256(store_start + 5, stash5);
    _mm256_storeu_si256(store_start + 6, stash6);
    _mm256_storeu_si256(store_start + 7, stash7);
    _mm256_storeu_si256(store_start + 8, stash8);
    _mm256_storeu_si256(store_start + 9, stash9);
    _mm256_storeu_last<T, 10>(store_start + 10, stash10, left_len);
}

template <typename T>
T * left_rotate_avx(T * data, std::size_t length, std::size_t offset)
{
    //typedef T   value_type;
    typedef T * pointer;

    pointer first = data;
    pointer mid   = data + offset;
    pointer last  = data + length;

    if (kUsePrefetchHint) {
        _mm_prefetch((const char *)mid, _MM_HINT_T1);
    }

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
                        left_rotate_avx_6_regs(first, mid, last, left_len, right_len);
                        break;
                    case 6:
                        left_rotate_avx_7_regs(first, mid, last, left_len, right_len);
                        break;
                    case 7:
                        left_rotate_avx_8_regs(first, mid, last, left_len, right_len);
                        break;
                    case 8:
                        left_rotate_avx_9_regs(first, mid, last, left_len, right_len);
                        break;
                    case 9:
                        left_rotate_avx_10_regs(first, mid, last, left_len, right_len);
                        break;
                    case 10:
                        left_rotate_avx_11_regs(first, mid, last, left_len, right_len);
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

#undef PREFETCH_HINT_LEVEL

#endif // JSTD_ARRAY_ROTATE_SIMD_H
