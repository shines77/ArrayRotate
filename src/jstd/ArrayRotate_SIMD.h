
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

//
// TLB miss and L3 Cache miss
//

//
// About Single-threaded memory bandwidth
//
// Single-threaded memory bandwidth on modern CPUs is limited by max_concurrency / latency of
// the transfers from L1D to the rest of the system, not by DRAM-controller bottlenecks.
// Each core has 10 Line-Fill Buffers (LFBs) which track outstanding requests to/from L1D.
// (And 16 "superqueue" entries which track lines to/from L2).
//
// experiments show that Skylake probably has 12 LFBs, up from 10 in Broadwell. e.g.
//
// See: https://stackoverflow.com/questions/39260020/why-is-skylake-so-much-better-than-broadwell-e-for-single-threaded-memory-throug
//

namespace jstd {
namespace simd {

static const bool kUsePrefetchHint = true;
static const std::size_t kPrefetchOffset = 512;

#if defined(__GNUC__) && !defined(__clang__)
static const enum _mm_hint kPrefetchHintLevel = PREFETCH_HINT_LEVEL;
#else
static const int kPrefetchHintLevel = PREFETCH_HINT_LEVEL;
#endif

static const std::size_t kSSERegBytes = 16;
static const std::size_t kAVXRegBytes = 32;

#if defined(JSTD_X86_64)
static const std::size_t kSSERegCount = 16;
#else
static const std::size_t kSSERegCount = 8;
#endif
static const std::size_t kAVXRegCount = 16;

static const std::size_t kSSEAlignment = kSSERegBytes;
static const std::size_t kAVXAlignment = kAVXRegBytes;

static const std::size_t kSSEAlignMask = kSSEAlignment - 1;
static const std::size_t kAVXAlignMask = kAVXAlignment - 1;

static const std::size_t kSSERotateThresholdLength = 16;

#if defined(JSTD_X86_64)
static const std::size_t kMaxSSEStashBytes = (kSSERegCount - 8) * kAVXRegBytes;
#else
static const std::size_t kMaxSSEStashBytes = (kSSERegCount - 4) * kAVXRegBytes;
#endif

static const std::size_t kAVXRotateThresholdBytes = 32;
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
                    *write = std::move(*read);
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
                    --read;
                    --write;
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
                    --read;
                    --write;
                    *write = std::move(*read);
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

    JSTD_ASSERT((offset <= length), "left_rotate_simple(): Error, offset > length.");
    std::size_t right_len = length - offset;
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
    JSTD_ASSERT((last >= mid),  "simd::rotate_simple(): (last < mid)");
    JSTD_ASSERT((mid >= first), "simd::rotate_simple(): (mid < first)");
    return left_rotate_simple(first, std::size_t(last - first), std::size_t(mid - first));
}

template <typename T, bool loadIsAligned, bool storeIsAligned, int LeftUints = 7>
static
JSTD_FORCE_INLINE
void avx_forward_move_N_tailing(char * __restrict target, char * __restrict source, char * __restrict end)
{
    static const std::size_t kValueSize = sizeof(T);
    std::size_t lastUnalignedBytes = (std::size_t)end & kAVXAlignMask;
    const char * __restrict limit = end - lastUnalignedBytes;

    if (loadIsAligned && storeIsAligned) {
        if (((source + (8 * kAVXRegBytes)) <= limit) && (LeftUints >= 8)) {
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

            source += 8 * kAVXRegBytes;
            target += 8 * kAVXRegBytes;
        }

        if (((source + (4 * kAVXRegBytes)) <= limit) && (LeftUints >= 4)) {
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

        if (((source + (2 * kAVXRegBytes)) <= limit) && (LeftUints >= 2)) {
            __m256i ymm0 = _mm256_load_si256((const __m256i *)(source + 32 * 0));
            __m256i ymm1 = _mm256_load_si256((const __m256i *)(source + 32 * 1));

            _mm256_store_si256((__m256i *)(target + 32 * 0), ymm0);
            _mm256_store_si256((__m256i *)(target + 32 * 1), ymm1);

            source += 2 * kAVXRegBytes;
            target += 2 * kAVXRegBytes;
        }

        if (((source + (1 * kAVXRegBytes)) <= limit) && (LeftUints >= 1)) {
            __m256i ymm0 = _mm256_load_si256((const __m256i *)(source + 32 * 0));

            _mm256_store_si256((__m256i *)(target + 32 * 0), ymm0);

            source += 1 * kAVXRegBytes;
            target += 1 * kAVXRegBytes;
        }
    }
    else if (loadIsAligned && !storeIsAligned) {
        if (((source + (8 * kAVXRegBytes)) <= limit) && (LeftUints >= 8)) {
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

        if (((source + (4 * kAVXRegBytes)) <= limit) && (LeftUints >= 4)) {
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

        if (((source + (2 * kAVXRegBytes)) <= limit) && (LeftUints >= 2)) {
            __m256i ymm0 = _mm256_load_si256((const __m256i *)(source + 32 * 0));
            __m256i ymm1 = _mm256_load_si256((const __m256i *)(source + 32 * 1));

            _mm256_storeu_si256((__m256i *)(target + 32 * 0), ymm0);
            _mm256_storeu_si256((__m256i *)(target + 32 * 1), ymm1);

            source += 2 * kAVXRegBytes;
            target += 2 * kAVXRegBytes;
        }

        if (((source + (1 * kAVXRegBytes)) <= limit) && (LeftUints >= 1)) {
            __m256i ymm0 = _mm256_load_si256((const __m256i *)(source + 32 * 0));

            _mm256_storeu_si256((__m256i *)(target + 32 * 0), ymm0);

            source += 1 * kAVXRegBytes;
            target += 1 * kAVXRegBytes;
        }
    }
    else if (!loadIsAligned && storeIsAligned) {
        if (((source + (8 * kAVXRegBytes)) <= limit) && (LeftUints >= 8)) {
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

            source += 8 * kAVXRegBytes;
            target += 8 * kAVXRegBytes;
        }

        if (((source + (4 * kAVXRegBytes)) <= limit) && (LeftUints >= 4)) {
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

        if (((source + (2 * kAVXRegBytes)) <= limit) && (LeftUints >= 2)) {
            __m256i ymm0 = _mm256_loadu_si256((const __m256i *)(source + 32 * 0));
            __m256i ymm1 = _mm256_loadu_si256((const __m256i *)(source + 32 * 1));

            _mm256_store_si256((__m256i *)(target + 32 * 0), ymm0);
            _mm256_store_si256((__m256i *)(target + 32 * 1), ymm1);

            source += 2 * kAVXRegBytes;
            target += 2 * kAVXRegBytes;
        }

        if (((source + (1 * kAVXRegBytes)) <= limit) && (LeftUints >= 1)) {
            __m256i ymm0 = _mm256_loadu_si256((const __m256i *)(source + 32 * 0));

            _mm256_store_si256((__m256i *)(target + 32 * 0), ymm0);

            source += 1 * kAVXRegBytes;
            target += 1 * kAVXRegBytes;
        }
    }
    else {
        if (((source + (8 * kAVXRegBytes)) <= limit) && (LeftUints >= 8)) {
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

        if (((source + (4 * kAVXRegBytes)) <= limit) && (LeftUints >= 4)) {
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

        if (((source + (2 * kAVXRegBytes)) <= limit) && (LeftUints >= 2)) {
            __m256i ymm0 = _mm256_loadu_si256((const __m256i *)(source + 32 * 0));
            __m256i ymm1 = _mm256_loadu_si256((const __m256i *)(source + 32 * 1));

            _mm256_storeu_si256((__m256i *)(target + 32 * 0), ymm0);
            _mm256_storeu_si256((__m256i *)(target + 32 * 1), ymm1);

            source += 2 * kAVXRegBytes;
            target += 2 * kAVXRegBytes;
        }

        if (((source + (1 * kAVXRegBytes)) <= limit) && (LeftUints >= 1)) {
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

template <typename T, bool loadIsAligned, bool storeIsAligned, int LeftUints = 7>
static
JSTD_FORCE_INLINE
void avx_forward_move_N_tailing_nt(char * __restrict target, char * __restrict source, char * __restrict end)
{
    static const std::size_t kValueSize = sizeof(T);
    std::size_t lastUnalignedBytes = (std::size_t)end & kAVXAlignMask;
    const char * __restrict limit = end - lastUnalignedBytes;

    if (loadIsAligned && storeIsAligned) {
        if (((source + (8 * kAVXRegBytes)) <= limit) && (LeftUints >= 8)) {
            __m256i ymm0 = _mm256_load_si256((const __m256i *)(source + 32 * 0));
            __m256i ymm1 = _mm256_load_si256((const __m256i *)(source + 32 * 1));
            __m256i ymm2 = _mm256_load_si256((const __m256i *)(source + 32 * 2));
            __m256i ymm3 = _mm256_load_si256((const __m256i *)(source + 32 * 3));
            __m256i ymm4 = _mm256_load_si256((const __m256i *)(source + 32 * 4));
            __m256i ymm5 = _mm256_load_si256((const __m256i *)(source + 32 * 5));
            __m256i ymm6 = _mm256_load_si256((const __m256i *)(source + 32 * 6));
            __m256i ymm7 = _mm256_load_si256((const __m256i *)(source + 32 * 7));

            _mm256_stream_si256((__m256i *)(target + 32 * 0), ymm0);
            _mm256_stream_si256((__m256i *)(target + 32 * 1), ymm1);
            _mm256_stream_si256((__m256i *)(target + 32 * 2), ymm2);
            _mm256_stream_si256((__m256i *)(target + 32 * 3), ymm3);
            _mm256_stream_si256((__m256i *)(target + 32 * 4), ymm4);
            _mm256_stream_si256((__m256i *)(target + 32 * 5), ymm5);
            _mm256_stream_si256((__m256i *)(target + 32 * 6), ymm6);
            _mm256_stream_si256((__m256i *)(target + 32 * 7), ymm7);

            source += 8 * kAVXRegBytes;
            target += 8 * kAVXRegBytes;
        }

        if (((source + (4 * kAVXRegBytes)) <= limit) && (LeftUints >= 4)) {
            __m256i ymm0 = _mm256_load_si256((const __m256i *)(source + 32 * 0));
            __m256i ymm1 = _mm256_load_si256((const __m256i *)(source + 32 * 1));
            __m256i ymm2 = _mm256_load_si256((const __m256i *)(source + 32 * 2));
            __m256i ymm3 = _mm256_load_si256((const __m256i *)(source + 32 * 3));

            _mm256_stream_si256((__m256i *)(target + 32 * 0), ymm0);
            _mm256_stream_si256((__m256i *)(target + 32 * 1), ymm1);
            _mm256_stream_si256((__m256i *)(target + 32 * 2), ymm2);
            _mm256_stream_si256((__m256i *)(target + 32 * 3), ymm3);

            source += 4 * kAVXRegBytes;
            target += 4 * kAVXRegBytes;
        }

        if (((source + (2 * kAVXRegBytes)) <= limit) && (LeftUints >= 2)) {
            __m256i ymm0 = _mm256_load_si256((const __m256i *)(source + 32 * 0));
            __m256i ymm1 = _mm256_load_si256((const __m256i *)(source + 32 * 1));

            _mm256_stream_si256((__m256i *)(target + 32 * 0), ymm0);
            _mm256_stream_si256((__m256i *)(target + 32 * 1), ymm1);

            source += 2 * kAVXRegBytes;
            target += 2 * kAVXRegBytes;
        }

        if (((source + (1 * kAVXRegBytes)) <= limit) && (LeftUints >= 1)) {
            __m256i ymm0 = _mm256_stream_load_si256((const __m256i *)(source + 32 * 0));

            _mm256_stream_si256((__m256i *)(target + 32 * 0), ymm0);

            source += 1 * kAVXRegBytes;
            target += 1 * kAVXRegBytes;
        }
    }
    else if (loadIsAligned && !storeIsAligned) {
        if (((source + (8 * kAVXRegBytes)) <= limit) && (LeftUints >= 8)) {
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

        if (((source + (4 * kAVXRegBytes)) <= limit) && (LeftUints >= 4)) {
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

        if (((source + (2 * kAVXRegBytes)) <= limit) && (LeftUints >= 2)) {
            __m256i ymm0 = _mm256_load_si256((const __m256i *)(source + 32 * 0));
            __m256i ymm1 = _mm256_load_si256((const __m256i *)(source + 32 * 1));

            _mm256_storeu_si256((__m256i *)(target + 32 * 0), ymm0);
            _mm256_storeu_si256((__m256i *)(target + 32 * 1), ymm1);

            source += 2 * kAVXRegBytes;
            target += 2 * kAVXRegBytes;
        }

        if (((source + (1 * kAVXRegBytes)) <= limit) && (LeftUints >= 1)) {
            __m256i ymm0 = _mm256_load_si256((const __m256i *)(source + 32 * 0));

            _mm256_storeu_si256((__m256i *)(target + 32 * 0), ymm0);

            source += 1 * kAVXRegBytes;
            target += 1 * kAVXRegBytes;
        }
    }
    else if (!loadIsAligned && storeIsAligned) {
        if (((source + (8 * kAVXRegBytes)) <= limit) && (LeftUints >= 8)) {
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

            source += 8 * kAVXRegBytes;
            target += 8 * kAVXRegBytes;
        }

        if (((source + (4 * kAVXRegBytes)) <= limit) && (LeftUints >= 4)) {
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

        if (((source + (2 * kAVXRegBytes)) <= limit) && (LeftUints >= 2)) {
            __m256i ymm0 = _mm256_loadu_si256((const __m256i *)(source + 32 * 0));
            __m256i ymm1 = _mm256_loadu_si256((const __m256i *)(source + 32 * 1));

            _mm256_store_si256((__m256i *)(target + 32 * 0), ymm0);
            _mm256_store_si256((__m256i *)(target + 32 * 1), ymm1);

            source += 2 * kAVXRegBytes;
            target += 2 * kAVXRegBytes;
        }

        if (((source + (1 * kAVXRegBytes)) <= limit) && (LeftUints >= 1)) {
            __m256i ymm0 = _mm256_loadu_si256((const __m256i *)(source + 32 * 0));

            _mm256_store_si256((__m256i *)(target + 32 * 0), ymm0);

            source += 1 * kAVXRegBytes;
            target += 1 * kAVXRegBytes;
        }
    }
    else {
        if (((source + (8 * kAVXRegBytes)) <= limit) && (LeftUints >= 8)) {
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

        if (((source + (4 * kAVXRegBytes)) <= limit) && (LeftUints >= 4)) {
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

        if (((source + (2 * kAVXRegBytes)) <= limit) && (LeftUints >= 2)) {
            __m256i ymm0 = _mm256_loadu_si256((const __m256i *)(source + 32 * 0));
            __m256i ymm1 = _mm256_loadu_si256((const __m256i *)(source + 32 * 1));

            _mm256_storeu_si256((__m256i *)(target + 32 * 0), ymm0);
            _mm256_storeu_si256((__m256i *)(target + 32 * 1), ymm1);

            source += 2 * kAVXRegBytes;
            target += 2 * kAVXRegBytes;
        }

        if (((source + (1 * kAVXRegBytes)) <= limit) && (LeftUints >= 1)) {
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

//
// Code block alignment setting to 64 byte maybe better than 32,
// but it's too wasteful for code size.
//
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC push_options
#pragma GCC optimize ("align-labels=32")
#endif

template <typename T, std::size_t N = 8>
static
JSTD_NO_INLINE
void avx_forward_move_N_load_aligned(T * __restrict first, T * __restrict mid, T * __restrict last)
{
    static const std::size_t kValueSize = sizeof(T);
    static const bool kValueSizeIsPower2 = ((kValueSize & (kValueSize - 1)) == 0);
    static const bool kValueSizeIsDivisible =  (kValueSize < kAVXRegBytes) ?
                                              ((kAVXRegBytes % kValueSize) == 0) :
                                              ((kValueSize % kAVXRegBytes) == 0);
    // minimum AVX regs = 1, maximum AVX regs = 8
    static const std::size_t _N = (N == 0) ? 1 : ((N <= 8) ? N : 8);
    static const std::size_t kSingleLoopBytes = _N * kAVXRegBytes;

    std::size_t unAlignedBytes = (std::size_t)mid & kAVXAlignMask;
    bool loadAddrCanAlign;
    if (kValueSize < kAVXRegBytes)
        loadAddrCanAlign = (kValueSizeIsDivisible && ((unAlignedBytes % kValueSize) == 0));
    else
        loadAddrCanAlign = (kValueSizeIsDivisible && (unAlignedBytes == 0));

    if (likely(kValueSizeIsDivisible && loadAddrCanAlign)) {
        //unAlignedBytes = (unAlignedBytes != 0) ? (kAVXRegBytes - unAlignedBytes) : 0;
        unAlignedBytes = (kAVXRegBytes - unAlignedBytes) & kAVXAlignMask;
        while (unAlignedBytes != 0) {
            *first++ = *mid++;
            unAlignedBytes -= kValueSize;
        }

        char * __restrict target = (char * __restrict)first;
        char * __restrict source = (char * __restrict)mid;
        char * __restrict end = (char * __restrict)last;

        std::size_t lastUnalignedBytes = (std::size_t)last % kSingleLoopBytes;
        std::size_t totalBytes = (last - first) * kValueSize;
        const char * __restrict limit = (totalBytes >= kSingleLoopBytes) ? (end - lastUnalignedBytes) : source;

        bool storeAddrIsAligned = (((std::size_t)target & kAVXAlignMask) == 0);
        if (likely(!storeAddrIsAligned)) {
#if defined(__ICL)
#pragma code_align(64)
#endif
            while (source < limit) {
                __m256i ymm0, ymm1, ymm2, ymm3, ymm4, ymm5, ymm6, ymm7;
                if (N >= 0)
                    ymm0 = _mm256_load_si256((const __m256i *)(source + 32 * 0));
                if (N >= 2)
                    ymm1 = _mm256_load_si256((const __m256i *)(source + 32 * 1));
                if (N >= 3)
                    ymm2 = _mm256_load_si256((const __m256i *)(source + 32 * 2));
                if (N >= 4)
                    ymm3 = _mm256_load_si256((const __m256i *)(source + 32 * 3));
                if (N >= 5)
                    ymm4 = _mm256_load_si256((const __m256i *)(source + 32 * 4));
                if (N >= 6)
                    ymm5 = _mm256_load_si256((const __m256i *)(source + 32 * 5));
                if (N >= 7)
                    ymm6 = _mm256_load_si256((const __m256i *)(source + 32 * 6));
                // Use "{" and "}" to avoid the gcc warnings
                if (N >= 8) {
                    ymm7 = _mm256_load_si256((const __m256i *)(source + 32 * 7));
                }

                //
                // See: https://blog.csdn.net/qq_43401808/article/details/87360789
                //
                if (kUsePrefetchHint) {
                    // Here, N would be best a multiple of 2.
                    _mm_prefetch((const char *)(source + kPrefetchOffset + 64 * 0), kPrefetchHintLevel);
                    if (N >= 3)
                    _mm_prefetch((const char *)(source + kPrefetchOffset + 64 * 1), kPrefetchHintLevel);
                    if (N >= 5)
                    _mm_prefetch((const char *)(source + kPrefetchOffset + 64 * 2), kPrefetchHintLevel);
                    if (N >= 7)
                    _mm_prefetch((const char *)(source + kPrefetchOffset + 64 * 3), kPrefetchHintLevel);
                }

                source += kSingleLoopBytes;

                if (N >= 0)
                    _mm256_storeu_si256((__m256i *)(target + 32 * 0), ymm0);
                if (N >= 2)
                    _mm256_storeu_si256((__m256i *)(target + 32 * 1), ymm1);
                if (N >= 3)
                    _mm256_storeu_si256((__m256i *)(target + 32 * 2), ymm2);
                if (N >= 4)
                    _mm256_storeu_si256((__m256i *)(target + 32 * 3), ymm3);
                if (N >= 5)
                    _mm256_storeu_si256((__m256i *)(target + 32 * 4), ymm4);
                if (N >= 6)
                    _mm256_storeu_si256((__m256i *)(target + 32 * 5), ymm5);
                if (N >= 7)
                    _mm256_storeu_si256((__m256i *)(target + 32 * 6), ymm6);
                // Use "{" and "}" to avoid the gcc warnings
                if (N >= 8) {
                    _mm256_storeu_si256((__m256i *)(target + 32 * 7), ymm7);
                }

                target += kSingleLoopBytes;
            }

            avx_forward_move_N_tailing<T, kLoadIsAligned, kStoreIsNotAligned, _N - 1>(target, source, end);
        } else {
#if defined(__ICL)
#pragma code_align(64)
#endif
            while (source < limit) {
                __m256i ymm0, ymm1, ymm2, ymm3, ymm4, ymm5, ymm6, ymm7;
                if (N >= 0)
                    ymm0 = _mm256_load_si256((const __m256i *)(source + 32 * 0));
                if (N >= 2)
                    ymm1 = _mm256_load_si256((const __m256i *)(source + 32 * 1));
                if (N >= 3)
                    ymm2 = _mm256_load_si256((const __m256i *)(source + 32 * 2));
                if (N >= 4)
                    ymm3 = _mm256_load_si256((const __m256i *)(source + 32 * 3));
                if (N >= 5)
                    ymm4 = _mm256_load_si256((const __m256i *)(source + 32 * 4));
                if (N >= 6)
                    ymm5 = _mm256_load_si256((const __m256i *)(source + 32 * 5));
                if (N >= 7)
                    ymm6 = _mm256_load_si256((const __m256i *)(source + 32 * 6));
                // Use "{" and "}" to avoid the gcc warnings
                if (N >= 8) {
                    ymm7 = _mm256_load_si256((const __m256i *)(source + 32 * 7));
                }

                if (kUsePrefetchHint) {
                    // Here, N would be best a multiple of 2.
                    _mm_prefetch((const char *)(source + kPrefetchOffset + 64 * 0), kPrefetchHintLevel);
                    if (N >= 3)
                    _mm_prefetch((const char *)(source + kPrefetchOffset + 64 * 1), kPrefetchHintLevel);
                    if (N >= 5)
                    _mm_prefetch((const char *)(source + kPrefetchOffset + 64 * 2), kPrefetchHintLevel);
                    if (N >= 7)
                    _mm_prefetch((const char *)(source + kPrefetchOffset + 64 * 3), kPrefetchHintLevel);
                }

                source += kSingleLoopBytes;

                if (N >= 0)
                    _mm256_store_si256((__m256i *)(target + 32 * 0), ymm0);
                if (N >= 2)
                    _mm256_store_si256((__m256i *)(target + 32 * 1), ymm1);
                if (N >= 3)
                    _mm256_store_si256((__m256i *)(target + 32 * 2), ymm2);
                if (N >= 4)
                    _mm256_store_si256((__m256i *)(target + 32 * 3), ymm3);
                if (N >= 5)
                    _mm256_store_si256((__m256i *)(target + 32 * 4), ymm4);
                if (N >= 6)
                    _mm256_store_si256((__m256i *)(target + 32 * 5), ymm5);
                if (N >= 7)
                    _mm256_store_si256((__m256i *)(target + 32 * 6), ymm6);
                // Use "{" and "}" to avoid the gcc warnings
                if (N >= 8) {
                    _mm256_store_si256((__m256i *)(target + 32 * 7), ymm7);
                }

                target += kSingleLoopBytes;
            }

            avx_forward_move_N_tailing<T, kLoadIsAligned, kStoreIsAligned, _N - 1>(target, source, end);
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

        char * __restrict target = (char * __restrict)first;
        char * __restrict source = (char * __restrict)mid;
        char * __restrict end = (char * __restrict)last;

        std::size_t lastUnalignedBytes = (std::size_t)last % kSingleLoopBytes;
        std::size_t totalBytes = (last - first) * kValueSize;
        const char * __restrict limit = (totalBytes >= kSingleLoopBytes) ? (end - lastUnalignedBytes) : source;

        if (likely(storeAddrCanAlign)) {
#if defined(__ICL)
#pragma code_align(64)
#endif
            while (source < limit) {
                __m256i ymm0, ymm1, ymm2, ymm3, ymm4, ymm5, ymm6, ymm7;
                if (N >= 0)
                    ymm0 = _mm256_loadu_si256((const __m256i *)(source + 32 * 0));
                if (N >= 2)
                    ymm1 = _mm256_loadu_si256((const __m256i *)(source + 32 * 1));
                if (N >= 3)
                    ymm2 = _mm256_loadu_si256((const __m256i *)(source + 32 * 2));
                if (N >= 4)
                    ymm3 = _mm256_loadu_si256((const __m256i *)(source + 32 * 3));
                if (N >= 5)
                    ymm4 = _mm256_loadu_si256((const __m256i *)(source + 32 * 4));
                if (N >= 6)
                    ymm5 = _mm256_loadu_si256((const __m256i *)(source + 32 * 5));
                if (N >= 7)
                    ymm6 = _mm256_loadu_si256((const __m256i *)(source + 32 * 6));
                // Use "{" and "}" to avoid the gcc warnings
                if (N >= 8) {
                    ymm7 = _mm256_loadu_si256((const __m256i *)(source + 32 * 7));
                }

                if (kUsePrefetchHint) {
                    // Here, N would be best a multiple of 2.
                    _mm_prefetch((const char *)(source + kPrefetchOffset + 64 * 0), kPrefetchHintLevel);
                    if (N >= 3)
                    _mm_prefetch((const char *)(source + kPrefetchOffset + 64 * 1), kPrefetchHintLevel);
                    if (N >= 5)
                    _mm_prefetch((const char *)(source + kPrefetchOffset + 64 * 2), kPrefetchHintLevel);
                    if (N >= 7)
                    _mm_prefetch((const char *)(source + kPrefetchOffset + 64 * 3), kPrefetchHintLevel);
                }

                source += kSingleLoopBytes;

                if (N >= 0)
                    _mm256_store_si256((__m256i *)(target + 32 * 0), ymm0);
                if (N >= 2)
                    _mm256_store_si256((__m256i *)(target + 32 * 1), ymm1);
                if (N >= 3)
                    _mm256_store_si256((__m256i *)(target + 32 * 2), ymm2);
                if (N >= 4)
                    _mm256_store_si256((__m256i *)(target + 32 * 3), ymm3);
                if (N >= 5)
                    _mm256_store_si256((__m256i *)(target + 32 * 4), ymm4);
                if (N >= 6)
                    _mm256_store_si256((__m256i *)(target + 32 * 5), ymm5);
                if (N >= 7)
                    _mm256_store_si256((__m256i *)(target + 32 * 6), ymm6);
                // Use "{" and "}" to avoid the gcc warnings
                if (N >= 8) {
                    _mm256_store_si256((__m256i *)(target + 32 * 7), ymm7);
                }

                target += kSingleLoopBytes;
            }

            avx_forward_move_N_tailing<T, kLoadIsNotAligned, kStoreIsAligned, _N - 1>(target, source, end);
        } else {
#if defined(__ICL)
#pragma code_align(64)
#endif
            while (source < limit) {
                __m256i ymm0, ymm1, ymm2, ymm3, ymm4, ymm5, ymm6, ymm7;
                if (N >= 0)
                    ymm0 = _mm256_loadu_si256((const __m256i *)(source + 32 * 0));
                if (N >= 2)
                    ymm1 = _mm256_loadu_si256((const __m256i *)(source + 32 * 1));
                if (N >= 3)
                    ymm2 = _mm256_loadu_si256((const __m256i *)(source + 32 * 2));
                if (N >= 4)
                    ymm3 = _mm256_loadu_si256((const __m256i *)(source + 32 * 3));
                if (N >= 5)
                    ymm4 = _mm256_loadu_si256((const __m256i *)(source + 32 * 4));
                if (N >= 6)
                    ymm5 = _mm256_loadu_si256((const __m256i *)(source + 32 * 5));
                if (N >= 7)
                    ymm6 = _mm256_loadu_si256((const __m256i *)(source + 32 * 6));
                // Use "{" and "}" to avoid the gcc warnings
                if (N >= 8) {
                    ymm7 = _mm256_loadu_si256((const __m256i *)(source + 32 * 7));
                }

                if (kUsePrefetchHint) {
                    // Here, N would be best a multiple of 2.
                    _mm_prefetch((const char *)(source + kPrefetchOffset + 64 * 0), kPrefetchHintLevel);
                    if (N >= 3)
                    _mm_prefetch((const char *)(source + kPrefetchOffset + 64 * 1), kPrefetchHintLevel);
                    if (N >= 5)
                    _mm_prefetch((const char *)(source + kPrefetchOffset + 64 * 2), kPrefetchHintLevel);
                    if (N >= 7)
                    _mm_prefetch((const char *)(source + kPrefetchOffset + 64 * 3), kPrefetchHintLevel);
                }

                source += kSingleLoopBytes;

                if (N >= 0)
                    _mm256_storeu_si256((__m256i *)(target + 32 * 0), ymm0);
                if (N >= 2)
                    _mm256_storeu_si256((__m256i *)(target + 32 * 1), ymm1);
                if (N >= 3)
                    _mm256_storeu_si256((__m256i *)(target + 32 * 2), ymm2);
                if (N >= 4)
                    _mm256_storeu_si256((__m256i *)(target + 32 * 3), ymm3);
                if (N >= 5)
                    _mm256_storeu_si256((__m256i *)(target + 32 * 4), ymm4);
                if (N >= 6)
                    _mm256_storeu_si256((__m256i *)(target + 32 * 5), ymm5);
                if (N >= 7)
                    _mm256_storeu_si256((__m256i *)(target + 32 * 6), ymm6);
                // Use "{" and "}" to avoid the gcc warnings
                if (N >= 8) {
                    _mm256_storeu_si256((__m256i *)(target + 32 * 7), ymm7);
                }

                target += kSingleLoopBytes;
            }

            avx_forward_move_N_tailing<T, kLoadIsNotAligned, kStoreIsNotAligned, _N - 1>(target, source, end);
        }
    }
}

template <typename T, std::size_t N = 8>
static
JSTD_NO_INLINE
void avx_forward_move_N_store_aligned(T * __restrict first, T * __restrict mid, T * __restrict last)
{
    static const std::size_t kValueSize = sizeof(T);
    static const bool kValueSizeIsPower2 = ((kValueSize & (kValueSize - 1)) == 0);
    static const bool kValueSizeIsDivisible =  (kValueSize < kAVXRegBytes) ?
                                              ((kAVXRegBytes % kValueSize) == 0) :
                                              ((kValueSize % kAVXRegBytes) == 0);
    // minimum AVX regs = 1, maximum AVX regs = 8
    static const std::size_t _N = (N == 0) ? 1 : ((N <= 8) ? N : 8);
    static const std::size_t kSingleLoopBytes = _N * kAVXRegBytes;

    std::size_t unAlignedBytes = (std::size_t)first & kAVXAlignMask;
    bool storeAddrCanAlign;
    if (kValueSize < kAVXRegBytes)
        storeAddrCanAlign = (kValueSizeIsDivisible && ((unAlignedBytes % kValueSize) == 0));
    else
        storeAddrCanAlign = (kValueSizeIsDivisible && (unAlignedBytes == 0));

    if (likely(kValueSizeIsDivisible && storeAddrCanAlign)) {
        //unAlignedBytes = (unAlignedBytes != 0) ? (kAVXRegBytes - unAlignedBytes) : 0;
        unAlignedBytes = (kAVXRegBytes - unAlignedBytes) & kAVXAlignMask;
        while (unAlignedBytes != 0) {
            *first++ = *mid++;
            unAlignedBytes -= kValueSize;
        }

        char * __restrict target = (char * __restrict)first;
        char * __restrict source = (char * __restrict)mid;
        char * __restrict end = (char * __restrict)last;

        std::size_t lastUnalignedBytes = (std::size_t)last % kSingleLoopBytes;
        std::size_t totalBytes = (last - first) * kValueSize;
        const char * __restrict limit = (totalBytes >= kSingleLoopBytes) ? (end - lastUnalignedBytes) : source;

        bool loadAddrIsAligned = (((std::size_t)source & kAVXAlignMask) == 0);
        if (likely(!loadAddrIsAligned)) {
#if defined(__ICL)
#pragma code_align(64)
#endif
            while (source < limit) {
                __m256i ymm0, ymm1, ymm2, ymm3, ymm4, ymm5, ymm6, ymm7;
                if (N >= 0)
                    ymm0 = _mm256_loadu_si256((const __m256i *)(source + 32 * 0));
                if (N >= 2)
                    ymm1 = _mm256_loadu_si256((const __m256i *)(source + 32 * 1));
                if (N >= 3)
                    ymm2 = _mm256_loadu_si256((const __m256i *)(source + 32 * 2));
                if (N >= 4)
                    ymm3 = _mm256_loadu_si256((const __m256i *)(source + 32 * 3));
                if (N >= 5)
                    ymm4 = _mm256_loadu_si256((const __m256i *)(source + 32 * 4));
                if (N >= 6)
                    ymm5 = _mm256_loadu_si256((const __m256i *)(source + 32 * 5));
                if (N >= 7)
                    ymm6 = _mm256_loadu_si256((const __m256i *)(source + 32 * 6));
                // Use "{" and "}" to avoid the gcc warnings
                if (N >= 8) {
                    ymm7 = _mm256_loadu_si256((const __m256i *)(source + 32 * 7));
                }

                //
                // See: https://blog.csdn.net/qq_43401808/article/details/87360789
                //
                if (kUsePrefetchHint) {
                    // Here, N would be best a multiple of 2.
                    _mm_prefetch((const char *)(source + kPrefetchOffset + 64 * 0), kPrefetchHintLevel);
                    if (N >= 3)
                    _mm_prefetch((const char *)(source + kPrefetchOffset + 64 * 1), kPrefetchHintLevel);
                    if (N >= 5)
                    _mm_prefetch((const char *)(source + kPrefetchOffset + 64 * 2), kPrefetchHintLevel);
                    if (N >= 7)
                    _mm_prefetch((const char *)(source + kPrefetchOffset + 64 * 3), kPrefetchHintLevel);
                }

                source += kSingleLoopBytes;

                if (N >= 0)
                    _mm256_store_si256((__m256i *)(target + 32 * 0), ymm0);
                if (N >= 2)
                    _mm256_store_si256((__m256i *)(target + 32 * 1), ymm1);
                if (N >= 3)
                    _mm256_store_si256((__m256i *)(target + 32 * 2), ymm2);
                if (N >= 4)
                    _mm256_store_si256((__m256i *)(target + 32 * 3), ymm3);
                if (N >= 5)
                    _mm256_store_si256((__m256i *)(target + 32 * 4), ymm4);
                if (N >= 6)
                    _mm256_store_si256((__m256i *)(target + 32 * 5), ymm5);
                if (N >= 7)
                    _mm256_store_si256((__m256i *)(target + 32 * 6), ymm6);
                // Use "{" and "}" to avoid the gcc warnings
                if (N >= 8) {
                    _mm256_store_si256((__m256i *)(target + 32 * 7), ymm7);
                }

                target += kSingleLoopBytes;
            }

            avx_forward_move_N_tailing<T, kLoadIsNotAligned, kStoreIsAligned, _N - 1>(target, source, end);
        } else {
#if defined(__ICL)
#pragma code_align(64)
#endif
            while (source < limit) {
                __m256i ymm0, ymm1, ymm2, ymm3, ymm4, ymm5, ymm6, ymm7;
                if (N >= 0)
                    ymm0 = _mm256_load_si256((const __m256i *)(source + 32 * 0));
                if (N >= 2)
                    ymm1 = _mm256_load_si256((const __m256i *)(source + 32 * 1));
                if (N >= 3)
                    ymm2 = _mm256_load_si256((const __m256i *)(source + 32 * 2));
                if (N >= 4)
                    ymm3 = _mm256_load_si256((const __m256i *)(source + 32 * 3));
                if (N >= 5)
                    ymm4 = _mm256_load_si256((const __m256i *)(source + 32 * 4));
                if (N >= 6)
                    ymm5 = _mm256_load_si256((const __m256i *)(source + 32 * 5));
                if (N >= 7)
                    ymm6 = _mm256_load_si256((const __m256i *)(source + 32 * 6));
                // Use "{" and "}" to avoid the gcc warnings
                if (N >= 8) {
                    ymm7 = _mm256_load_si256((const __m256i *)(source + 32 * 7));
                }

                if (kUsePrefetchHint) {
                    // Here, N would be best a multiple of 2.
                    _mm_prefetch((const char *)(source + kPrefetchOffset + 64 * 0), kPrefetchHintLevel);
                    if (N >= 3)
                    _mm_prefetch((const char *)(source + kPrefetchOffset + 64 * 1), kPrefetchHintLevel);
                    if (N >= 5)
                    _mm_prefetch((const char *)(source + kPrefetchOffset + 64 * 2), kPrefetchHintLevel);
                    if (N >= 7)
                    _mm_prefetch((const char *)(source + kPrefetchOffset + 64 * 3), kPrefetchHintLevel);
                }

                source += kSingleLoopBytes;

                if (N >= 0)
                    _mm256_store_si256((__m256i *)(target + 32 * 0), ymm0);
                if (N >= 2)
                    _mm256_store_si256((__m256i *)(target + 32 * 1), ymm1);
                if (N >= 3)
                    _mm256_store_si256((__m256i *)(target + 32 * 2), ymm2);
                if (N >= 4)
                    _mm256_store_si256((__m256i *)(target + 32 * 3), ymm3);
                if (N >= 5)
                    _mm256_store_si256((__m256i *)(target + 32 * 4), ymm4);
                if (N >= 6)
                    _mm256_store_si256((__m256i *)(target + 32 * 5), ymm5);
                if (N >= 7)
                    _mm256_store_si256((__m256i *)(target + 32 * 6), ymm6);
                // Use "{" and "}" to avoid the gcc warnings
                if (N >= 8) {
                    _mm256_store_si256((__m256i *)(target + 32 * 7), ymm7);
                }

                target += kSingleLoopBytes;
            }

            avx_forward_move_N_tailing<T, kLoadIsAligned, kStoreIsAligned, _N - 1>(target, source, end);
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

        char * __restrict target = (char * __restrict)first;
        char * __restrict source = (char * __restrict)mid;
        char * __restrict end = (char * __restrict)last;

        std::size_t lastUnalignedBytes = (std::size_t)last % kSingleLoopBytes;
        std::size_t totalBytes = (last - first) * kValueSize;
        const char * __restrict limit = (totalBytes >= kSingleLoopBytes) ? (end - lastUnalignedBytes) : source;

        if (likely(loadAddrCanAlign)) {
#if defined(__ICL)
#pragma code_align(64)
#endif
            while (source < limit) {
                __m256i ymm0, ymm1, ymm2, ymm3, ymm4, ymm5, ymm6, ymm7;
                if (N >= 0)
                    ymm0 = _mm256_load_si256((const __m256i *)(source + 32 * 0));
                if (N >= 2)
                    ymm1 = _mm256_load_si256((const __m256i *)(source + 32 * 1));
                if (N >= 3)
                    ymm2 = _mm256_load_si256((const __m256i *)(source + 32 * 2));
                if (N >= 4)
                    ymm3 = _mm256_load_si256((const __m256i *)(source + 32 * 3));
                if (N >= 5)
                    ymm4 = _mm256_load_si256((const __m256i *)(source + 32 * 4));
                if (N >= 6)
                    ymm5 = _mm256_load_si256((const __m256i *)(source + 32 * 5));
                if (N >= 7)
                    ymm6 = _mm256_load_si256((const __m256i *)(source + 32 * 6));
                // Use "{" and "}" to avoid the gcc warnings
                if (N >= 8) {
                    ymm7 = _mm256_load_si256((const __m256i *)(source + 32 * 7));
                }

                if (kUsePrefetchHint) {
                    // Here, N would be best a multiple of 2.
                    _mm_prefetch((const char *)(source + kPrefetchOffset + 64 * 0), kPrefetchHintLevel);
                    if (N >= 3)
                    _mm_prefetch((const char *)(source + kPrefetchOffset + 64 * 1), kPrefetchHintLevel);
                    if (N >= 5)
                    _mm_prefetch((const char *)(source + kPrefetchOffset + 64 * 2), kPrefetchHintLevel);
                    if (N >= 7)
                    _mm_prefetch((const char *)(source + kPrefetchOffset + 64 * 3), kPrefetchHintLevel);
                }

                source += kSingleLoopBytes;

                if (N >= 0)
                    _mm256_storeu_si256((__m256i *)(target + 32 * 0), ymm0);
                if (N >= 2)
                    _mm256_storeu_si256((__m256i *)(target + 32 * 1), ymm1);
                if (N >= 3)
                    _mm256_storeu_si256((__m256i *)(target + 32 * 2), ymm2);
                if (N >= 4)
                    _mm256_storeu_si256((__m256i *)(target + 32 * 3), ymm3);
                if (N >= 5)
                    _mm256_storeu_si256((__m256i *)(target + 32 * 4), ymm4);
                if (N >= 6)
                    _mm256_storeu_si256((__m256i *)(target + 32 * 5), ymm5);
                if (N >= 7)
                    _mm256_storeu_si256((__m256i *)(target + 32 * 6), ymm6);
                // Use "{" and "}" to avoid the gcc warnings
                if (N >= 8) {
                    _mm256_storeu_si256((__m256i *)(target + 32 * 7), ymm7);
                }

                target += kSingleLoopBytes;
            }

            avx_forward_move_N_tailing<T, kLoadIsAligned, kStoreIsNotAligned, _N - 1>(target, source, end);
        } else {
#if defined(__ICL)
#pragma code_align(64)
#endif
            while (source < limit) {
                __m256i ymm0, ymm1, ymm2, ymm3, ymm4, ymm5, ymm6, ymm7;
                if (N >= 0)
                    ymm0 = _mm256_loadu_si256((const __m256i *)(source + 32 * 0));
                if (N >= 2)
                    ymm1 = _mm256_loadu_si256((const __m256i *)(source + 32 * 1));
                if (N >= 3)
                    ymm2 = _mm256_loadu_si256((const __m256i *)(source + 32 * 2));
                if (N >= 4)
                    ymm3 = _mm256_loadu_si256((const __m256i *)(source + 32 * 3));
                if (N >= 5)
                    ymm4 = _mm256_loadu_si256((const __m256i *)(source + 32 * 4));
                if (N >= 6)
                    ymm5 = _mm256_loadu_si256((const __m256i *)(source + 32 * 5));
                if (N >= 7)
                    ymm6 = _mm256_loadu_si256((const __m256i *)(source + 32 * 6));
                // Use "{" and "}" to avoid the gcc warnings
                if (N >= 8) {
                    ymm7 = _mm256_loadu_si256((const __m256i *)(source + 32 * 7));
                }

                if (kUsePrefetchHint) {
                    // Here, N would be best a multiple of 2.
                    _mm_prefetch((const char *)(source + kPrefetchOffset + 64 * 0), kPrefetchHintLevel);
                    if (N >= 3)
                    _mm_prefetch((const char *)(source + kPrefetchOffset + 64 * 1), kPrefetchHintLevel);
                    if (N >= 5)
                    _mm_prefetch((const char *)(source + kPrefetchOffset + 64 * 2), kPrefetchHintLevel);
                    if (N >= 7)
                    _mm_prefetch((const char *)(source + kPrefetchOffset + 64 * 3), kPrefetchHintLevel);
                }

                source += kSingleLoopBytes;

                if (N >= 0)
                    _mm256_storeu_si256((__m256i *)(target + 32 * 0), ymm0);
                if (N >= 2)
                    _mm256_storeu_si256((__m256i *)(target + 32 * 1), ymm1);
                if (N >= 3)
                    _mm256_storeu_si256((__m256i *)(target + 32 * 2), ymm2);
                if (N >= 4)
                    _mm256_storeu_si256((__m256i *)(target + 32 * 3), ymm3);
                if (N >= 5)
                    _mm256_storeu_si256((__m256i *)(target + 32 * 4), ymm4);
                if (N >= 6)
                    _mm256_storeu_si256((__m256i *)(target + 32 * 5), ymm5);
                if (N >= 7)
                    _mm256_storeu_si256((__m256i *)(target + 32 * 6), ymm6);
                // Use "{" and "}" to avoid the gcc warnings
                if (N >= 8) {
                    _mm256_storeu_si256((__m256i *)(target + 32 * 7), ymm7);
                }

                target += kSingleLoopBytes;
            }

            avx_forward_move_N_tailing<T, kLoadIsNotAligned, kStoreIsNotAligned, _N - 1>(target, source, end);
        }
    }
}

template <typename T, std::size_t N = 8>
static
JSTD_NO_INLINE
void avx_forward_move_N_store_aligned_nt(T * __restrict first, T * __restrict mid, T * __restrict last)
{
    static const std::size_t kValueSize = sizeof(T);
    static const bool kValueSizeIsPower2 = ((kValueSize & (kValueSize - 1)) == 0);
    static const bool kValueSizeIsDivisible =  (kValueSize < kAVXRegBytes) ?
                                              ((kAVXRegBytes % kValueSize) == 0) :
                                              ((kValueSize % kAVXRegBytes) == 0);
    // minimum AVX regs = 1, maximum AVX regs = 8
    static const std::size_t _N = (N == 0) ? 1 : ((N <= 8) ? N : 8);
    static const std::size_t kSingleLoopBytes = _N * kAVXRegBytes;

    std::size_t unAlignedBytes = (std::size_t)first & kAVXAlignMask;
    bool storeAddrCanAlign;
    if (kValueSize < kAVXRegBytes)
        storeAddrCanAlign = (kValueSizeIsDivisible && ((unAlignedBytes % kValueSize) == 0));
    else
        storeAddrCanAlign = (kValueSizeIsDivisible && (unAlignedBytes == 0));

    if (likely(kValueSizeIsDivisible && storeAddrCanAlign)) {
        //unAlignedBytes = (unAlignedBytes != 0) ? (kAVXRegBytes - unAlignedBytes) : 0;
        unAlignedBytes = (kAVXRegBytes - unAlignedBytes) & kAVXAlignMask;
        while (unAlignedBytes != 0) {
            *first++ = *mid++;
            unAlignedBytes -= kValueSize;
        }

        char * __restrict target = (char * __restrict)first;
        char * __restrict source = (char * __restrict)mid;
        char * __restrict end = (char * __restrict)last;

        std::size_t lastUnalignedBytes = (std::size_t)last % kSingleLoopBytes;
        std::size_t totalBytes = (last - first) * kValueSize;
        const char * __restrict limit = (totalBytes >= kSingleLoopBytes) ? (end - lastUnalignedBytes) : source;

        bool loadAddrIsAligned = (((std::size_t)source & kAVXAlignMask) == 0);
        if (likely(!loadAddrIsAligned)) {
            while (source < limit) {
                __m256i ymm0, ymm1, ymm2, ymm3, ymm4, ymm5, ymm6, ymm7;
                if (N >= 0)
                    ymm0 = _mm256_loadu_si256((const __m256i *)(source + 32 * 0));
                if (N >= 2)
                    ymm1 = _mm256_loadu_si256((const __m256i *)(source + 32 * 1));
                if (N >= 3)
                    ymm2 = _mm256_loadu_si256((const __m256i *)(source + 32 * 2));
                if (N >= 4)
                    ymm3 = _mm256_loadu_si256((const __m256i *)(source + 32 * 3));
                if (N >= 5)
                    ymm4 = _mm256_loadu_si256((const __m256i *)(source + 32 * 4));
                if (N >= 6)
                    ymm5 = _mm256_loadu_si256((const __m256i *)(source + 32 * 5));
                if (N >= 7)
                    ymm6 = _mm256_loadu_si256((const __m256i *)(source + 32 * 6));
                // Use "{" and "}" to avoid the gcc warnings
                if (N >= 8) {
                    ymm7 = _mm256_loadu_si256((const __m256i *)(source + 32 * 7));
                }

                //
                // See: https://blog.csdn.net/qq_43401808/article/details/87360789
                //
                if (kUsePrefetchHint) {
                    // Here, N would be best a multiple of 2.
                    _mm_prefetch((const char *)(source + kPrefetchOffset + 64 * 0), kPrefetchHintLevel);
                    if (N >= 3)
                    _mm_prefetch((const char *)(source + kPrefetchOffset + 64 * 1), kPrefetchHintLevel);
                    if (N >= 5)
                    _mm_prefetch((const char *)(source + kPrefetchOffset + 64 * 2), kPrefetchHintLevel);
                    if (N >= 7)
                    _mm_prefetch((const char *)(source + kPrefetchOffset + 64 * 3), kPrefetchHintLevel);
                }

                source += kSingleLoopBytes;

                if (N >= 0)
                    _mm256_store_si256((__m256i *)(target + 32 * 0), ymm0);
                if (N >= 2)
                    _mm256_store_si256((__m256i *)(target + 32 * 1), ymm1);
                if (N >= 3)
                    _mm256_store_si256((__m256i *)(target + 32 * 2), ymm2);
                if (N >= 4)
                    _mm256_store_si256((__m256i *)(target + 32 * 3), ymm3);
                if (N >= 5)
                    _mm256_store_si256((__m256i *)(target + 32 * 4), ymm4);
                if (N >= 6)
                    _mm256_store_si256((__m256i *)(target + 32 * 5), ymm5);
                if (N >= 7)
                    _mm256_store_si256((__m256i *)(target + 32 * 6), ymm6);
                // Use "{" and "}" to avoid the gcc warnings
                if (N >= 8) {
                    _mm256_store_si256((__m256i *)(target + 32 * 7), ymm7);
                }

                target += kSingleLoopBytes;
            }

            avx_forward_move_N_tailing<T, kLoadIsNotAligned, kStoreIsAligned, _N - 1>(target, source, end);
        } else {
            while (source < limit) {
                __m256i ymm0, ymm1, ymm2, ymm3, ymm4, ymm5, ymm6, ymm7;
                if (N >= 0)
                    ymm0 = _mm256_load_si256((const __m256i *)(source + 32 * 0));
                if (N >= 2)
                    ymm1 = _mm256_load_si256((const __m256i *)(source + 32 * 1));
                if (N >= 3)
                    ymm2 = _mm256_load_si256((const __m256i *)(source + 32 * 2));
                if (N >= 4)
                    ymm3 = _mm256_load_si256((const __m256i *)(source + 32 * 3));
                if (N >= 5)
                    ymm4 = _mm256_load_si256((const __m256i *)(source + 32 * 4));
                if (N >= 6)
                    ymm5 = _mm256_load_si256((const __m256i *)(source + 32 * 5));
                if (N >= 7)
                    ymm6 = _mm256_load_si256((const __m256i *)(source + 32 * 6));
                // Use "{" and "}" to avoid the gcc warnings
                if (N >= 8) {
                    ymm7 = _mm256_load_si256((const __m256i *)(source + 32 * 7));
                }

                if (kUsePrefetchHint) {
                    // Here, N would be best a multiple of 2.
                    _mm_prefetch((const char *)(source + kPrefetchOffset + 64 * 0), kPrefetchHintLevel);
                    if (N >= 3)
                    _mm_prefetch((const char *)(source + kPrefetchOffset + 64 * 1), kPrefetchHintLevel);
                    if (N >= 5)
                    _mm_prefetch((const char *)(source + kPrefetchOffset + 64 * 2), kPrefetchHintLevel);
                    if (N >= 7)
                    _mm_prefetch((const char *)(source + kPrefetchOffset + 64 * 3), kPrefetchHintLevel);
                }

                source += kSingleLoopBytes;

                if (N >= 0)
                    _mm256_stream_si256((__m256i *)(target + 32 * 0), ymm0);
                if (N >= 2)
                    _mm256_stream_si256((__m256i *)(target + 32 * 1), ymm1);
                if (N >= 3)
                    _mm256_stream_si256((__m256i *)(target + 32 * 2), ymm2);
                if (N >= 4)
                    _mm256_stream_si256((__m256i *)(target + 32 * 3), ymm3);
                if (N >= 5)
                    _mm256_stream_si256((__m256i *)(target + 32 * 4), ymm4);
                if (N >= 6)
                    _mm256_stream_si256((__m256i *)(target + 32 * 5), ymm5);
                if (N >= 7)
                    _mm256_stream_si256((__m256i *)(target + 32 * 6), ymm6);
                // Use "{" and "}" to avoid the gcc warnings
                if (N >= 8) {
                    _mm256_stream_si256((__m256i *)(target + 32 * 7), ymm7);
                }

                target += kSingleLoopBytes;
            }

            avx_forward_move_N_tailing_nt<T, kLoadIsAligned, kStoreIsAligned, _N - 1>(target, source, end);

            _mm_sfence();
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

        char * __restrict target = (char * __restrict)first;
        char * __restrict source = (char * __restrict)mid;
        char * __restrict end = (char * __restrict)last;

        std::size_t lastUnalignedBytes = (std::size_t)last % kSingleLoopBytes;
        std::size_t totalBytes = (last - first) * kValueSize;
        const char * __restrict limit = (totalBytes >= kSingleLoopBytes) ? (end - lastUnalignedBytes) : source;

        if (likely(loadAddrCanAlign)) {
            while (source < limit) {
                __m256i ymm0, ymm1, ymm2, ymm3, ymm4, ymm5, ymm6, ymm7;
                if (N >= 0)
                    ymm0 = _mm256_load_si256((const __m256i *)(source + 32 * 0));
                if (N >= 2)
                    ymm1 = _mm256_load_si256((const __m256i *)(source + 32 * 1));
                if (N >= 3)
                    ymm2 = _mm256_load_si256((const __m256i *)(source + 32 * 2));
                if (N >= 4)
                    ymm3 = _mm256_load_si256((const __m256i *)(source + 32 * 3));
                if (N >= 5)
                    ymm4 = _mm256_load_si256((const __m256i *)(source + 32 * 4));
                if (N >= 6)
                    ymm5 = _mm256_load_si256((const __m256i *)(source + 32 * 5));
                if (N >= 7)
                    ymm6 = _mm256_load_si256((const __m256i *)(source + 32 * 6));
                // Use "{" and "}" to avoid the gcc warnings
                if (N >= 8) {
                    ymm7 = _mm256_load_si256((const __m256i *)(source + 32 * 7));
                }

                if (kUsePrefetchHint) {
                    // Here, N would be best a multiple of 2.
                    _mm_prefetch((const char *)(source + kPrefetchOffset + 64 * 0), kPrefetchHintLevel);
                    if (N >= 3)
                    _mm_prefetch((const char *)(source + kPrefetchOffset + 64 * 1), kPrefetchHintLevel);
                    if (N >= 5)
                    _mm_prefetch((const char *)(source + kPrefetchOffset + 64 * 2), kPrefetchHintLevel);
                    if (N >= 7)
                    _mm_prefetch((const char *)(source + kPrefetchOffset + 64 * 3), kPrefetchHintLevel);
                }

                source += kSingleLoopBytes;

                if (N >= 0)
                    _mm256_storeu_si256((__m256i *)(target + 32 * 0), ymm0);
                if (N >= 2)
                    _mm256_storeu_si256((__m256i *)(target + 32 * 1), ymm1);
                if (N >= 3)
                    _mm256_storeu_si256((__m256i *)(target + 32 * 2), ymm2);
                if (N >= 4)
                    _mm256_storeu_si256((__m256i *)(target + 32 * 3), ymm3);
                if (N >= 5)
                    _mm256_storeu_si256((__m256i *)(target + 32 * 4), ymm4);
                if (N >= 6)
                    _mm256_storeu_si256((__m256i *)(target + 32 * 5), ymm5);
                if (N >= 7)
                    _mm256_storeu_si256((__m256i *)(target + 32 * 6), ymm6);
                // Use "{" and "}" to avoid the gcc warnings
                if (N >= 8) {
                    _mm256_storeu_si256((__m256i *)(target + 32 * 7), ymm7);
                }

                target += kSingleLoopBytes;
            }

            avx_forward_move_N_tailing<T, kLoadIsAligned, kStoreIsNotAligned, _N - 1>(target, source, end);
        } else {
            while (source < limit) {
                __m256i ymm0, ymm1, ymm2, ymm3, ymm4, ymm5, ymm6, ymm7;
                if (N >= 0)
                    ymm0 = _mm256_loadu_si256((const __m256i *)(source + 32 * 0));
                if (N >= 2)
                    ymm1 = _mm256_loadu_si256((const __m256i *)(source + 32 * 1));
                if (N >= 3)
                    ymm2 = _mm256_loadu_si256((const __m256i *)(source + 32 * 2));
                if (N >= 4)
                    ymm3 = _mm256_loadu_si256((const __m256i *)(source + 32 * 3));
                if (N >= 5)
                    ymm4 = _mm256_loadu_si256((const __m256i *)(source + 32 * 4));
                if (N >= 6)
                    ymm5 = _mm256_loadu_si256((const __m256i *)(source + 32 * 5));
                if (N >= 7)
                    ymm6 = _mm256_loadu_si256((const __m256i *)(source + 32 * 6));
                // Use "{" and "}" to avoid the gcc warnings
                if (N >= 8) {
                    ymm7 = _mm256_loadu_si256((const __m256i *)(source + 32 * 7));
                }

                if (kUsePrefetchHint) {
                    // Here, N would be best a multiple of 2.
                    _mm_prefetch((const char *)(source + kPrefetchOffset + 64 * 0), kPrefetchHintLevel);
                    if (N >= 3)
                    _mm_prefetch((const char *)(source + kPrefetchOffset + 64 * 1), kPrefetchHintLevel);
                    if (N >= 5)
                    _mm_prefetch((const char *)(source + kPrefetchOffset + 64 * 2), kPrefetchHintLevel);
                    if (N >= 7)
                    _mm_prefetch((const char *)(source + kPrefetchOffset + 64 * 3), kPrefetchHintLevel);
                }

                source += kSingleLoopBytes;

                if (N >= 0)
                    _mm256_storeu_si256((__m256i *)(target + 32 * 0), ymm0);
                if (N >= 2)
                    _mm256_storeu_si256((__m256i *)(target + 32 * 1), ymm1);
                if (N >= 3)
                    _mm256_storeu_si256((__m256i *)(target + 32 * 2), ymm2);
                if (N >= 4)
                    _mm256_storeu_si256((__m256i *)(target + 32 * 3), ymm3);
                if (N >= 5)
                    _mm256_storeu_si256((__m256i *)(target + 32 * 4), ymm4);
                if (N >= 6)
                    _mm256_storeu_si256((__m256i *)(target + 32 * 5), ymm5);
                if (N >= 7)
                    _mm256_storeu_si256((__m256i *)(target + 32 * 6), ymm6);
                // Use "{" and "}" to avoid the gcc warnings
                if (N >= 8) {
                    _mm256_storeu_si256((__m256i *)(target + 32 * 7), ymm7);
                }

                target += kSingleLoopBytes;
            }

            avx_forward_move_N_tailing<T, kLoadIsNotAligned, kStoreIsNotAligned, _N - 1>(target, source, end);
        }
    }
}

template <typename T, std::size_t N = 8>
static
JSTD_NO_INLINE
void avx_forward_move_Nx2_load_aligned(T * __restrict first, T * __restrict mid, T * __restrict last)
{
    static const std::size_t kValueSize = sizeof(T);
    static const bool kValueSizeIsPower2 = ((kValueSize & (kValueSize - 1)) == 0);
    static const bool kValueSizeIsDivisible =  (kValueSize < kAVXRegBytes) ?
                                              ((kAVXRegBytes % kValueSize) == 0) :
                                              ((kValueSize % kAVXRegBytes) == 0);
    // minimum AVX regs = 1 * 2, maximum AVX regs = 8 * 2
    static const std::size_t _N = (N == 0) ? 1 : ((N <= 8) ? N : 8);
    static const std::size_t kHalfLoopBytes = _N * kAVXRegBytes;
    static const std::size_t kSingleLoopBytes = kHalfLoopBytes * 2;

    std::size_t unAlignedBytes = (std::size_t)mid & kAVXAlignMask;
    bool loadAddrCanAlign;
    if (kValueSize < kAVXRegBytes)
        loadAddrCanAlign = (kValueSizeIsDivisible && ((unAlignedBytes % kValueSize) == 0));
    else
        loadAddrCanAlign = (kValueSizeIsDivisible && (unAlignedBytes == 0));

    if (likely(kValueSizeIsDivisible && loadAddrCanAlign)) {
        //unAlignedBytes = (unAlignedBytes != 0) ? (kAVXRegBytes - unAlignedBytes) : 0;
        unAlignedBytes = (kAVXRegBytes - unAlignedBytes) & kAVXAlignMask;
        while (unAlignedBytes != 0) {
            *first++ = *mid++;
            unAlignedBytes -= kValueSize;
        }

        char * __restrict target = (char * __restrict)first;
        char * __restrict source = (char * __restrict)mid;
        char * __restrict end = (char * __restrict)last;

        std::size_t lastUnalignedBytes = (std::size_t)last % kSingleLoopBytes;
        std::size_t totalBytes = (last - first) * kValueSize;
        const char * __restrict limit = (totalBytes >= kSingleLoopBytes) ? (end - lastUnalignedBytes) : source;

        bool storeAddrIsAligned = (((std::size_t)target & kAVXAlignMask) == 0);
        if (likely(!storeAddrIsAligned)) {
#if defined(__ICL)
#pragma code_align(64)
#endif
            while (source < limit) {
                __m256i ymm0, ymm1, ymm2, ymm3, ymm4, ymm5, ymm6, ymm7;
                if (N >= 0)
                    ymm0 = _mm256_load_si256((const __m256i *)(source + 32 * 0));
                if (N >= 2)
                    ymm1 = _mm256_load_si256((const __m256i *)(source + 32 * 1));
                if (N >= 3)
                    ymm2 = _mm256_load_si256((const __m256i *)(source + 32 * 2));
                if (N >= 4)
                    ymm3 = _mm256_load_si256((const __m256i *)(source + 32 * 3));
                if (N >= 5)
                    ymm4 = _mm256_load_si256((const __m256i *)(source + 32 * 4));
                if (N >= 6)
                    ymm5 = _mm256_load_si256((const __m256i *)(source + 32 * 5));
                if (N >= 7)
                    ymm6 = _mm256_load_si256((const __m256i *)(source + 32 * 6));
                // Use "{" and "}" to avoid the gcc warnings
                if (N >= 8) {
                    ymm7 = _mm256_load_si256((const __m256i *)(source + 32 * 7));
                }

                //
                // See: https://blog.csdn.net/qq_43401808/article/details/87360789
                //
                if (kUsePrefetchHint) {
                    // Here, N would be best a multiple of 2.
                    _mm_prefetch((const char *)(source + kPrefetchOffset + 64 * 0), kPrefetchHintLevel);
                    if (N >= 3)
                    _mm_prefetch((const char *)(source + kPrefetchOffset + 64 * 1), kPrefetchHintLevel);
                    if (N >= 5)
                    _mm_prefetch((const char *)(source + kPrefetchOffset + 64 * 2), kPrefetchHintLevel);
                    if (N >= 7)
                    _mm_prefetch((const char *)(source + kPrefetchOffset + 64 * 3), kPrefetchHintLevel);
                }

                source += kHalfLoopBytes;

                if (N >= 0)
                    _mm256_storeu_si256((__m256i *)(target + 32 * 0), ymm0);
                if (N >= 2)
                    _mm256_storeu_si256((__m256i *)(target + 32 * 1), ymm1);
                if (N >= 3)
                    _mm256_storeu_si256((__m256i *)(target + 32 * 2), ymm2);
                if (N >= 4)
                    _mm256_storeu_si256((__m256i *)(target + 32 * 3), ymm3);
                if (N >= 5)
                    _mm256_storeu_si256((__m256i *)(target + 32 * 4), ymm4);
                if (N >= 6)
                    _mm256_storeu_si256((__m256i *)(target + 32 * 5), ymm5);
                if (N >= 7)
                    _mm256_storeu_si256((__m256i *)(target + 32 * 6), ymm6);
                // Use "{" and "}" to avoid the gcc warnings
                if (N >= 8) {
                    _mm256_storeu_si256((__m256i *)(target + 32 * 7), ymm7);
                }

                target += kHalfLoopBytes;

                /////////////////////////////// Half loop //////////////////////////////////

                if (N >= 0)
                    ymm0 = _mm256_load_si256((const __m256i *)(source + 32 * 0));
                if (N >= 2)
                    ymm1 = _mm256_load_si256((const __m256i *)(source + 32 * 1));
                if (N >= 3)
                    ymm2 = _mm256_load_si256((const __m256i *)(source + 32 * 2));
                if (N >= 4)
                    ymm3 = _mm256_load_si256((const __m256i *)(source + 32 * 3));
                if (N >= 5)
                    ymm4 = _mm256_load_si256((const __m256i *)(source + 32 * 4));
                if (N >= 6)
                    ymm5 = _mm256_load_si256((const __m256i *)(source + 32 * 5));
                if (N >= 7)
                    ymm6 = _mm256_load_si256((const __m256i *)(source + 32 * 6));
                // Use "{" and "}" to avoid the gcc warnings
                if (N >= 8) {
                    ymm7 = _mm256_load_si256((const __m256i *)(source + 32 * 7));
                }

                //
                // See: https://blog.csdn.net/qq_43401808/article/details/87360789
                //
                if (kUsePrefetchHint) {
                    // Here, N would be best a multiple of 2.
                    _mm_prefetch((const char *)(source + kPrefetchOffset + 64 * 0), kPrefetchHintLevel);
                    if (N >= 3)
                    _mm_prefetch((const char *)(source + kPrefetchOffset + 64 * 1), kPrefetchHintLevel);
                    if (N >= 5)
                    _mm_prefetch((const char *)(source + kPrefetchOffset + 64 * 2), kPrefetchHintLevel);
                    if (N >= 7)
                    _mm_prefetch((const char *)(source + kPrefetchOffset + 64 * 3), kPrefetchHintLevel);
                }

                source += kHalfLoopBytes;

                if (N >= 0)
                    _mm256_storeu_si256((__m256i *)(target + 32 * 0), ymm0);
                if (N >= 2)
                    _mm256_storeu_si256((__m256i *)(target + 32 * 1), ymm1);
                if (N >= 3)
                    _mm256_storeu_si256((__m256i *)(target + 32 * 2), ymm2);
                if (N >= 4)
                    _mm256_storeu_si256((__m256i *)(target + 32 * 3), ymm3);
                if (N >= 5)
                    _mm256_storeu_si256((__m256i *)(target + 32 * 4), ymm4);
                if (N >= 6)
                    _mm256_storeu_si256((__m256i *)(target + 32 * 5), ymm5);
                if (N >= 7)
                    _mm256_storeu_si256((__m256i *)(target + 32 * 6), ymm6);
                // Use "{" and "}" to avoid the gcc warnings
                if (N >= 8) {
                    _mm256_storeu_si256((__m256i *)(target + 32 * 7), ymm7);
                }

                target += kHalfLoopBytes;
            }

            avx_forward_move_N_tailing<T, kLoadIsAligned, kStoreIsNotAligned, (_N * 2 - 1)>(target, source, end);
            //////////////////////////////////////////////////////////////////////////////////////////////////////
        } else {
#if defined(__ICL)
#pragma code_align(64)
#endif
            while (source < limit) {
                __m256i ymm0, ymm1, ymm2, ymm3, ymm4, ymm5, ymm6, ymm7;
                if (N >= 0)
                    ymm0 = _mm256_load_si256((const __m256i *)(source + 32 * 0));
                if (N >= 2)
                    ymm1 = _mm256_load_si256((const __m256i *)(source + 32 * 1));
                if (N >= 3)
                    ymm2 = _mm256_load_si256((const __m256i *)(source + 32 * 2));
                if (N >= 4)
                    ymm3 = _mm256_load_si256((const __m256i *)(source + 32 * 3));
                if (N >= 5)
                    ymm4 = _mm256_load_si256((const __m256i *)(source + 32 * 4));
                if (N >= 6)
                    ymm5 = _mm256_load_si256((const __m256i *)(source + 32 * 5));
                if (N >= 7)
                    ymm6 = _mm256_load_si256((const __m256i *)(source + 32 * 6));
                // Use "{" and "}" to avoid the gcc warnings
                if (N >= 8) {
                    ymm7 = _mm256_load_si256((const __m256i *)(source + 32 * 7));
                }

                if (kUsePrefetchHint) {
                    // Here, N would be best a multiple of 2.
                    _mm_prefetch((const char *)(source + kPrefetchOffset + 64 * 0), kPrefetchHintLevel);
                    if (N >= 3)
                    _mm_prefetch((const char *)(source + kPrefetchOffset + 64 * 1), kPrefetchHintLevel);
                    if (N >= 5)
                    _mm_prefetch((const char *)(source + kPrefetchOffset + 64 * 2), kPrefetchHintLevel);
                    if (N >= 7)
                    _mm_prefetch((const char *)(source + kPrefetchOffset + 64 * 3), kPrefetchHintLevel);
                }

                source += kHalfLoopBytes;

                if (N >= 0)
                    _mm256_store_si256((__m256i *)(target + 32 * 0), ymm0);
                if (N >= 2)
                    _mm256_store_si256((__m256i *)(target + 32 * 1), ymm1);
                if (N >= 3)
                    _mm256_store_si256((__m256i *)(target + 32 * 2), ymm2);
                if (N >= 4)
                    _mm256_store_si256((__m256i *)(target + 32 * 3), ymm3);
                if (N >= 5)
                    _mm256_store_si256((__m256i *)(target + 32 * 4), ymm4);
                if (N >= 6)
                    _mm256_store_si256((__m256i *)(target + 32 * 5), ymm5);
                if (N >= 7)
                    _mm256_store_si256((__m256i *)(target + 32 * 6), ymm6);
                // Use "{" and "}" to avoid the gcc warnings
                if (N >= 8) {
                    _mm256_store_si256((__m256i *)(target + 32 * 7), ymm7);
                }

                target += kHalfLoopBytes;

                /////////////////////////////// Half loop //////////////////////////////////

                if (N >= 0)
                    ymm0 = _mm256_load_si256((const __m256i *)(source + 32 * 0));
                if (N >= 2)
                    ymm1 = _mm256_load_si256((const __m256i *)(source + 32 * 1));
                if (N >= 3)
                    ymm2 = _mm256_load_si256((const __m256i *)(source + 32 * 2));
                if (N >= 4)
                    ymm3 = _mm256_load_si256((const __m256i *)(source + 32 * 3));
                if (N >= 5)
                    ymm4 = _mm256_load_si256((const __m256i *)(source + 32 * 4));
                if (N >= 6)
                    ymm5 = _mm256_load_si256((const __m256i *)(source + 32 * 5));
                if (N >= 7)
                    ymm6 = _mm256_load_si256((const __m256i *)(source + 32 * 6));
                // Use "{" and "}" to avoid the gcc warnings
                if (N >= 8) {
                    ymm7 = _mm256_load_si256((const __m256i *)(source + 32 * 7));
                }

                if (kUsePrefetchHint) {
                    // Here, N would be best a multiple of 2.
                    _mm_prefetch((const char *)(source + kPrefetchOffset + 64 * 0), kPrefetchHintLevel);
                    if (N >= 3)
                    _mm_prefetch((const char *)(source + kPrefetchOffset + 64 * 1), kPrefetchHintLevel);
                    if (N >= 5)
                    _mm_prefetch((const char *)(source + kPrefetchOffset + 64 * 2), kPrefetchHintLevel);
                    if (N >= 7)
                    _mm_prefetch((const char *)(source + kPrefetchOffset + 64 * 3), kPrefetchHintLevel);
                }

                source += kHalfLoopBytes;

                if (N >= 0)
                    _mm256_store_si256((__m256i *)(target + 32 * 0), ymm0);
                if (N >= 2)
                    _mm256_store_si256((__m256i *)(target + 32 * 1), ymm1);
                if (N >= 3)
                    _mm256_store_si256((__m256i *)(target + 32 * 2), ymm2);
                if (N >= 4)
                    _mm256_store_si256((__m256i *)(target + 32 * 3), ymm3);
                if (N >= 5)
                    _mm256_store_si256((__m256i *)(target + 32 * 4), ymm4);
                if (N >= 6)
                    _mm256_store_si256((__m256i *)(target + 32 * 5), ymm5);
                if (N >= 7)
                    _mm256_store_si256((__m256i *)(target + 32 * 6), ymm6);
                // Use "{" and "}" to avoid the gcc warnings
                if (N >= 8) {
                    _mm256_store_si256((__m256i *)(target + 32 * 7), ymm7);
                }

                target += kHalfLoopBytes;
            }

            avx_forward_move_N_tailing<T, kLoadIsAligned, kStoreIsAligned, (_N * 2 - 1)>(target, source, end);
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

        char * __restrict target = (char * __restrict)first;
        char * __restrict source = (char * __restrict)mid;
        char * __restrict end = (char * __restrict)last;

        std::size_t lastUnalignedBytes = (std::size_t)last % kSingleLoopBytes;
        std::size_t totalBytes = (last - first) * kValueSize;
        const char * __restrict limit = (totalBytes >= kSingleLoopBytes) ? (end - lastUnalignedBytes) : source;

        if (likely(storeAddrCanAlign)) {
#if defined(__ICL)
#pragma code_align(64)
#endif
            while (source < limit) {
                __m256i ymm0, ymm1, ymm2, ymm3, ymm4, ymm5, ymm6, ymm7;
                if (N >= 0)
                    ymm0 = _mm256_loadu_si256((const __m256i *)(source + 32 * 0));
                if (N >= 2)
                    ymm1 = _mm256_loadu_si256((const __m256i *)(source + 32 * 1));
                if (N >= 3)
                    ymm2 = _mm256_loadu_si256((const __m256i *)(source + 32 * 2));
                if (N >= 4)
                    ymm3 = _mm256_loadu_si256((const __m256i *)(source + 32 * 3));
                if (N >= 5)
                    ymm4 = _mm256_loadu_si256((const __m256i *)(source + 32 * 4));
                if (N >= 6)
                    ymm5 = _mm256_loadu_si256((const __m256i *)(source + 32 * 5));
                if (N >= 7)
                    ymm6 = _mm256_loadu_si256((const __m256i *)(source + 32 * 6));
                // Use "{" and "}" to avoid the gcc warnings
                if (N >= 8) {
                    ymm7 = _mm256_loadu_si256((const __m256i *)(source + 32 * 7));
                }

                if (kUsePrefetchHint) {
                    // Here, N would be best a multiple of 2.
                    _mm_prefetch((const char *)(source + kPrefetchOffset + 64 * 0), kPrefetchHintLevel);
                    if (N >= 3)
                    _mm_prefetch((const char *)(source + kPrefetchOffset + 64 * 1), kPrefetchHintLevel);
                    if (N >= 5)
                    _mm_prefetch((const char *)(source + kPrefetchOffset + 64 * 2), kPrefetchHintLevel);
                    if (N >= 7)
                    _mm_prefetch((const char *)(source + kPrefetchOffset + 64 * 3), kPrefetchHintLevel);
                }

                source += kHalfLoopBytes;

                if (N >= 0)
                    _mm256_store_si256((__m256i *)(target + 32 * 0), ymm0);
                if (N >= 2)
                    _mm256_store_si256((__m256i *)(target + 32 * 1), ymm1);
                if (N >= 3)
                    _mm256_store_si256((__m256i *)(target + 32 * 2), ymm2);
                if (N >= 4)
                    _mm256_store_si256((__m256i *)(target + 32 * 3), ymm3);
                if (N >= 5)
                    _mm256_store_si256((__m256i *)(target + 32 * 4), ymm4);
                if (N >= 6)
                    _mm256_store_si256((__m256i *)(target + 32 * 5), ymm5);
                if (N >= 7)
                    _mm256_store_si256((__m256i *)(target + 32 * 6), ymm6);
                // Use "{" and "}" to avoid the gcc warnings
                if (N >= 8) {
                    _mm256_store_si256((__m256i *)(target + 32 * 7), ymm7);
                }

                target += kHalfLoopBytes;

                /////////////////////////////// Half loop //////////////////////////////////

                if (N >= 0)
                    ymm0 = _mm256_loadu_si256((const __m256i *)(source + 32 * 0));
                if (N >= 2)
                    ymm1 = _mm256_loadu_si256((const __m256i *)(source + 32 * 1));
                if (N >= 3)
                    ymm2 = _mm256_loadu_si256((const __m256i *)(source + 32 * 2));
                if (N >= 4)
                    ymm3 = _mm256_loadu_si256((const __m256i *)(source + 32 * 3));
                if (N >= 5)
                    ymm4 = _mm256_loadu_si256((const __m256i *)(source + 32 * 4));
                if (N >= 6)
                    ymm5 = _mm256_loadu_si256((const __m256i *)(source + 32 * 5));
                if (N >= 7)
                    ymm6 = _mm256_loadu_si256((const __m256i *)(source + 32 * 6));
                // Use "{" and "}" to avoid the gcc warnings
                if (N >= 8) {
                    ymm7 = _mm256_loadu_si256((const __m256i *)(source + 32 * 7));
                }

                if (kUsePrefetchHint) {
                    // Here, N would be best a multiple of 2.
                    _mm_prefetch((const char *)(source + kPrefetchOffset + 64 * 0), kPrefetchHintLevel);
                    if (N >= 3)
                    _mm_prefetch((const char *)(source + kPrefetchOffset + 64 * 1), kPrefetchHintLevel);
                    if (N >= 5)
                    _mm_prefetch((const char *)(source + kPrefetchOffset + 64 * 2), kPrefetchHintLevel);
                    if (N >= 7)
                    _mm_prefetch((const char *)(source + kPrefetchOffset + 64 * 3), kPrefetchHintLevel);
                }

                source += kHalfLoopBytes;

                if (N >= 0)
                    _mm256_store_si256((__m256i *)(target + 32 * 0), ymm0);
                if (N >= 2)
                    _mm256_store_si256((__m256i *)(target + 32 * 1), ymm1);
                if (N >= 3)
                    _mm256_store_si256((__m256i *)(target + 32 * 2), ymm2);
                if (N >= 4)
                    _mm256_store_si256((__m256i *)(target + 32 * 3), ymm3);
                if (N >= 5)
                    _mm256_store_si256((__m256i *)(target + 32 * 4), ymm4);
                if (N >= 6)
                    _mm256_store_si256((__m256i *)(target + 32 * 5), ymm5);
                if (N >= 7)
                    _mm256_store_si256((__m256i *)(target + 32 * 6), ymm6);
                // Use "{" and "}" to avoid the gcc warnings
                if (N >= 8) {
                    _mm256_store_si256((__m256i *)(target + 32 * 7), ymm7);
                }

                target += kHalfLoopBytes;
            }

            avx_forward_move_N_tailing<T, kLoadIsNotAligned, kStoreIsAligned, (_N * 2 - 1)>(target, source, end);
            ////////////////////////////////////////////////////////////////////////////////////////////////
        } else {
#if defined(__ICL)
#pragma code_align(64)
#endif
            while (source < limit) {
                __m256i ymm0, ymm1, ymm2, ymm3, ymm4, ymm5, ymm6, ymm7;
                if (N >= 0)
                    ymm0 = _mm256_loadu_si256((const __m256i *)(source + 32 * 0));
                if (N >= 2)
                    ymm1 = _mm256_loadu_si256((const __m256i *)(source + 32 * 1));
                if (N >= 3)
                    ymm2 = _mm256_loadu_si256((const __m256i *)(source + 32 * 2));
                if (N >= 4)
                    ymm3 = _mm256_loadu_si256((const __m256i *)(source + 32 * 3));
                if (N >= 5)
                    ymm4 = _mm256_loadu_si256((const __m256i *)(source + 32 * 4));
                if (N >= 6)
                    ymm5 = _mm256_loadu_si256((const __m256i *)(source + 32 * 5));
                if (N >= 7)
                    ymm6 = _mm256_loadu_si256((const __m256i *)(source + 32 * 6));
                // Use "{" and "}" to avoid the gcc warnings
                if (N >= 8) {
                    ymm7 = _mm256_loadu_si256((const __m256i *)(source + 32 * 7));
                }

                if (kUsePrefetchHint) {
                    // Here, N would be best a multiple of 2.
                    _mm_prefetch((const char *)(source + kPrefetchOffset + 64 * 0), kPrefetchHintLevel);
                    if (N >= 3)
                    _mm_prefetch((const char *)(source + kPrefetchOffset + 64 * 1), kPrefetchHintLevel);
                    if (N >= 5)
                    _mm_prefetch((const char *)(source + kPrefetchOffset + 64 * 2), kPrefetchHintLevel);
                    if (N >= 7)
                    _mm_prefetch((const char *)(source + kPrefetchOffset + 64 * 3), kPrefetchHintLevel);
                }

                source += kHalfLoopBytes;

                if (N >= 0)
                    _mm256_storeu_si256((__m256i *)(target + 32 * 0), ymm0);
                if (N >= 2)
                    _mm256_storeu_si256((__m256i *)(target + 32 * 1), ymm1);
                if (N >= 3)
                    _mm256_storeu_si256((__m256i *)(target + 32 * 2), ymm2);
                if (N >= 4)
                    _mm256_storeu_si256((__m256i *)(target + 32 * 3), ymm3);
                if (N >= 5)
                    _mm256_storeu_si256((__m256i *)(target + 32 * 4), ymm4);
                if (N >= 6)
                    _mm256_storeu_si256((__m256i *)(target + 32 * 5), ymm5);
                if (N >= 7)
                    _mm256_storeu_si256((__m256i *)(target + 32 * 6), ymm6);
                // Use "{" and "}" to avoid the gcc warnings
                if (N >= 8) {
                    _mm256_storeu_si256((__m256i *)(target + 32 * 7), ymm7);
                }

                target += kHalfLoopBytes;

                /////////////////////////////// Half loop //////////////////////////////////

                if (N >= 0)
                    ymm0 = _mm256_loadu_si256((const __m256i *)(source + 32 * 0));
                if (N >= 2)
                    ymm1 = _mm256_loadu_si256((const __m256i *)(source + 32 * 1));
                if (N >= 3)
                    ymm2 = _mm256_loadu_si256((const __m256i *)(source + 32 * 2));
                if (N >= 4)
                    ymm3 = _mm256_loadu_si256((const __m256i *)(source + 32 * 3));
                if (N >= 5)
                    ymm4 = _mm256_loadu_si256((const __m256i *)(source + 32 * 4));
                if (N >= 6)
                    ymm5 = _mm256_loadu_si256((const __m256i *)(source + 32 * 5));
                if (N >= 7)
                    ymm6 = _mm256_loadu_si256((const __m256i *)(source + 32 * 6));
                // Use "{" and "}" to avoid the gcc warnings
                if (N >= 8) {
                    ymm7 = _mm256_loadu_si256((const __m256i *)(source + 32 * 7));
                }

                if (kUsePrefetchHint) {
                    // Here, N would be best a multiple of 2.
                    _mm_prefetch((const char *)(source + kPrefetchOffset + 64 * 0), kPrefetchHintLevel);
                    if (N >= 3)
                    _mm_prefetch((const char *)(source + kPrefetchOffset + 64 * 1), kPrefetchHintLevel);
                    if (N >= 5)
                    _mm_prefetch((const char *)(source + kPrefetchOffset + 64 * 2), kPrefetchHintLevel);
                    if (N >= 7)
                    _mm_prefetch((const char *)(source + kPrefetchOffset + 64 * 3), kPrefetchHintLevel);
                }

                source += kHalfLoopBytes;

                if (N >= 0)
                    _mm256_storeu_si256((__m256i *)(target + 32 * 0), ymm0);
                if (N >= 2)
                    _mm256_storeu_si256((__m256i *)(target + 32 * 1), ymm1);
                if (N >= 3)
                    _mm256_storeu_si256((__m256i *)(target + 32 * 2), ymm2);
                if (N >= 4)
                    _mm256_storeu_si256((__m256i *)(target + 32 * 3), ymm3);
                if (N >= 5)
                    _mm256_storeu_si256((__m256i *)(target + 32 * 4), ymm4);
                if (N >= 6)
                    _mm256_storeu_si256((__m256i *)(target + 32 * 5), ymm5);
                if (N >= 7)
                    _mm256_storeu_si256((__m256i *)(target + 32 * 6), ymm6);
                // Use "{" and "}" to avoid the gcc warnings
                if (N >= 8) {
                    _mm256_storeu_si256((__m256i *)(target + 32 * 7), ymm7);
                }

                target += kHalfLoopBytes;
            }

            avx_forward_move_N_tailing<T, kLoadIsNotAligned, kStoreIsNotAligned, (_N * 2 - 1)>(target, source, end);
        }
    }
}

template <typename T, std::size_t N = 8>
static
JSTD_NO_INLINE
void avx_forward_move_Nx2_store_aligned(T * __restrict first, T * __restrict mid, T * __restrict last)
{
    static const std::size_t kValueSize = sizeof(T);
    static const bool kValueSizeIsPower2 = ((kValueSize & (kValueSize - 1)) == 0);
    static const bool kValueSizeIsDivisible =  (kValueSize < kAVXRegBytes) ?
                                              ((kAVXRegBytes % kValueSize) == 0) :
                                              ((kValueSize % kAVXRegBytes) == 0);
    // minimum AVX regs = 1 * 2, maximum AVX regs = 8 * 2
    static const std::size_t _N = (N == 0) ? 1 : ((N <= 8) ? N : 8);
    static const std::size_t kHalfLoopBytes = _N * kAVXRegBytes;
    static const std::size_t kSingleLoopBytes = kHalfLoopBytes * 2;

    std::size_t unAlignedBytes = (std::size_t)first & kAVXAlignMask;
    bool storeAddrCanAlign;
    if (kValueSize < kAVXRegBytes)
        storeAddrCanAlign = (kValueSizeIsDivisible && ((unAlignedBytes % kValueSize) == 0));
    else
        storeAddrCanAlign = (kValueSizeIsDivisible && (unAlignedBytes == 0));

    if (likely(kValueSizeIsDivisible && storeAddrCanAlign)) {
        //unAlignedBytes = (unAlignedBytes != 0) ? (kAVXRegBytes - unAlignedBytes) : 0;
        unAlignedBytes = (kAVXRegBytes - unAlignedBytes) & kAVXAlignMask;
        while (unAlignedBytes != 0) {
            *first++ = *mid++;
            unAlignedBytes -= kValueSize;
        }

        char * __restrict target = (char * __restrict)first;
        char * __restrict source = (char * __restrict)mid;
        char * __restrict end = (char * __restrict)last;

        std::size_t lastUnalignedBytes = (std::size_t)last % kSingleLoopBytes;
        std::size_t totalBytes = (last - first) * kValueSize;
        const char * __restrict limit = (totalBytes >= kSingleLoopBytes) ? (end - lastUnalignedBytes) : source;

        bool loadAddrIsAligned = (((std::size_t)source & kAVXAlignMask) == 0);
        if (likely(!loadAddrIsAligned)) {
#if defined(__ICL)
#pragma code_align(64)
#endif
            while (source < limit) {
                __m256i ymm0, ymm1, ymm2, ymm3, ymm4, ymm5, ymm6, ymm7;
                if (N >= 0)
                    ymm0 = _mm256_loadu_si256((const __m256i *)(source + 32 * 0));
                if (N >= 2)
                    ymm1 = _mm256_loadu_si256((const __m256i *)(source + 32 * 1));
                if (N >= 3)
                    ymm2 = _mm256_loadu_si256((const __m256i *)(source + 32 * 2));
                if (N >= 4)
                    ymm3 = _mm256_loadu_si256((const __m256i *)(source + 32 * 3));
                if (N >= 5)
                    ymm4 = _mm256_loadu_si256((const __m256i *)(source + 32 * 4));
                if (N >= 6)
                    ymm5 = _mm256_loadu_si256((const __m256i *)(source + 32 * 5));
                if (N >= 7)
                    ymm6 = _mm256_loadu_si256((const __m256i *)(source + 32 * 6));
                // Use "{" and "}" to avoid the gcc warnings
                if (N >= 8) {
                    ymm7 = _mm256_loadu_si256((const __m256i *)(source + 32 * 7));
                }

                //
                // See: https://blog.csdn.net/qq_43401808/article/details/87360789
                //
                if (kUsePrefetchHint) {
                    // Here, N would be best a multiple of 2.
                    _mm_prefetch((const char *)(source + kPrefetchOffset + 64 * 0), kPrefetchHintLevel);
                    if (N >= 3)
                    _mm_prefetch((const char *)(source + kPrefetchOffset + 64 * 1), kPrefetchHintLevel);
                    if (N >= 5)
                    _mm_prefetch((const char *)(source + kPrefetchOffset + 64 * 2), kPrefetchHintLevel);
                    if (N >= 7)
                    _mm_prefetch((const char *)(source + kPrefetchOffset + 64 * 3), kPrefetchHintLevel);
                }

                source += kHalfLoopBytes;

                if (N >= 0)
                    _mm256_store_si256((__m256i *)(target + 32 * 0), ymm0);
                if (N >= 2)
                    _mm256_store_si256((__m256i *)(target + 32 * 1), ymm1);
                if (N >= 3)
                    _mm256_store_si256((__m256i *)(target + 32 * 2), ymm2);
                if (N >= 4)
                    _mm256_store_si256((__m256i *)(target + 32 * 3), ymm3);
                if (N >= 5)
                    _mm256_store_si256((__m256i *)(target + 32 * 4), ymm4);
                if (N >= 6)
                    _mm256_store_si256((__m256i *)(target + 32 * 5), ymm5);
                if (N >= 7)
                    _mm256_store_si256((__m256i *)(target + 32 * 6), ymm6);
                // Use "{" and "}" to avoid the gcc warnings
                if (N >= 8) {
                    _mm256_store_si256((__m256i *)(target + 32 * 7), ymm7);
                }

                target += kHalfLoopBytes;

                /////////////////////////////// Half loop //////////////////////////////////

                if (N >= 0)
                    ymm0 = _mm256_loadu_si256((const __m256i *)(source + 32 * 0));
                if (N >= 2)
                    ymm1 = _mm256_loadu_si256((const __m256i *)(source + 32 * 1));
                if (N >= 3)
                    ymm2 = _mm256_loadu_si256((const __m256i *)(source + 32 * 2));
                if (N >= 4)
                    ymm3 = _mm256_loadu_si256((const __m256i *)(source + 32 * 3));
                if (N >= 5)
                    ymm4 = _mm256_loadu_si256((const __m256i *)(source + 32 * 4));
                if (N >= 6)
                    ymm5 = _mm256_loadu_si256((const __m256i *)(source + 32 * 5));
                if (N >= 7)
                    ymm6 = _mm256_loadu_si256((const __m256i *)(source + 32 * 6));
                // Use "{" and "}" to avoid the gcc warnings
                if (N >= 8) {
                    ymm7 = _mm256_loadu_si256((const __m256i *)(source + 32 * 7));
                }

                if (kUsePrefetchHint) {
                    // Here, N would be best a multiple of 2.
                    _mm_prefetch((const char *)(source + kPrefetchOffset + 64 * 0), kPrefetchHintLevel);
                    if (N >= 3)
                    _mm_prefetch((const char *)(source + kPrefetchOffset + 64 * 1), kPrefetchHintLevel);
                    if (N >= 5)
                    _mm_prefetch((const char *)(source + kPrefetchOffset + 64 * 2), kPrefetchHintLevel);
                    if (N >= 7)
                    _mm_prefetch((const char *)(source + kPrefetchOffset + 64 * 3), kPrefetchHintLevel);
                }

                source += kHalfLoopBytes;

                if (N >= 0)
                    _mm256_store_si256((__m256i *)(target + 32 * 0), ymm0);
                if (N >= 2)
                    _mm256_store_si256((__m256i *)(target + 32 * 1), ymm1);
                if (N >= 3)
                    _mm256_store_si256((__m256i *)(target + 32 * 2), ymm2);
                if (N >= 4)
                    _mm256_store_si256((__m256i *)(target + 32 * 3), ymm3);
                if (N >= 5)
                    _mm256_store_si256((__m256i *)(target + 32 * 4), ymm4);
                if (N >= 6)
                    _mm256_store_si256((__m256i *)(target + 32 * 5), ymm5);
                if (N >= 7)
                    _mm256_store_si256((__m256i *)(target + 32 * 6), ymm6);
                // Use "{" and "}" to avoid the gcc warnings
                if (N >= 8) {
                    _mm256_store_si256((__m256i *)(target + 32 * 7), ymm7);
                }

                target += kHalfLoopBytes;
            }

            avx_forward_move_N_tailing<T, kLoadIsNotAligned, kStoreIsAligned, (_N * 2 - 1)>(target, source, end);
            //////////////////////////////////////////////////////////////////////////////////////////////////////
        } else {
#if defined(__ICL)
#pragma code_align(64)
#endif
            while (source < limit) {
                __m256i ymm0, ymm1, ymm2, ymm3, ymm4, ymm5, ymm6, ymm7;
                if (N >= 0)
                    ymm0 = _mm256_load_si256((const __m256i *)(source + 32 * 0));
                if (N >= 2)
                    ymm1 = _mm256_load_si256((const __m256i *)(source + 32 * 1));
                if (N >= 3)
                    ymm2 = _mm256_load_si256((const __m256i *)(source + 32 * 2));
                if (N >= 4)
                    ymm3 = _mm256_load_si256((const __m256i *)(source + 32 * 3));
                if (N >= 5)
                    ymm4 = _mm256_load_si256((const __m256i *)(source + 32 * 4));
                if (N >= 6)
                    ymm5 = _mm256_load_si256((const __m256i *)(source + 32 * 5));
                if (N >= 7)
                    ymm6 = _mm256_load_si256((const __m256i *)(source + 32 * 6));
                // Use "{" and "}" to avoid the gcc warnings
                if (N >= 8) {
                    ymm7 = _mm256_load_si256((const __m256i *)(source + 32 * 7));
                }

                if (kUsePrefetchHint) {
                    // Here, N would be best a multiple of 2.
                    _mm_prefetch((const char *)(source + kPrefetchOffset + 64 * 0), kPrefetchHintLevel);
                    if (N >= 3)
                    _mm_prefetch((const char *)(source + kPrefetchOffset + 64 * 1), kPrefetchHintLevel);
                    if (N >= 5)
                    _mm_prefetch((const char *)(source + kPrefetchOffset + 64 * 2), kPrefetchHintLevel);
                    if (N >= 7)
                    _mm_prefetch((const char *)(source + kPrefetchOffset + 64 * 3), kPrefetchHintLevel);
                }

                source += kHalfLoopBytes;

                if (N >= 0)
                    _mm256_store_si256((__m256i *)(target + 32 * 0), ymm0);
                if (N >= 2)
                    _mm256_store_si256((__m256i *)(target + 32 * 1), ymm1);
                if (N >= 3)
                    _mm256_store_si256((__m256i *)(target + 32 * 2), ymm2);
                if (N >= 4)
                    _mm256_store_si256((__m256i *)(target + 32 * 3), ymm3);
                if (N >= 5)
                    _mm256_store_si256((__m256i *)(target + 32 * 4), ymm4);
                if (N >= 6)
                    _mm256_store_si256((__m256i *)(target + 32 * 5), ymm5);
                if (N >= 7)
                    _mm256_store_si256((__m256i *)(target + 32 * 6), ymm6);
                // Use "{" and "}" to avoid the gcc warnings
                if (N >= 8) {
                    _mm256_store_si256((__m256i *)(target + 32 * 7), ymm7);
                }

                target += kHalfLoopBytes;

                /////////////////////////////// Half loop //////////////////////////////////

                if (N >= 0)
                    ymm0 = _mm256_load_si256((const __m256i *)(source + 32 * 0));
                if (N >= 2)
                    ymm1 = _mm256_load_si256((const __m256i *)(source + 32 * 1));
                if (N >= 3)
                    ymm2 = _mm256_load_si256((const __m256i *)(source + 32 * 2));
                if (N >= 4)
                    ymm3 = _mm256_load_si256((const __m256i *)(source + 32 * 3));
                if (N >= 5)
                    ymm4 = _mm256_load_si256((const __m256i *)(source + 32 * 4));
                if (N >= 6)
                    ymm5 = _mm256_load_si256((const __m256i *)(source + 32 * 5));
                if (N >= 7)
                    ymm6 = _mm256_load_si256((const __m256i *)(source + 32 * 6));
                // Use "{" and "}" to avoid the gcc warnings
                if (N >= 8) {
                    ymm7 = _mm256_load_si256((const __m256i *)(source + 32 * 7));
                }

                if (kUsePrefetchHint) {
                    // Here, N would be best a multiple of 2.
                    _mm_prefetch((const char *)(source + kPrefetchOffset + 64 * 0), kPrefetchHintLevel);
                    if (N >= 3)
                    _mm_prefetch((const char *)(source + kPrefetchOffset + 64 * 1), kPrefetchHintLevel);
                    if (N >= 5)
                    _mm_prefetch((const char *)(source + kPrefetchOffset + 64 * 2), kPrefetchHintLevel);
                    if (N >= 7)
                    _mm_prefetch((const char *)(source + kPrefetchOffset + 64 * 3), kPrefetchHintLevel);
                }

                source += kHalfLoopBytes;

                if (N >= 0)
                    _mm256_store_si256((__m256i *)(target + 32 * 0), ymm0);
                if (N >= 2)
                    _mm256_store_si256((__m256i *)(target + 32 * 1), ymm1);
                if (N >= 3)
                    _mm256_store_si256((__m256i *)(target + 32 * 2), ymm2);
                if (N >= 4)
                    _mm256_store_si256((__m256i *)(target + 32 * 3), ymm3);
                if (N >= 5)
                    _mm256_store_si256((__m256i *)(target + 32 * 4), ymm4);
                if (N >= 6)
                    _mm256_store_si256((__m256i *)(target + 32 * 5), ymm5);
                if (N >= 7)
                    _mm256_store_si256((__m256i *)(target + 32 * 6), ymm6);
                // Use "{" and "}" to avoid the gcc warnings
                if (N >= 8) {
                    _mm256_store_si256((__m256i *)(target + 32 * 7), ymm7);
                }

                target += kHalfLoopBytes;
            }

            avx_forward_move_N_tailing<T, kLoadIsAligned, kStoreIsAligned, (_N * 2 - 1)>(target, source, end);
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

        char * __restrict target = (char * __restrict)first;
        char * __restrict source = (char * __restrict)mid;
        char * __restrict end = (char * __restrict)last;

        std::size_t lastUnalignedBytes = (std::size_t)last % kSingleLoopBytes;
        std::size_t totalBytes = (last - first) * kValueSize;
        const char * __restrict limit = (totalBytes >= kSingleLoopBytes) ? (end - lastUnalignedBytes) : source;

        if (likely(loadAddrCanAlign)) {
#if defined(__ICL)
#pragma code_align(64)
#endif
            while (source < limit) {
                __m256i ymm0, ymm1, ymm2, ymm3, ymm4, ymm5, ymm6, ymm7;
                if (N >= 0)
                    ymm0 = _mm256_load_si256((const __m256i *)(source + 32 * 0));
                if (N >= 2)
                    ymm1 = _mm256_load_si256((const __m256i *)(source + 32 * 1));
                if (N >= 3)
                    ymm2 = _mm256_load_si256((const __m256i *)(source + 32 * 2));
                if (N >= 4)
                    ymm3 = _mm256_load_si256((const __m256i *)(source + 32 * 3));
                if (N >= 5)
                    ymm4 = _mm256_load_si256((const __m256i *)(source + 32 * 4));
                if (N >= 6)
                    ymm5 = _mm256_load_si256((const __m256i *)(source + 32 * 5));
                if (N >= 7)
                    ymm6 = _mm256_load_si256((const __m256i *)(source + 32 * 6));
                // Use "{" and "}" to avoid the gcc warnings
                if (N >= 8) {
                    ymm7 = _mm256_load_si256((const __m256i *)(source + 32 * 7));
                }

                if (kUsePrefetchHint) {
                    // Here, N would be best a multiple of 2.
                    _mm_prefetch((const char *)(source + kPrefetchOffset + 64 * 0), kPrefetchHintLevel);
                    if (N >= 3)
                    _mm_prefetch((const char *)(source + kPrefetchOffset + 64 * 1), kPrefetchHintLevel);
                    if (N >= 5)
                    _mm_prefetch((const char *)(source + kPrefetchOffset + 64 * 2), kPrefetchHintLevel);
                    if (N >= 7)
                    _mm_prefetch((const char *)(source + kPrefetchOffset + 64 * 3), kPrefetchHintLevel);
                }

                source += kHalfLoopBytes;

                if (N >= 0)
                    _mm256_storeu_si256((__m256i *)(target + 32 * 0), ymm0);
                if (N >= 2)
                    _mm256_storeu_si256((__m256i *)(target + 32 * 1), ymm1);
                if (N >= 3)
                    _mm256_storeu_si256((__m256i *)(target + 32 * 2), ymm2);
                if (N >= 4)
                    _mm256_storeu_si256((__m256i *)(target + 32 * 3), ymm3);
                if (N >= 5)
                    _mm256_storeu_si256((__m256i *)(target + 32 * 4), ymm4);
                if (N >= 6)
                    _mm256_storeu_si256((__m256i *)(target + 32 * 5), ymm5);
                if (N >= 7)
                    _mm256_storeu_si256((__m256i *)(target + 32 * 6), ymm6);
                // Use "{" and "}" to avoid the gcc warnings
                if (N >= 8) {
                    _mm256_storeu_si256((__m256i *)(target + 32 * 7), ymm7);
                }

                target += kHalfLoopBytes;

                /////////////////////////////// Half loop //////////////////////////////////

                if (N >= 0)
                    ymm0 = _mm256_load_si256((const __m256i *)(source + 32 * 0));
                if (N >= 2)
                    ymm1 = _mm256_load_si256((const __m256i *)(source + 32 * 1));
                if (N >= 3)
                    ymm2 = _mm256_load_si256((const __m256i *)(source + 32 * 2));
                if (N >= 4)
                    ymm3 = _mm256_load_si256((const __m256i *)(source + 32 * 3));
                if (N >= 5)
                    ymm4 = _mm256_load_si256((const __m256i *)(source + 32 * 4));
                if (N >= 6)
                    ymm5 = _mm256_load_si256((const __m256i *)(source + 32 * 5));
                if (N >= 7)
                    ymm6 = _mm256_load_si256((const __m256i *)(source + 32 * 6));
                // Use "{" and "}" to avoid the gcc warnings
                if (N >= 8) {
                    ymm7 = _mm256_load_si256((const __m256i *)(source + 32 * 7));
                }

                if (kUsePrefetchHint) {
                    // Here, N would be best a multiple of 2.
                    _mm_prefetch((const char *)(source + kPrefetchOffset + 64 * 0), kPrefetchHintLevel);
                    if (N >= 3)
                    _mm_prefetch((const char *)(source + kPrefetchOffset + 64 * 1), kPrefetchHintLevel);
                    if (N >= 5)
                    _mm_prefetch((const char *)(source + kPrefetchOffset + 64 * 2), kPrefetchHintLevel);
                    if (N >= 7)
                    _mm_prefetch((const char *)(source + kPrefetchOffset + 64 * 3), kPrefetchHintLevel);
                }

                source += kHalfLoopBytes;

                if (N >= 0)
                    _mm256_storeu_si256((__m256i *)(target + 32 * 0), ymm0);
                if (N >= 2)
                    _mm256_storeu_si256((__m256i *)(target + 32 * 1), ymm1);
                if (N >= 3)
                    _mm256_storeu_si256((__m256i *)(target + 32 * 2), ymm2);
                if (N >= 4)
                    _mm256_storeu_si256((__m256i *)(target + 32 * 3), ymm3);
                if (N >= 5)
                    _mm256_storeu_si256((__m256i *)(target + 32 * 4), ymm4);
                if (N >= 6)
                    _mm256_storeu_si256((__m256i *)(target + 32 * 5), ymm5);
                if (N >= 7)
                    _mm256_storeu_si256((__m256i *)(target + 32 * 6), ymm6);
                // Use "{" and "}" to avoid the gcc warnings
                if (N >= 8) {
                    _mm256_storeu_si256((__m256i *)(target + 32 * 7), ymm7);
                }

                target += kHalfLoopBytes;
            }

            avx_forward_move_N_tailing<T, kLoadIsAligned, kStoreIsNotAligned, (_N * 2 - 1)>(target, source, end);
            ////////////////////////////////////////////////////////////////////////////////////////////////
        } else {
#if defined(__ICL)
#pragma code_align(64)
#endif
            while (source < limit) {
                __m256i ymm0, ymm1, ymm2, ymm3, ymm4, ymm5, ymm6, ymm7;
                if (N >= 0)
                    ymm0 = _mm256_loadu_si256((const __m256i *)(source + 32 * 0));
                if (N >= 2)
                    ymm1 = _mm256_loadu_si256((const __m256i *)(source + 32 * 1));
                if (N >= 3)
                    ymm2 = _mm256_loadu_si256((const __m256i *)(source + 32 * 2));
                if (N >= 4)
                    ymm3 = _mm256_loadu_si256((const __m256i *)(source + 32 * 3));
                if (N >= 5)
                    ymm4 = _mm256_loadu_si256((const __m256i *)(source + 32 * 4));
                if (N >= 6)
                    ymm5 = _mm256_loadu_si256((const __m256i *)(source + 32 * 5));
                if (N >= 7)
                    ymm6 = _mm256_loadu_si256((const __m256i *)(source + 32 * 6));
                // Use "{" and "}" to avoid the gcc warnings
                if (N >= 8) {
                    ymm7 = _mm256_loadu_si256((const __m256i *)(source + 32 * 7));
                }

                if (kUsePrefetchHint) {
                    // Here, N would be best a multiple of 2.
                    _mm_prefetch((const char *)(source + kPrefetchOffset + 64 * 0), kPrefetchHintLevel);
                    if (N >= 3)
                    _mm_prefetch((const char *)(source + kPrefetchOffset + 64 * 1), kPrefetchHintLevel);
                    if (N >= 5)
                    _mm_prefetch((const char *)(source + kPrefetchOffset + 64 * 2), kPrefetchHintLevel);
                    if (N >= 7)
                    _mm_prefetch((const char *)(source + kPrefetchOffset + 64 * 3), kPrefetchHintLevel);
                }

                source += kHalfLoopBytes;

                if (N >= 0)
                    _mm256_storeu_si256((__m256i *)(target + 32 * 0), ymm0);
                if (N >= 2)
                    _mm256_storeu_si256((__m256i *)(target + 32 * 1), ymm1);
                if (N >= 3)
                    _mm256_storeu_si256((__m256i *)(target + 32 * 2), ymm2);
                if (N >= 4)
                    _mm256_storeu_si256((__m256i *)(target + 32 * 3), ymm3);
                if (N >= 5)
                    _mm256_storeu_si256((__m256i *)(target + 32 * 4), ymm4);
                if (N >= 6)
                    _mm256_storeu_si256((__m256i *)(target + 32 * 5), ymm5);
                if (N >= 7)
                    _mm256_storeu_si256((__m256i *)(target + 32 * 6), ymm6);
                // Use "{" and "}" to avoid the gcc warnings
                if (N >= 8) {
                    _mm256_storeu_si256((__m256i *)(target + 32 * 7), ymm7);
                }

                target += kHalfLoopBytes;

                /////////////////////////////// Half loop //////////////////////////////////

                if (N >= 0)
                    ymm0 = _mm256_loadu_si256((const __m256i *)(source + 32 * 0));
                if (N >= 2)
                    ymm1 = _mm256_loadu_si256((const __m256i *)(source + 32 * 1));
                if (N >= 3)
                    ymm2 = _mm256_loadu_si256((const __m256i *)(source + 32 * 2));
                if (N >= 4)
                    ymm3 = _mm256_loadu_si256((const __m256i *)(source + 32 * 3));
                if (N >= 5)
                    ymm4 = _mm256_loadu_si256((const __m256i *)(source + 32 * 4));
                if (N >= 6)
                    ymm5 = _mm256_loadu_si256((const __m256i *)(source + 32 * 5));
                if (N >= 7)
                    ymm6 = _mm256_loadu_si256((const __m256i *)(source + 32 * 6));
                // Use "{" and "}" to avoid the gcc warnings
                if (N >= 8) {
                    ymm7 = _mm256_loadu_si256((const __m256i *)(source + 32 * 7));
                }

                if (kUsePrefetchHint) {
                    // Here, N would be best a multiple of 2.
                    _mm_prefetch((const char *)(source + kPrefetchOffset + 64 * 0), kPrefetchHintLevel);
                    if (N >= 3)
                    _mm_prefetch((const char *)(source + kPrefetchOffset + 64 * 1), kPrefetchHintLevel);
                    if (N >= 5)
                    _mm_prefetch((const char *)(source + kPrefetchOffset + 64 * 2), kPrefetchHintLevel);
                    if (N >= 7)
                    _mm_prefetch((const char *)(source + kPrefetchOffset + 64 * 3), kPrefetchHintLevel);
                }

                source += kHalfLoopBytes;

                if (N >= 0)
                    _mm256_storeu_si256((__m256i *)(target + 32 * 0), ymm0);
                if (N >= 2)
                    _mm256_storeu_si256((__m256i *)(target + 32 * 1), ymm1);
                if (N >= 3)
                    _mm256_storeu_si256((__m256i *)(target + 32 * 2), ymm2);
                if (N >= 4)
                    _mm256_storeu_si256((__m256i *)(target + 32 * 3), ymm3);
                if (N >= 5)
                    _mm256_storeu_si256((__m256i *)(target + 32 * 4), ymm4);
                if (N >= 6)
                    _mm256_storeu_si256((__m256i *)(target + 32 * 5), ymm5);
                if (N >= 7)
                    _mm256_storeu_si256((__m256i *)(target + 32 * 6), ymm6);
                // Use "{" and "}" to avoid the gcc warnings
                if (N >= 8) {
                    _mm256_storeu_si256((__m256i *)(target + 32 * 7), ymm7);
                }

                target += kHalfLoopBytes;
            }

            avx_forward_move_N_tailing<T, kLoadIsNotAligned, kStoreIsNotAligned, (_N * 2 - 1)>(target, source, end);
        }
    }
}

#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC pop_options
#endif

template <typename T, std::size_t index>
void _mm_storeu_last(__m128i * addr, __m128i src, std::size_t left_len)
{
    uint8_t * target = (uint8_t *)addr;
    std::intptr_t left_bytes = left_len * sizeof(T) - index * kSSERegBytes;
    assert(left_bytes > 0 && left_bytes <= kSSERegBytes);

#if defined(__AVX512BW__) && defined(__AVX512VL__)
    static const uint32_t kFullStoreMask = 0x0000FFFFul;
    __mmask16 store_mask = (__mmask16)(kFullStoreMask >> (16 - left_bytes));
    _mm_mask_storeu_epi8(addr, store_mask, src);
#else
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
#endif
}

template <typename T, std::size_t index>
void _mm256_storeu_last(__m256i * addr, __m256i src, std::size_t left_len)
{
    uint8_t * target = (uint8_t *)addr;
    std::intptr_t left_bytes = left_len * sizeof(T) - index * kAVXRegBytes;
    assert(left_bytes > 0 && left_bytes <= kAVXRegBytes);

#if defined(__AVX512BW__) && defined(__AVX512VL__)
    static const uint32_t kFullStoreMask = 0xFFFFFFFFul;
    __mmask32 store_mask = (__mmask32)(kFullStoreMask >> (32 - left_bytes));
    _mm256_mask_storeu_epi8(addr, store_mask, src);
#else
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
#endif
}

template <typename T, std::size_t N>
JSTD_FORCE_INLINE
void left_rotate_avx_N_regs(T * first, T * mid, T * last, std::size_t left_len)
{
    __m256i stash0, stash1, stash2, stash3, stash4, stash5;
    __m256i stash6, stash7, stash8, stash9, stash10, stash11;

    const __m256i * stash_start = (const __m256i *)first;
    if (N >= 1)
        stash0 = _mm256_loadu_si256(stash_start + 0);
    if (N >= 2)
        stash1 = _mm256_loadu_si256(stash_start + 1);
    if (N >= 3)
        stash2 = _mm256_loadu_si256(stash_start + 2);
    if (N >= 4)
        stash3 = _mm256_loadu_si256(stash_start + 3);
    if (N >= 5)
        stash4 = _mm256_loadu_si256(stash_start + 4);
    if (N >= 6)
        stash5 = _mm256_loadu_si256(stash_start + 5);
    if (N >= 7)
        stash6 = _mm256_loadu_si256(stash_start + 6);
    if (N >= 8)
        stash7 = _mm256_loadu_si256(stash_start + 7);
    if (N >= 9)
        stash8 = _mm256_loadu_si256(stash_start + 8);
    if (N >= 10)
        stash9 = _mm256_loadu_si256(stash_start + 9);
    if (N >= 11)
        stash10 = _mm256_loadu_si256(stash_start + 10);
    // Use "{" and "}" to avoid the gcc warnings
    if (N >= 12) {
        stash11 = _mm256_loadu_si256(stash_start + 11);
    }

    ////////////////////////////////////////////////////////////////////////

#if defined(__clang__)
  #if 0
    if (N <= 6)         // 1 -- 6,
        avx_forward_move_Nx2_load_aligned<T, 8>(first, mid, last);
    else if (N <= 8)    // 7, 8
        avx_forward_move_Nx2_load_aligned<T, 6>(first, mid, last);
    else                // 9, 10, 11, 12
        avx_forward_move_Nx2_load_aligned<T, 4>(first, mid, last);
  #else
    if (N <= 6)         // 1 -- 6,
        avx_forward_move_Nx2_store_aligned<T, 8>(first, mid, last);
    else if (N <= 8)    // 7, 8
        avx_forward_move_Nx2_store_aligned<T, 6>(first, mid, last);
    else                // 9, 10, 11, 12
        avx_forward_move_Nx2_store_aligned<T, 4>(first, mid, last);
  #endif
#else
  #if 0
    if (N <= 6)         // 1 -- 6,
        avx_forward_move_N_load_aligned<T, 8>(first, mid, last);
    else if (N <= 8)    // 7, 8
        avx_forward_move_N_load_aligned<T, 6>(first, mid, last);
    else                // 9, 10, 11, 12
        avx_forward_move_N_load_aligned<T, 4>(first, mid, last);
  #elif 0
    if (N <= 6)         // 1 -- 6,
        avx_forward_move_N_store_aligned<T, 8>(first, mid, last);
    else if (N <= 8)    // 7, 8
        avx_forward_move_N_store_aligned<T, 6>(first, mid, last);
    else                // 9, 10, 11, 12
        avx_forward_move_N_store_aligned<T, 4>(first, mid, last);
  #else
    if (N <= 6)         // 1 -- 6,
        avx_forward_move_N_store_aligned_nt<T, 8>(first, mid, last);
    else if (N <= 8)    // 7, 8
        avx_forward_move_N_store_aligned_nt<T, 6>(first, mid, last);
    else                // 9, 10, 11, 12
        avx_forward_move_N_store_aligned_nt<T, 4>(first, mid, last);
  #endif
#endif

    ////////////////////////////////////////////////////////////////////////

    __m256i * store_start = (__m256i *)(last - left_len);
    if (N == 1)
        _mm256_storeu_last<T, 0>(store_start + 0, stash0, left_len);
    if (N > 1)
        _mm256_storeu_si256(store_start + 0, stash0);
    if (N == 2)
        _mm256_storeu_last<T, 1>(store_start + 1, stash1, left_len);
    if (N > 2)
        _mm256_storeu_si256(store_start + 1, stash1);
    if (N == 3)
        _mm256_storeu_last<T, 2>(store_start + 2, stash2, left_len);
    if (N > 3)
        _mm256_storeu_si256(store_start + 2, stash2);
    if (N == 4)
        _mm256_storeu_last<T, 3>(store_start + 3, stash3, left_len);
    if (N > 4)
        _mm256_storeu_si256(store_start + 3, stash3);
    if (N == 5)
        _mm256_storeu_last<T, 4>(store_start + 4, stash4, left_len);
    if (N > 5)
        _mm256_storeu_si256(store_start + 4, stash4);
    if (N == 6)
        _mm256_storeu_last<T, 5>(store_start + 5, stash5, left_len);
    if (N > 6)
        _mm256_storeu_si256(store_start + 5, stash5);
    if (N == 7)
        _mm256_storeu_last<T, 6>(store_start + 6, stash6, left_len);
    if (N > 7)
        _mm256_storeu_si256(store_start + 6, stash6);
    if (N == 8)
        _mm256_storeu_last<T, 7>(store_start + 7, stash7, left_len);
    if (N > 8)
        _mm256_storeu_si256(store_start + 7, stash7);
    if (N == 9)
        _mm256_storeu_last<T, 8>(store_start + 8, stash8, left_len);
    if (N > 9)
        _mm256_storeu_si256(store_start + 8, stash8);
    if (N == 10)
        _mm256_storeu_last<T, 9>(store_start + 9, stash9, left_len);
    if (N > 10)
        _mm256_storeu_si256(store_start + 9, stash9);
    if (N == 11)
        _mm256_storeu_last<T, 10>(store_start + 10, stash10, left_len);
    if (N > 11)
        _mm256_storeu_si256(store_start + 10, stash10);
    if (N == 12)
        _mm256_storeu_last<T, 11>(store_start + 11, stash11, left_len);
}

template <typename T>
JSTD_FORCE_INLINE
void left_rotate_sse_1_regs(T * first, T * mid, T * last, std::size_t left_len)
{
    const __m128i * stash_start = (const __m128i *)first;
    __m128i stash0 = _mm_loadu_si128(stash_start);

    avx_forward_move_N_load_aligned<T, 8>(first, mid, last);

    __m128i * store_start = (__m128i *)(last - left_len);
    _mm_storeu_last<T, 0>(store_start, stash0, left_len);
}

template <typename T>
JSTD_FORCE_INLINE
void left_rotate_avx_1_regs(T * first, T * mid, T * last, std::size_t left_len)
{
    const __m256i * stash_start = (const __m256i *)first;
    __m256i stash0 = _mm256_loadu_si256(stash_start + 0);

    avx_forward_move_N_load_aligned<T, 8>(first, mid, last);

    __m256i * store_start = (__m256i *)(last - left_len);
    _mm256_storeu_last<T, 0>(store_start + 0, stash0, left_len);
}

template <typename T>
JSTD_FORCE_INLINE
void left_rotate_avx_2_regs(T * first, T * mid, T * last, std::size_t left_len)
{
    const __m256i * stash_start = (const __m256i *)first;
    __m256i stash0 = _mm256_loadu_si256(stash_start + 0);
    __m256i stash1 = _mm256_loadu_si256(stash_start + 1);

    avx_forward_move_N_load_aligned<T, 8>(first, mid, last);

    __m256i * store_start = (__m256i *)(last - left_len);
    _mm256_storeu_si256(store_start + 0, stash0);
    _mm256_storeu_last<T, 1>(store_start + 1, stash1, left_len);
}

template <typename T>
JSTD_FORCE_INLINE
void left_rotate_avx_3_regs(T * first, T * mid, T * last, std::size_t left_len)
{
    const __m256i * stash_start = (const __m256i *)first;
    __m256i stash0 = _mm256_loadu_si256(stash_start + 0);
    __m256i stash1 = _mm256_loadu_si256(stash_start + 1);
    __m256i stash2 = _mm256_loadu_si256(stash_start + 2);

    avx_forward_move_N_load_aligned<T, 8>(first, mid, last);

    __m256i * store_start = (__m256i *)(last - left_len);
    _mm256_storeu_si256(store_start + 0, stash0);
    _mm256_storeu_si256(store_start + 1, stash1);
    _mm256_storeu_last<T, 2>(store_start + 2, stash2, left_len);
}

template <typename T>
JSTD_FORCE_INLINE
void left_rotate_avx_4_regs(T * first, T * mid, T * last, std::size_t left_len)
{
    const __m256i * stash_start = (const __m256i *)first;
    __m256i stash0 = _mm256_loadu_si256(stash_start + 0);
    __m256i stash1 = _mm256_loadu_si256(stash_start + 1);
    __m256i stash2 = _mm256_loadu_si256(stash_start + 2);
    __m256i stash3 = _mm256_loadu_si256(stash_start + 3);

#if defined(__clang__)
    avx_forward_move_Nx2_load_aligned<T, 8>(first, mid, last);
    //avx_forward_move_Nx2_store_aligned<T, 8>(first, mid, last);
#else
    avx_forward_move_N_load_aligned<T, 8>(first, mid, last);
    //avx_forward_move_N_store_aligned<T, 8>(first, mid, last);
#endif

    __m256i * store_start = (__m256i *)(last - left_len);
    _mm256_storeu_si256(store_start + 0, stash0);
    _mm256_storeu_si256(store_start + 1, stash1);
    _mm256_storeu_si256(store_start + 2, stash2);
    _mm256_storeu_last<T, 3>(store_start + 3, stash3, left_len);
}

template <typename T>
JSTD_FORCE_INLINE
void left_rotate_avx_5_regs(T * first, T * mid, T * last, std::size_t left_len)
{
    const __m256i * stash_start = (const __m256i *)first;
    __m256i stash0 = _mm256_loadu_si256(stash_start + 0);
    __m256i stash1 = _mm256_loadu_si256(stash_start + 1);
    __m256i stash2 = _mm256_loadu_si256(stash_start + 2);
    __m256i stash3 = _mm256_loadu_si256(stash_start + 3);
    __m256i stash4 = _mm256_loadu_si256(stash_start + 4);

#if defined(__clang__)
    avx_forward_move_Nx2_load_aligned<T, 8>(first, mid, last);
    //avx_forward_move_Nx2_store_aligned<T, 8>(first, mid, last);
#else
    avx_forward_move_N_load_aligned<T, 8>(first, mid, last);
    //avx_forward_move_N_store_aligned<T, 8>(first, mid, last);
#endif

    __m256i * store_start = (__m256i *)(last - left_len);
    _mm256_storeu_si256(store_start + 0, stash0);
    _mm256_storeu_si256(store_start + 1, stash1);
    _mm256_storeu_si256(store_start + 2, stash2);
    _mm256_storeu_si256(store_start + 3, stash3);
    _mm256_storeu_last<T, 4>(store_start + 4, stash4, left_len);
}

template <typename T>
JSTD_FORCE_INLINE
void left_rotate_avx_6_regs(T * first, T * mid, T * last, std::size_t left_len)
{
    const __m256i * stash_start = (const __m256i *)first;
    __m256i stash0 = _mm256_loadu_si256(stash_start + 0);
    __m256i stash1 = _mm256_loadu_si256(stash_start + 1);
    __m256i stash2 = _mm256_loadu_si256(stash_start + 2);
    __m256i stash3 = _mm256_loadu_si256(stash_start + 3);
    __m256i stash4 = _mm256_loadu_si256(stash_start + 4);
    __m256i stash5 = _mm256_loadu_si256(stash_start + 5);

    avx_forward_move_N_load_aligned<T, 8>(first, mid, last);

    __m256i * store_start = (__m256i *)(last - left_len);
    _mm256_storeu_si256(store_start + 0, stash0);
    _mm256_storeu_si256(store_start + 1, stash1);
    _mm256_storeu_si256(store_start + 2, stash2);
    _mm256_storeu_si256(store_start + 3, stash3);
    _mm256_storeu_si256(store_start + 4, stash4);
    _mm256_storeu_last<T, 5>(store_start + 5, stash5, left_len);
}

template <typename T>
JSTD_FORCE_INLINE
void left_rotate_avx_7_regs(T * first, T * mid, T * last, std::size_t left_len)
{
    const __m256i * stash_start = (const __m256i *)first;
    __m256i stash0 = _mm256_loadu_si256(stash_start + 0);
    __m256i stash1 = _mm256_loadu_si256(stash_start + 1);
    __m256i stash2 = _mm256_loadu_si256(stash_start + 2);
    __m256i stash3 = _mm256_loadu_si256(stash_start + 3);
    __m256i stash4 = _mm256_loadu_si256(stash_start + 4);
    __m256i stash5 = _mm256_loadu_si256(stash_start + 5);
    __m256i stash6 = _mm256_loadu_si256(stash_start + 6);

    avx_forward_move_N_load_aligned<T, 6>(first, mid, last);

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
JSTD_FORCE_INLINE
void left_rotate_avx_8_regs(T * first, T * mid, T * last, std::size_t left_len)
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

    avx_forward_move_N_load_aligned<T, 6>(first, mid, last);

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
JSTD_FORCE_INLINE
void left_rotate_avx_9_regs(T * first, T * mid, T * last, std::size_t left_len)
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

    avx_forward_move_N_load_aligned<T, 4>(first, mid, last);

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
JSTD_FORCE_INLINE
void left_rotate_avx_10_regs(T * first, T * mid, T * last, std::size_t left_len)
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

    avx_forward_move_N_load_aligned<T, 4>(first, mid, last);

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
JSTD_FORCE_INLINE
void left_rotate_avx_11_regs(T * first, T * mid, T * last, std::size_t left_len)
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

    avx_forward_move_N_load_aligned<T, 4>(first, mid, last);

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
JSTD_FORCE_INLINE
void left_rotate_avx_12_regs(T * first, T * mid, T * last, std::size_t left_len)
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
    __m256i stash11 = _mm256_loadu_si256(stash_start + 11);

    avx_forward_move_N_load_aligned<T, 4>(first, mid, last);

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
    _mm256_storeu_si256(store_start + 10, stash10);
    _mm256_storeu_last<T, 11>(store_start + 11, stash11, left_len);
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

    JSTD_ASSERT((offset <= length), "left_rotate_avx(): Error, offset > length.");
    std::size_t right_len = length - offset;
    if (right_len == 0) return last;

    if (length * sizeof(T) <= kAVXRotateThresholdBytes) {
        return left_rotate_simple_impl(first, mid, last, left_len, right_len);
    }

    pointer result = first + right_len;

    if (left_len <= right_len) {
        std::size_t left_bytes = left_len * sizeof(T);
        if (left_bytes <= kMaxAVXStashBytes) {
            std::size_t avx_needs = (left_bytes - 1) / kAVXRegBytes;
            switch (avx_needs) {
                case 0:
                    if (left_bytes <= kSSERegBytes)
                        left_rotate_sse_1_regs(first, mid, last, left_len);
                    else
                        left_rotate_avx_N_regs<T, 1>(first, mid, last, left_len);
                    break;
                case 1:
                    left_rotate_avx_N_regs<T, 2>(first, mid, last, left_len);
                    break;
                case 2:
                    left_rotate_avx_N_regs<T, 3>(first, mid, last, left_len);
                    break;
                case 3:
                    left_rotate_avx_N_regs<T, 4>(first, mid, last, left_len);
                    break;
                case 4:
                    left_rotate_avx_N_regs<T, 5>(first, mid, last, left_len);
                    break;
                case 5:
                    left_rotate_avx_N_regs<T, 6>(first, mid, last, left_len);
                    break;
                case 6:
                    left_rotate_avx_N_regs<T, 7>(first, mid, last, left_len);
                    break;
                case 7:
                    left_rotate_avx_N_regs<T, 8>(first, mid, last, left_len);
                    break;
                case 8:
                    left_rotate_avx_N_regs<T, 9>(first, mid, last, left_len);
                    break;
                case 9:
                    left_rotate_avx_N_regs<T, 10>(first, mid, last, left_len);
                    break;
                case 10:
                    left_rotate_avx_N_regs<T, 11>(first, mid, last, left_len);
                    break;
                case 11:
                    left_rotate_avx_N_regs<T, 12>(first, mid, last, left_len);
                    break;
                default:
                    assert(false);
                    break;
            }
        }
        else {
            return left_rotate_simple_impl(first, mid, last, left_len, right_len);
        }
    } else {
        std::size_t right_bytes = right_len * sizeof(T);
        if (right_bytes <= kMaxAVXStashBytes) {
            return left_rotate_simple_impl(first, mid, last, left_len, right_len);
        }
        else {
            return left_rotate_simple_impl(first, mid, last, left_len, right_len);
        }
    }

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
    JSTD_ASSERT((last >= mid),  "simd::rotate(): (last < mid)");
    JSTD_ASSERT((mid >= first), "simd::rotate(): (mid < first)");
    return left_rotate_avx((T *)first, std::size_t(last - first), std::size_t(mid - first));
}

} // namespace simd
} // namespace jstd

#undef PREFETCH_HINT_LEVEL

#endif // JSTD_ARRAY_ROTATE_SIMD_H
