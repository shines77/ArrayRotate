
#ifndef JSTD_ARRAY_ROTATE_SIMD_H
#define JSTD_ARRAY_ROTATE_SIMD_H

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

#include <assert.h>
#include <cstdint>
#include <cstddef>
#include <cstdbool>
#include <atomic>
#include <algorithm>
#include <type_traits>

#include "jstd/stddef.h"
#include "jstd/BitVec.h"

#define USE_COMPILER_BARRIER    1

#if USE_COMPILER_BARRIER
#ifndef jstd_compiler_barrier
#define jstd_compiler_barrier()     std::atomic_signal_fence(std::memory_order_release);
#endif
#else
#define jstd_compiler_barrier()     ((void)0)
#endif // USE_COMPILER_BARRIER

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
static const std::size_t kPrefetchOffset = 256;

#if defined(JSTD_IS_PURE_GCC)
static const enum _mm_hint kPrefetchHintLevel = PREFETCH_HINT_LEVEL;
#else
static const int kPrefetchHintLevel = PREFETCH_HINT_LEVEL;
#endif

///////////////////////////////////////////////

// Actual cache line size (bytes)
static const std::size_t kCacheLineSize = 64;

// Maximum cache line size (bytes),
// for future CPU, it can adjust to 128 bytes.
// However, if the setting is larger than
// the actual cache line size, it will cause waste.
static const std::size_t kMaxCacheLineSize = 64;

///////////////////////////////////////////////

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

///////////////////////////////////////////////

// The Enum of aligned property
static const bool kIsNotAligned = false;
static const bool kIsAligned = true;

static const int kUnknownAligned = 2;

///////////////////////////////////////////////

static const bool kSrcIsAligned = true;
static const bool kDestIsAligned = true;

static const bool kSrcIsNotAligned = false;
static const bool kDestIsNotAligned = false;

///////////////////////////////////////////////

template <typename T>
T * right_rotate_simple_impl(T * first, T * mid, T * last,
                             std::size_t left_len, std::size_t right_len)
{
    typedef T   value_type;
    typedef T * pointer;

    pointer result = first + right_len;

    do {
        if (right_len <= left_len) {
            pointer read = mid;
            pointer write = last;
            if (right_len != 1) {
                while (read != first) {
                    --read;
                    --write;
                    std::iter_swap(read, write);
                }

                left_len %= right_len;
                last = write;
                right_len -= left_len;
                mid = first + left_len;
                if (left_len == 0 || right_len == 0)
                    break;
            }
            else {
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
        else {
            pointer read = mid;
            pointer write = first;
            if (left_len != 1) {
                while (read != last) {
                    std::iter_swap(write, read);
                    ++write;
                    ++read;
                }

                right_len %= left_len;
                first = write;
                left_len -= right_len;
                mid = last - right_len;
                if (right_len == 0 || left_len == 0)
                    break;
            }
            else {
                value_type tmp(std::move(*write));
                while (read != last) {
                    *write = std::move(*read);
                    ++write;
                    ++read;
                }
                *write = std::move(tmp);
                break;
            }
        }
    } while (1);

    return result;
}

template <typename T>
inline
T * right_rotate_simple(T * first, T * mid, T * last)
{
    // If (first > mid), it's a error under DEBUG mode.
    JSTD_ASSERT_EX((first <= mid), "simd::right_rotate_simple(): Error, first > mid.");

    std::ptrdiff_t s_left_len = mid - first;
    if (first > mid) return first;

    // If (mid > last), it's a error under DEBUG mode.
    JSTD_ASSERT_EX((mid <= last), "simd::right_rotate_simple(): Error, mid > last.");

    std::ptrdiff_t s_right_len = last - mid;
    if (mid > last) return last;

    std::size_t left_len = std::size_t(s_left_len);
    std::size_t right_len = std::size_t(s_right_len);

    return right_rotate_simple_impl(first, mid, last, left_len, right_len);
}

template <typename T>
inline
T * right_rotate_simple(T * data, std::size_t length, std::size_t offset)
{
    typedef T * pointer;

    pointer first = data;
    pointer last  = data + length;
    pointer mid   = last - offset;

    // If (offset > length), it's a error under DEBUG mode.
    JSTD_ASSERT_EX((offset <= length), "simd::right_rotate_simple(): Error, offset > length.");

    // If offset is bigger than length, directly return last.
    std::intptr_t s_left_len = length - offset;
    if (offset >= length) return first;

    std::size_t left_len = (std::size_t)(s_left_len);

    std::size_t right_len = offset;
    if (right_len == 0) return last;   

    return right_rotate_simple_impl(first, mid, last, left_len, right_len);
}

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
inline
T * left_rotate_simple(T * first, T * mid, T * last)
{
    // If (first > mid), it's a error under DEBUG mode.
    JSTD_ASSERT_EX((first <= mid), "simd::left_rotate_simple(): Error, first > mid.");

    std::ptrdiff_t s_left_len = mid - first;
    if (first >= mid) return first;

    // If (mid > last), it's a error under DEBUG mode.
    JSTD_ASSERT_EX((mid <= last), "simd::left_rotate_simple(): Error, mid > last.");

    std::ptrdiff_t s_right_len = last - mid;
    if (mid >= last) return last;

    std::size_t left_len = std::size_t(s_left_len);
    std::size_t right_len = std::size_t(s_right_len);

    return left_rotate_simple_impl(first, mid, last, left_len, right_len);
}

template <typename T>
inline
T * left_rotate_simple(T * data, std::size_t length, std::size_t offset)
{
    typedef T * pointer;

    pointer first = data;
    pointer mid   = data + offset;
    pointer last  = data + length;

    std::size_t left_len = offset;
    if (left_len == 0) return first;

    // If (offset > length), it's a error under DEBUG mode.
    JSTD_ASSERT_EX((offset <= length), "simd::left_rotate_simple(): Error, offset > length.");

    // If offset is bigger than length, directly return last.
    std::intptr_t s_right_len = length - offset;
    if (offset >= length) return last;

    std::size_t right_len = (std::size_t)(s_right_len);

    return left_rotate_simple_impl(first, mid, last, left_len, right_len);
}

template <typename T>
inline
T * rotate_simple(T * first, T * mid, T * last)
{
    return left_rotate_simple(first, mid, last);
}

template <typename T>
inline
T * rotate_simple(T * data, std::size_t length, std::size_t offset)
{
    return left_rotate_simple(data, length, offset);
}

template <typename T, bool srcIsAligned, bool destIsAligned, int LeftUints = 7>
static
JSTD_NO_INLINE
void avx_forward_move_N_tailing(char * JSTD_RESTRICT dest, char * JSTD_RESTRICT src, char * JSTD_RESTRICT end)
{
    static const std::size_t kValueSize = sizeof(T);
    JSTD_ASSERT(end >= src);
    std::size_t left_bytes = (std::size_t)(end - src);
    JSTD_ASSERT((left_bytes % kValueSize) == 0);

    if (srcIsAligned && destIsAligned) {
        if (((src + (8 * kAVXRegBytes)) <= end) && (LeftUints >= 8)) {
            __m256i ymm0 = _mm256_load_si256((const __m256i *)(src + 32 * 0));
            __m256i ymm1 = _mm256_load_si256((const __m256i *)(src + 32 * 1));
            __m256i ymm2 = _mm256_load_si256((const __m256i *)(src + 32 * 2));
            __m256i ymm3 = _mm256_load_si256((const __m256i *)(src + 32 * 3));
            __m256i ymm4 = _mm256_load_si256((const __m256i *)(src + 32 * 4));
            __m256i ymm5 = _mm256_load_si256((const __m256i *)(src + 32 * 5));
            __m256i ymm6 = _mm256_load_si256((const __m256i *)(src + 32 * 6));
            __m256i ymm7 = _mm256_load_si256((const __m256i *)(src + 32 * 7));

            src += 8 * kAVXRegBytes;

            _mm256_store_si256((__m256i *)(dest + 32 * 0), ymm0);
            _mm256_store_si256((__m256i *)(dest + 32 * 1), ymm1);
            _mm256_store_si256((__m256i *)(dest + 32 * 2), ymm2);
            _mm256_store_si256((__m256i *)(dest + 32 * 3), ymm3);
            _mm256_store_si256((__m256i *)(dest + 32 * 4), ymm4);
            _mm256_store_si256((__m256i *)(dest + 32 * 5), ymm5);
            _mm256_store_si256((__m256i *)(dest + 32 * 6), ymm6);
            _mm256_store_si256((__m256i *)(dest + 32 * 7), ymm7);

            dest += 8 * kAVXRegBytes;
        }

        if (((src + (4 * kAVXRegBytes)) <= end) && (LeftUints >= 4)) {
            __m256i ymm0 = _mm256_load_si256((const __m256i *)(src + 32 * 0));
            __m256i ymm1 = _mm256_load_si256((const __m256i *)(src + 32 * 1));
            __m256i ymm2 = _mm256_load_si256((const __m256i *)(src + 32 * 2));
            __m256i ymm3 = _mm256_load_si256((const __m256i *)(src + 32 * 3));

            src += 4 * kAVXRegBytes;

            _mm256_store_si256((__m256i *)(dest + 32 * 0), ymm0);
            _mm256_store_si256((__m256i *)(dest + 32 * 1), ymm1);
            _mm256_store_si256((__m256i *)(dest + 32 * 2), ymm2);
            _mm256_store_si256((__m256i *)(dest + 32 * 3), ymm3);

            dest += 4 * kAVXRegBytes;
        }

        if (((src + (2 * kAVXRegBytes)) <= end) && (LeftUints >= 2)) {
            __m256i ymm0 = _mm256_load_si256((const __m256i *)(src + 32 * 0));
            __m256i ymm1 = _mm256_load_si256((const __m256i *)(src + 32 * 1));

            src += 2 * kAVXRegBytes;

            _mm256_store_si256((__m256i *)(dest + 32 * 0), ymm0);
            _mm256_store_si256((__m256i *)(dest + 32 * 1), ymm1);

            dest += 2 * kAVXRegBytes;
        }

        if (((src + (1 * kAVXRegBytes)) <= end) && (LeftUints >= 1)) {
            __m256i ymm0 = _mm256_load_si256((const __m256i *)(src + 32 * 0));

            src += 1 * kAVXRegBytes;

            _mm256_store_si256((__m256i *)(dest + 32 * 0), ymm0);

            dest += 1 * kAVXRegBytes;
        }
    }
    else if (srcIsAligned && !destIsAligned) {
        if (((src + (8 * kAVXRegBytes)) <= end) && (LeftUints >= 8)) {
            __m256i ymm0 = _mm256_load_si256((const __m256i *)(src + 32 * 0));
            __m256i ymm1 = _mm256_load_si256((const __m256i *)(src + 32 * 1));
            __m256i ymm2 = _mm256_load_si256((const __m256i *)(src + 32 * 2));
            __m256i ymm3 = _mm256_load_si256((const __m256i *)(src + 32 * 3));
            __m256i ymm4 = _mm256_load_si256((const __m256i *)(src + 32 * 4));
            __m256i ymm5 = _mm256_load_si256((const __m256i *)(src + 32 * 5));
            __m256i ymm6 = _mm256_load_si256((const __m256i *)(src + 32 * 6));
            __m256i ymm7 = _mm256_load_si256((const __m256i *)(src + 32 * 7));

            src += 8 * kAVXRegBytes;

            _mm256_storeu_si256((__m256i *)(dest + 32 * 0), ymm0);
            _mm256_storeu_si256((__m256i *)(dest + 32 * 1), ymm1);
            _mm256_storeu_si256((__m256i *)(dest + 32 * 2), ymm2);
            _mm256_storeu_si256((__m256i *)(dest + 32 * 3), ymm3);
            _mm256_storeu_si256((__m256i *)(dest + 32 * 4), ymm4);
            _mm256_storeu_si256((__m256i *)(dest + 32 * 5), ymm5);
            _mm256_storeu_si256((__m256i *)(dest + 32 * 6), ymm6);
            _mm256_storeu_si256((__m256i *)(dest + 32 * 7), ymm7);

            dest += 8 * kAVXRegBytes;
        }

        if (((src + (4 * kAVXRegBytes)) <= end) && (LeftUints >= 4)) {
            __m256i ymm0 = _mm256_load_si256((const __m256i *)(src + 32 * 0));
            __m256i ymm1 = _mm256_load_si256((const __m256i *)(src + 32 * 1));
            __m256i ymm2 = _mm256_load_si256((const __m256i *)(src + 32 * 2));
            __m256i ymm3 = _mm256_load_si256((const __m256i *)(src + 32 * 3));

            src += 4 * kAVXRegBytes;

            _mm256_storeu_si256((__m256i *)(dest + 32 * 0), ymm0);
            _mm256_storeu_si256((__m256i *)(dest + 32 * 1), ymm1);
            _mm256_storeu_si256((__m256i *)(dest + 32 * 2), ymm2);
            _mm256_storeu_si256((__m256i *)(dest + 32 * 3), ymm3);

            dest += 4 * kAVXRegBytes;
        }

        if (((src + (2 * kAVXRegBytes)) <= end) && (LeftUints >= 2)) {
            __m256i ymm0 = _mm256_load_si256((const __m256i *)(src + 32 * 0));
            __m256i ymm1 = _mm256_load_si256((const __m256i *)(src + 32 * 1));

            src += 2 * kAVXRegBytes;

            _mm256_storeu_si256((__m256i *)(dest + 32 * 0), ymm0);
            _mm256_storeu_si256((__m256i *)(dest + 32 * 1), ymm1);

            dest += 2 * kAVXRegBytes;
        }

        if (((src + (1 * kAVXRegBytes)) <= end) && (LeftUints >= 1)) {
            __m256i ymm0 = _mm256_load_si256((const __m256i *)(src + 32 * 0));

            src += 1 * kAVXRegBytes;

            _mm256_storeu_si256((__m256i *)(dest + 32 * 0), ymm0);

            dest += 1 * kAVXRegBytes;
        }
    }
    else if (!srcIsAligned && destIsAligned) {
        if (((src + (8 * kAVXRegBytes)) <= end) && (LeftUints >= 8)) {
            __m256i ymm0 = _mm256_loadu_si256((const __m256i *)(src + 32 * 0));
            __m256i ymm1 = _mm256_loadu_si256((const __m256i *)(src + 32 * 1));
            __m256i ymm2 = _mm256_loadu_si256((const __m256i *)(src + 32 * 2));
            __m256i ymm3 = _mm256_loadu_si256((const __m256i *)(src + 32 * 3));
            __m256i ymm4 = _mm256_loadu_si256((const __m256i *)(src + 32 * 4));
            __m256i ymm5 = _mm256_loadu_si256((const __m256i *)(src + 32 * 5));
            __m256i ymm6 = _mm256_loadu_si256((const __m256i *)(src + 32 * 6));
            __m256i ymm7 = _mm256_loadu_si256((const __m256i *)(src + 32 * 7));

            src += 8 * kAVXRegBytes;

            _mm256_store_si256((__m256i *)(dest + 32 * 0), ymm0);
            _mm256_store_si256((__m256i *)(dest + 32 * 1), ymm1);
            _mm256_store_si256((__m256i *)(dest + 32 * 2), ymm2);
            _mm256_store_si256((__m256i *)(dest + 32 * 3), ymm3);
            _mm256_store_si256((__m256i *)(dest + 32 * 4), ymm4);
            _mm256_store_si256((__m256i *)(dest + 32 * 5), ymm5);
            _mm256_store_si256((__m256i *)(dest + 32 * 6), ymm6);
            _mm256_store_si256((__m256i *)(dest + 32 * 7), ymm7);

            dest += 8 * kAVXRegBytes;
        }

        if (((src + (4 * kAVXRegBytes)) <= end) && (LeftUints >= 4)) {
            __m256i ymm0 = _mm256_loadu_si256((const __m256i *)(src + 32 * 0));
            __m256i ymm1 = _mm256_loadu_si256((const __m256i *)(src + 32 * 1));
            __m256i ymm2 = _mm256_loadu_si256((const __m256i *)(src + 32 * 2));
            __m256i ymm3 = _mm256_loadu_si256((const __m256i *)(src + 32 * 3));

            src += 4 * kAVXRegBytes;

            _mm256_store_si256((__m256i *)(dest + 32 * 0), ymm0);
            _mm256_store_si256((__m256i *)(dest + 32 * 1), ymm1);
            _mm256_store_si256((__m256i *)(dest + 32 * 2), ymm2);
            _mm256_store_si256((__m256i *)(dest + 32 * 3), ymm3);

            dest += 4 * kAVXRegBytes;
        }

        if (((src + (2 * kAVXRegBytes)) <= end) && (LeftUints >= 2)) {
            __m256i ymm0 = _mm256_loadu_si256((const __m256i *)(src + 32 * 0));
            __m256i ymm1 = _mm256_loadu_si256((const __m256i *)(src + 32 * 1));

            src += 2 * kAVXRegBytes;

            _mm256_store_si256((__m256i *)(dest + 32 * 0), ymm0);
            _mm256_store_si256((__m256i *)(dest + 32 * 1), ymm1);

            dest += 2 * kAVXRegBytes;
        }

        if (((src + (1 * kAVXRegBytes)) <= end) && (LeftUints >= 1)) {
            __m256i ymm0 = _mm256_loadu_si256((const __m256i *)(src + 32 * 0));

            src += 1 * kAVXRegBytes;

            _mm256_store_si256((__m256i *)(dest + 32 * 0), ymm0);

            dest += 1 * kAVXRegBytes;
        }
    }
    else {
        if (((src + (8 * kAVXRegBytes)) <= end) && (LeftUints >= 8)) {
            __m256i ymm0 = _mm256_loadu_si256((const __m256i *)(src + 32 * 0));
            __m256i ymm1 = _mm256_loadu_si256((const __m256i *)(src + 32 * 1));
            __m256i ymm2 = _mm256_loadu_si256((const __m256i *)(src + 32 * 2));
            __m256i ymm3 = _mm256_loadu_si256((const __m256i *)(src + 32 * 3));
            __m256i ymm4 = _mm256_loadu_si256((const __m256i *)(src + 32 * 4));
            __m256i ymm5 = _mm256_loadu_si256((const __m256i *)(src + 32 * 5));
            __m256i ymm6 = _mm256_loadu_si256((const __m256i *)(src + 32 * 6));
            __m256i ymm7 = _mm256_loadu_si256((const __m256i *)(src + 32 * 7));

            src += 8 * kAVXRegBytes;

            _mm256_storeu_si256((__m256i *)(dest + 32 * 0), ymm0);
            _mm256_storeu_si256((__m256i *)(dest + 32 * 1), ymm1);
            _mm256_storeu_si256((__m256i *)(dest + 32 * 2), ymm2);
            _mm256_storeu_si256((__m256i *)(dest + 32 * 3), ymm3);
            _mm256_storeu_si256((__m256i *)(dest + 32 * 4), ymm4);
            _mm256_storeu_si256((__m256i *)(dest + 32 * 5), ymm5);
            _mm256_storeu_si256((__m256i *)(dest + 32 * 6), ymm6);
            _mm256_storeu_si256((__m256i *)(dest + 32 * 7), ymm7);

            dest += 8 * kAVXRegBytes;
        }

        if (((src + (4 * kAVXRegBytes)) <= end) && (LeftUints >= 4)) {
            __m256i ymm0 = _mm256_loadu_si256((const __m256i *)(src + 32 * 0));
            __m256i ymm1 = _mm256_loadu_si256((const __m256i *)(src + 32 * 1));
            __m256i ymm2 = _mm256_loadu_si256((const __m256i *)(src + 32 * 2));
            __m256i ymm3 = _mm256_loadu_si256((const __m256i *)(src + 32 * 3));

            src += 4 * kAVXRegBytes;

            _mm256_storeu_si256((__m256i *)(dest + 32 * 0), ymm0);
            _mm256_storeu_si256((__m256i *)(dest + 32 * 1), ymm1);
            _mm256_storeu_si256((__m256i *)(dest + 32 * 2), ymm2);
            _mm256_storeu_si256((__m256i *)(dest + 32 * 3), ymm3);

            dest += 4 * kAVXRegBytes;
        }

        if (((src + (2 * kAVXRegBytes)) <= end) && (LeftUints >= 2)) {
            __m256i ymm0 = _mm256_loadu_si256((const __m256i *)(src + 32 * 0));
            __m256i ymm1 = _mm256_loadu_si256((const __m256i *)(src + 32 * 1));

            src += 2 * kAVXRegBytes;

            _mm256_storeu_si256((__m256i *)(dest + 32 * 0), ymm0);
            _mm256_storeu_si256((__m256i *)(dest + 32 * 1), ymm1);

            dest += 2 * kAVXRegBytes;
        }

        if (((src + (1 * kAVXRegBytes)) <= end) && (LeftUints >= 1)) {
            __m256i ymm0 = _mm256_loadu_si256((const __m256i *)(src + 32 * 0));

            src += 1 * kAVXRegBytes;

            _mm256_storeu_si256((__m256i *)(dest + 32 * 0), ymm0);

            dest += 1 * kAVXRegBytes;
        }
    }

    while (src < end) {
        *(T *)dest = *(T *)src;
        src += kValueSize;
        dest += kValueSize;
    }
}

template <typename T, bool srcIsAligned, bool destIsAligned, int LeftUints = 7>
static
JSTD_NO_INLINE
void avx_forward_move_N_tailing_nt(char * JSTD_RESTRICT dest, char * JSTD_RESTRICT src, char * JSTD_RESTRICT end)
{
    static const std::size_t kValueSize = sizeof(T);
    JSTD_ASSERT(end >= src);
    std::size_t left_bytes = (std::size_t)(end - src);
    JSTD_ASSERT((left_bytes % kValueSize) == 0);

    if (srcIsAligned && destIsAligned) {
        if (((src + (8 * kAVXRegBytes)) <= end) && (LeftUints >= 8)) {
            __m256i ymm0 = _mm256_load_si256((const __m256i *)(src + 32 * 0));
            __m256i ymm1 = _mm256_load_si256((const __m256i *)(src + 32 * 1));
            __m256i ymm2 = _mm256_load_si256((const __m256i *)(src + 32 * 2));
            __m256i ymm3 = _mm256_load_si256((const __m256i *)(src + 32 * 3));
            __m256i ymm4 = _mm256_load_si256((const __m256i *)(src + 32 * 4));
            __m256i ymm5 = _mm256_load_si256((const __m256i *)(src + 32 * 5));
            __m256i ymm6 = _mm256_load_si256((const __m256i *)(src + 32 * 6));
            __m256i ymm7 = _mm256_load_si256((const __m256i *)(src + 32 * 7));

            src += 8 * kAVXRegBytes;

            _mm256_stream_si256((__m256i *)(dest + 32 * 0), ymm0);
            _mm256_stream_si256((__m256i *)(dest + 32 * 1), ymm1);
            _mm256_stream_si256((__m256i *)(dest + 32 * 2), ymm2);
            _mm256_stream_si256((__m256i *)(dest + 32 * 3), ymm3);
            _mm256_stream_si256((__m256i *)(dest + 32 * 4), ymm4);
            _mm256_stream_si256((__m256i *)(dest + 32 * 5), ymm5);
            _mm256_stream_si256((__m256i *)(dest + 32 * 6), ymm6);
            _mm256_stream_si256((__m256i *)(dest + 32 * 7), ymm7);

            dest += 8 * kAVXRegBytes;
        }

        if (((src + (4 * kAVXRegBytes)) <= end) && (LeftUints >= 4)) {
            __m256i ymm0 = _mm256_load_si256((const __m256i *)(src + 32 * 0));
            __m256i ymm1 = _mm256_load_si256((const __m256i *)(src + 32 * 1));
            __m256i ymm2 = _mm256_load_si256((const __m256i *)(src + 32 * 2));
            __m256i ymm3 = _mm256_load_si256((const __m256i *)(src + 32 * 3));

            src += 4 * kAVXRegBytes;

            _mm256_stream_si256((__m256i *)(dest + 32 * 0), ymm0);
            _mm256_stream_si256((__m256i *)(dest + 32 * 1), ymm1);
            _mm256_stream_si256((__m256i *)(dest + 32 * 2), ymm2);
            _mm256_stream_si256((__m256i *)(dest + 32 * 3), ymm3);

            dest += 4 * kAVXRegBytes;
        }

        if (((src + (2 * kAVXRegBytes)) <= end) && (LeftUints >= 2)) {
            __m256i ymm0 = _mm256_load_si256((const __m256i *)(src + 32 * 0));
            __m256i ymm1 = _mm256_load_si256((const __m256i *)(src + 32 * 1));

            src += 2 * kAVXRegBytes;

            _mm256_stream_si256((__m256i *)(dest + 32 * 0), ymm0);
            _mm256_stream_si256((__m256i *)(dest + 32 * 1), ymm1);

            dest += 2 * kAVXRegBytes;
        }

        if (((src + (1 * kAVXRegBytes)) <= end) && (LeftUints >= 1)) {
            __m256i ymm0 = _mm256_load_si256((const __m256i *)(src + 32 * 0));

            src += 1 * kAVXRegBytes;

            _mm256_stream_si256((__m256i *)(dest + 32 * 0), ymm0);

            dest += 1 * kAVXRegBytes;
        }
    }
    else if (srcIsAligned && !destIsAligned) {
        if (((src + (8 * kAVXRegBytes)) <= end) && (LeftUints >= 8)) {
            __m256i ymm0 = _mm256_load_si256((const __m256i *)(src + 32 * 0));
            __m256i ymm1 = _mm256_load_si256((const __m256i *)(src + 32 * 1));
            __m256i ymm2 = _mm256_load_si256((const __m256i *)(src + 32 * 2));
            __m256i ymm3 = _mm256_load_si256((const __m256i *)(src + 32 * 3));
            __m256i ymm4 = _mm256_load_si256((const __m256i *)(src + 32 * 4));
            __m256i ymm5 = _mm256_load_si256((const __m256i *)(src + 32 * 5));
            __m256i ymm6 = _mm256_load_si256((const __m256i *)(src + 32 * 6));
            __m256i ymm7 = _mm256_load_si256((const __m256i *)(src + 32 * 7));

            src += 8 * kAVXRegBytes;

            _mm256_storeu_si256((__m256i *)(dest + 32 * 0), ymm0);
            _mm256_storeu_si256((__m256i *)(dest + 32 * 1), ymm1);
            _mm256_storeu_si256((__m256i *)(dest + 32 * 2), ymm2);
            _mm256_storeu_si256((__m256i *)(dest + 32 * 3), ymm3);
            _mm256_storeu_si256((__m256i *)(dest + 32 * 4), ymm4);
            _mm256_storeu_si256((__m256i *)(dest + 32 * 5), ymm5);
            _mm256_storeu_si256((__m256i *)(dest + 32 * 6), ymm6);
            _mm256_storeu_si256((__m256i *)(dest + 32 * 7), ymm7);

            dest += 8 * kAVXRegBytes;
        }

        if (((src + (4 * kAVXRegBytes)) <= end) && (LeftUints >= 4)) {
            __m256i ymm0 = _mm256_load_si256((const __m256i *)(src + 32 * 0));
            __m256i ymm1 = _mm256_load_si256((const __m256i *)(src + 32 * 1));
            __m256i ymm2 = _mm256_load_si256((const __m256i *)(src + 32 * 2));
            __m256i ymm3 = _mm256_load_si256((const __m256i *)(src + 32 * 3));

            src += 4 * kAVXRegBytes;

            _mm256_storeu_si256((__m256i *)(dest + 32 * 0), ymm0);
            _mm256_storeu_si256((__m256i *)(dest + 32 * 1), ymm1);
            _mm256_storeu_si256((__m256i *)(dest + 32 * 2), ymm2);
            _mm256_storeu_si256((__m256i *)(dest + 32 * 3), ymm3);

            dest += 4 * kAVXRegBytes;
        }

        if (((src + (2 * kAVXRegBytes)) <= end) && (LeftUints >= 2)) {
            __m256i ymm0 = _mm256_load_si256((const __m256i *)(src + 32 * 0));
            __m256i ymm1 = _mm256_load_si256((const __m256i *)(src + 32 * 1));

            src += 2 * kAVXRegBytes;

            _mm256_storeu_si256((__m256i *)(dest + 32 * 0), ymm0);
            _mm256_storeu_si256((__m256i *)(dest + 32 * 1), ymm1);

            dest += 2 * kAVXRegBytes;
        }

        if (((src + (1 * kAVXRegBytes)) <= end) && (LeftUints >= 1)) {
            __m256i ymm0 = _mm256_load_si256((const __m256i *)(src + 32 * 0));

            src += 1 * kAVXRegBytes;

            _mm256_storeu_si256((__m256i *)(dest + 32 * 0), ymm0);

            dest += 1 * kAVXRegBytes;
        }
    }
    else if (!srcIsAligned && destIsAligned) {
        if (((src + (8 * kAVXRegBytes)) <= end) && (LeftUints >= 8)) {
            __m256i ymm0 = _mm256_loadu_si256((const __m256i *)(src + 32 * 0));
            __m256i ymm1 = _mm256_loadu_si256((const __m256i *)(src + 32 * 1));
            __m256i ymm2 = _mm256_loadu_si256((const __m256i *)(src + 32 * 2));
            __m256i ymm3 = _mm256_loadu_si256((const __m256i *)(src + 32 * 3));
            __m256i ymm4 = _mm256_loadu_si256((const __m256i *)(src + 32 * 4));
            __m256i ymm5 = _mm256_loadu_si256((const __m256i *)(src + 32 * 5));
            __m256i ymm6 = _mm256_loadu_si256((const __m256i *)(src + 32 * 6));
            __m256i ymm7 = _mm256_loadu_si256((const __m256i *)(src + 32 * 7));

            src += 8 * kAVXRegBytes;

            _mm256_store_si256((__m256i *)(dest + 32 * 0), ymm0);
            _mm256_store_si256((__m256i *)(dest + 32 * 1), ymm1);
            _mm256_store_si256((__m256i *)(dest + 32 * 2), ymm2);
            _mm256_store_si256((__m256i *)(dest + 32 * 3), ymm3);
            _mm256_store_si256((__m256i *)(dest + 32 * 4), ymm4);
            _mm256_store_si256((__m256i *)(dest + 32 * 5), ymm5);
            _mm256_store_si256((__m256i *)(dest + 32 * 6), ymm6);
            _mm256_store_si256((__m256i *)(dest + 32 * 7), ymm7);

            dest += 8 * kAVXRegBytes;
        }

        if (((src + (4 * kAVXRegBytes)) <= end) && (LeftUints >= 4)) {
            __m256i ymm0 = _mm256_loadu_si256((const __m256i *)(src + 32 * 0));
            __m256i ymm1 = _mm256_loadu_si256((const __m256i *)(src + 32 * 1));
            __m256i ymm2 = _mm256_loadu_si256((const __m256i *)(src + 32 * 2));
            __m256i ymm3 = _mm256_loadu_si256((const __m256i *)(src + 32 * 3));

            src += 4 * kAVXRegBytes;

            _mm256_store_si256((__m256i *)(dest + 32 * 0), ymm0);
            _mm256_store_si256((__m256i *)(dest + 32 * 1), ymm1);
            _mm256_store_si256((__m256i *)(dest + 32 * 2), ymm2);
            _mm256_store_si256((__m256i *)(dest + 32 * 3), ymm3);

            dest += 4 * kAVXRegBytes;
        }

        if (((src + (2 * kAVXRegBytes)) <= end) && (LeftUints >= 2)) {
            __m256i ymm0 = _mm256_loadu_si256((const __m256i *)(src + 32 * 0));
            __m256i ymm1 = _mm256_loadu_si256((const __m256i *)(src + 32 * 1));

            src += 2 * kAVXRegBytes;

            _mm256_store_si256((__m256i *)(dest + 32 * 0), ymm0);
            _mm256_store_si256((__m256i *)(dest + 32 * 1), ymm1);

            dest += 2 * kAVXRegBytes;
        }

        if (((src + (1 * kAVXRegBytes)) <= end) && (LeftUints >= 1)) {
            __m256i ymm0 = _mm256_loadu_si256((const __m256i *)(src + 32 * 0));

            src += 1 * kAVXRegBytes;

            _mm256_store_si256((__m256i *)(dest + 32 * 0), ymm0);

            dest += 1 * kAVXRegBytes;
        }
    }
    else {
        if (((src + (8 * kAVXRegBytes)) <= end) && (LeftUints >= 8)) {
            __m256i ymm0 = _mm256_loadu_si256((const __m256i *)(src + 32 * 0));
            __m256i ymm1 = _mm256_loadu_si256((const __m256i *)(src + 32 * 1));
            __m256i ymm2 = _mm256_loadu_si256((const __m256i *)(src + 32 * 2));
            __m256i ymm3 = _mm256_loadu_si256((const __m256i *)(src + 32 * 3));
            __m256i ymm4 = _mm256_loadu_si256((const __m256i *)(src + 32 * 4));
            __m256i ymm5 = _mm256_loadu_si256((const __m256i *)(src + 32 * 5));
            __m256i ymm6 = _mm256_loadu_si256((const __m256i *)(src + 32 * 6));
            __m256i ymm7 = _mm256_loadu_si256((const __m256i *)(src + 32 * 7));

            src += 8 * kAVXRegBytes;

            _mm256_storeu_si256((__m256i *)(dest + 32 * 0), ymm0);
            _mm256_storeu_si256((__m256i *)(dest + 32 * 1), ymm1);
            _mm256_storeu_si256((__m256i *)(dest + 32 * 2), ymm2);
            _mm256_storeu_si256((__m256i *)(dest + 32 * 3), ymm3);
            _mm256_storeu_si256((__m256i *)(dest + 32 * 4), ymm4);
            _mm256_storeu_si256((__m256i *)(dest + 32 * 5), ymm5);
            _mm256_storeu_si256((__m256i *)(dest + 32 * 6), ymm6);
            _mm256_storeu_si256((__m256i *)(dest + 32 * 7), ymm7);

            dest += 8 * kAVXRegBytes;
        }

        if (((src + (4 * kAVXRegBytes)) <= end) && (LeftUints >= 4)) {
            __m256i ymm0 = _mm256_loadu_si256((const __m256i *)(src + 32 * 0));
            __m256i ymm1 = _mm256_loadu_si256((const __m256i *)(src + 32 * 1));
            __m256i ymm2 = _mm256_loadu_si256((const __m256i *)(src + 32 * 2));
            __m256i ymm3 = _mm256_loadu_si256((const __m256i *)(src + 32 * 3));

            src += 4 * kAVXRegBytes;

            _mm256_storeu_si256((__m256i *)(dest + 32 * 0), ymm0);
            _mm256_storeu_si256((__m256i *)(dest + 32 * 1), ymm1);
            _mm256_storeu_si256((__m256i *)(dest + 32 * 2), ymm2);
            _mm256_storeu_si256((__m256i *)(dest + 32 * 3), ymm3);

            dest += 4 * kAVXRegBytes;
        }

        if (((src + (2 * kAVXRegBytes)) <= end) && (LeftUints >= 2)) {
            __m256i ymm0 = _mm256_loadu_si256((const __m256i *)(src + 32 * 0));
            __m256i ymm1 = _mm256_loadu_si256((const __m256i *)(src + 32 * 1));

            src += 2 * kAVXRegBytes;

            _mm256_storeu_si256((__m256i *)(dest + 32 * 0), ymm0);
            _mm256_storeu_si256((__m256i *)(dest + 32 * 1), ymm1);

            dest += 2 * kAVXRegBytes;
        }

        if (((src + (1 * kAVXRegBytes)) <= end) && (LeftUints >= 1)) {
            __m256i ymm0 = _mm256_loadu_si256((const __m256i *)(src + 32 * 0));

            src += 1 * kAVXRegBytes;

            _mm256_storeu_si256((__m256i *)(dest + 32 * 0), ymm0);

            dest += 1 * kAVXRegBytes;
        }
    }

    while (src < end) {
        *(T *)dest = *(T *)src;
        src += kValueSize;
        dest += kValueSize;
    }
}

//
// Code block alignment setting to 64 bytes maybe better than 32,
// but it's too wasteful for code size.
//
#if defined(JSTD_IS_PURE_GCC)
#pragma GCC push_options
#pragma GCC optimize ("align-labels=32")
#endif

template <typename T, std::size_t N = 8,
                      bool srcIsAligned = false,
                      bool destIsAligned = false,
                      std::size_t estimatedSize = sizeof(T)>
JSTD_FORCE_INLINE
void avx_mem_copy_forward_store_aligned(void * JSTD_RESTRICT _dest, void * JSTD_RESTRICT _src, void * JSTD_RESTRICT _end)
{
    static const std::size_t kValueSize = sizeof(T);
    static const bool kValueSizeIsPower2 = ((kValueSize & (kValueSize - 1)) == 0);
    static const bool kValueSizeIsDivisible =  (kValueSize < kAVXRegBytes) ?
                                              ((kAVXRegBytes % kValueSize) == 0) :
                                              ((kValueSize % kAVXRegBytes) == 0);
    // minimum AVX regs = 1, maximum AVX regs = 8
    static const std::size_t _N = (N == 0) ? 1 : ((N <= 8) ? N : 8);
    static const std::size_t kSingleLoopBytes = _N * kAVXRegBytes;

    if (destIsAligned) {
        char * JSTD_RESTRICT dest = (char * JSTD_RESTRICT)_dest;
        char * JSTD_RESTRICT src = (char * JSTD_RESTRICT)_src;
        char * JSTD_RESTRICT end = (char * JSTD_RESTRICT)_end;

        JSTD_ASSERT(end >= src);
        std::size_t totalCopyBytes = (end - src);
        JSTD_ASSERT((totalCopyBytes % kValueSize) == 0);
        std::size_t unalignedCopyBytes = (std::size_t)totalCopyBytes % kSingleLoopBytes;
        const char * JSTD_RESTRICT limit = ((estimatedSize >= kSingleLoopBytes) || (totalCopyBytes >= kSingleLoopBytes))
                                        ? (end - unalignedCopyBytes) : src;

        std::size_t srcUnalignedBytes = (std::size_t)src & kAVXAlignMask;
        bool srcAddrCanAlign;
        if (kValueSize < kAVXRegBytes)
            srcAddrCanAlign = (kValueSizeIsDivisible && ((srcUnalignedBytes % kValueSize) == 0));
        else
            srcAddrCanAlign = (kValueSizeIsDivisible && (srcUnalignedBytes == 0));

        bool srcAddrIsAligned = (srcUnalignedBytes == 0);
        if (srcIsAligned || (srcAddrIsAligned && srcAddrCanAlign)) {
            // srcIsAligned = true, destIsAligned = true
#if defined(JSTD_IS_ICC)
#pragma code_align(64)
#endif
            while (src < limit) {
                __m256i ymm0, ymm1, ymm2, ymm3, ymm4, ymm5, ymm6, ymm7;
                if (N >= 0)
                    ymm0 = _mm256_load_si256((const __m256i *)(src + 32 * 0));
                if (N >= 2)
                    ymm1 = _mm256_load_si256((const __m256i *)(src + 32 * 1));
                if (N >= 3)
                    ymm2 = _mm256_load_si256((const __m256i *)(src + 32 * 2));
                if (N >= 4)
                    ymm3 = _mm256_load_si256((const __m256i *)(src + 32 * 3));
                if (N >= 5)
                    ymm4 = _mm256_load_si256((const __m256i *)(src + 32 * 4));
                if (N >= 6)
                    ymm5 = _mm256_load_si256((const __m256i *)(src + 32 * 5));
                if (N >= 7)
                    ymm6 = _mm256_load_si256((const __m256i *)(src + 32 * 6));
                // Use "{" and "}" to avoid the gcc warnings
                if (N >= 8) {
                    ymm7 = _mm256_load_si256((const __m256i *)(src + 32 * 7));
                }

                if (kUsePrefetchHint) {
                    // Here, N would be best a multiple of 2.
                    _mm_prefetch((const char *)(src + kPrefetchOffset + 64 * 0), kPrefetchHintLevel);
                    if (N >= 3)
                    _mm_prefetch((const char *)(src + kPrefetchOffset + 64 * 1), kPrefetchHintLevel);
                    if (N >= 5)
                    _mm_prefetch((const char *)(src + kPrefetchOffset + 64 * 2), kPrefetchHintLevel);
                    if (N >= 7)
                    _mm_prefetch((const char *)(src + kPrefetchOffset + 64 * 3), kPrefetchHintLevel);
                }

                src += kSingleLoopBytes;

                if (N >= 0)
                    _mm256_store_si256((__m256i *)(dest + 32 * 0), ymm0);
                if (N >= 2)
                    _mm256_store_si256((__m256i *)(dest + 32 * 1), ymm1);
                if (N >= 3)
                    _mm256_store_si256((__m256i *)(dest + 32 * 2), ymm2);
                if (N >= 4)
                    _mm256_store_si256((__m256i *)(dest + 32 * 3), ymm3);
                if (N >= 5)
                    _mm256_store_si256((__m256i *)(dest + 32 * 4), ymm4);
                if (N >= 6)
                    _mm256_store_si256((__m256i *)(dest + 32 * 5), ymm5);
                if (N >= 7)
                    _mm256_store_si256((__m256i *)(dest + 32 * 6), ymm6);
                // Use "{" and "}" to avoid the gcc warnings
                if (N >= 8) {
                    _mm256_store_si256((__m256i *)(dest + 32 * 7), ymm7);
                }

                dest += kSingleLoopBytes;
            }

            avx_forward_move_N_tailing<T, kSrcIsAligned, kDestIsAligned, _N - 1>(dest, src, end);
        } else {
            // srcIsAligned = false (Actually), destIsAligned = true
#if defined(JSTD_IS_ICC)
#pragma code_align(64)
#endif
            while (src < limit) {
                __m256i ymm0, ymm1, ymm2, ymm3, ymm4, ymm5, ymm6, ymm7;
                if (N >= 0)
                    ymm0 = _mm256_loadu_si256((const __m256i *)(src + 32 * 0));
                if (N >= 2)
                    ymm1 = _mm256_loadu_si256((const __m256i *)(src + 32 * 1));
                if (N >= 3)
                    ymm2 = _mm256_loadu_si256((const __m256i *)(src + 32 * 2));
                if (N >= 4)
                    ymm3 = _mm256_loadu_si256((const __m256i *)(src + 32 * 3));
                if (N >= 5)
                    ymm4 = _mm256_loadu_si256((const __m256i *)(src + 32 * 4));
                if (N >= 6)
                    ymm5 = _mm256_loadu_si256((const __m256i *)(src + 32 * 5));
                if (N >= 7)
                    ymm6 = _mm256_loadu_si256((const __m256i *)(src + 32 * 6));
                // Use "{" and "}" to avoid the gcc warnings
                if (N >= 8) {
                    ymm7 = _mm256_loadu_si256((const __m256i *)(src + 32 * 7));
                }

                if (kUsePrefetchHint) {
                    // Here, N would be best a multiple of 2.
                    _mm_prefetch((const char *)(src + kPrefetchOffset + 64 * 0), kPrefetchHintLevel);
                    if (N >= 3)
                    _mm_prefetch((const char *)(src + kPrefetchOffset + 64 * 1), kPrefetchHintLevel);
                    if (N >= 5)
                    _mm_prefetch((const char *)(src + kPrefetchOffset + 64 * 2), kPrefetchHintLevel);
                    if (N >= 7)
                    _mm_prefetch((const char *)(src + kPrefetchOffset + 64 * 3), kPrefetchHintLevel);
                }

                src += kSingleLoopBytes;

                if (N >= 0)
                    _mm256_store_si256((__m256i *)(dest + 32 * 0), ymm0);
                if (N >= 2)
                    _mm256_store_si256((__m256i *)(dest + 32 * 1), ymm1);
                if (N >= 3)
                    _mm256_store_si256((__m256i *)(dest + 32 * 2), ymm2);
                if (N >= 4)
                    _mm256_store_si256((__m256i *)(dest + 32 * 3), ymm3);
                if (N >= 5)
                    _mm256_store_si256((__m256i *)(dest + 32 * 4), ymm4);
                if (N >= 6)
                    _mm256_store_si256((__m256i *)(dest + 32 * 5), ymm5);
                if (N >= 7)
                    _mm256_store_si256((__m256i *)(dest + 32 * 6), ymm6);
                // Use "{" and "}" to avoid the gcc warnings
                if (N >= 8) {
                    _mm256_store_si256((__m256i *)(dest + 32 * 7), ymm7);
                }

                dest += kSingleLoopBytes;
            }

            avx_forward_move_N_tailing<T, kSrcIsNotAligned, kDestIsAligned, _N - 1>(dest, src, end);
        }
    }
    else {
        // destIsAligned = false (Unknown)
        if (srcIsAligned) {
            // srcIsAligned = true, destIsAligned = false (Unknown)
            char * JSTD_RESTRICT dest = (char * JSTD_RESTRICT)_dest;
            char * JSTD_RESTRICT src = (char * JSTD_RESTRICT)_src;
            char * JSTD_RESTRICT end = (char * JSTD_RESTRICT)_end;

            JSTD_ASSERT(end >= src);
            std::size_t totalCopyBytes = (end - src);
            JSTD_ASSERT((totalCopyBytes % kValueSize) == 0);
            std::size_t unalignedCopyBytes = (std::size_t)totalCopyBytes % kSingleLoopBytes;
            const char * JSTD_RESTRICT limit = ((estimatedSize >= kSingleLoopBytes) || (totalCopyBytes >= kSingleLoopBytes))
                                              ? (end - unalignedCopyBytes) : src;

            std::size_t destUnalignedBytes = (std::size_t)dest & kAVXAlignMask;
            bool destAddrCanAlign;
            if (kValueSize < kAVXRegBytes)
                destAddrCanAlign = (kValueSizeIsDivisible && ((destUnalignedBytes % kValueSize) == 0));
            else
                destAddrCanAlign = (kValueSizeIsDivisible && (destUnalignedBytes == 0));

            bool destAddrIsAligned = (destUnalignedBytes == 0);
            if (destAddrIsAligned && destAddrCanAlign) {
                // srcIsAligned = true, destIsAligned = true
                // TODO: XXXXX

            } else {
                // srcIsAligned = true, destIsAligned = false (Actually)
                // TODO: XXXXX

            }
        } else {
            // srcIsAligned = false (Unknown), destIsAligned = false (Unknown)
            char * JSTD_RESTRICT dest = (char * JSTD_RESTRICT)_dest;
            char * JSTD_RESTRICT src = (char * JSTD_RESTRICT)_src;
            char * JSTD_RESTRICT end = (char * JSTD_RESTRICT)_end;

            std::size_t srcUnalignedBytes = (std::size_t)src & kAVXAlignMask;
            bool srcAddrCanAlign;
            if (kValueSize < kAVXRegBytes)
                srcAddrCanAlign = (kValueSizeIsDivisible && ((srcUnalignedBytes % kValueSize) == 0));
            else
                srcAddrCanAlign = (kValueSizeIsDivisible && (srcUnalignedBytes == 0));

            bool destAddrIsAligned = true;
            if (!srcIsAligned && srcAddrCanAlign) {
                std::size_t srcPaddingBytes = (kAVXRegBytes - srcUnalignedBytes) & kAVXAlignMask;
                JSTD_ASSERT((srcPaddingBytes % kValueSize) == 0);
                while (srcPaddingBytes != 0) {
                    *(T *)dest = *(T *)src;
                    dest += kValueSize;
                    src += kValueSize;
                    srcPaddingBytes -= kValueSize;
                }

                destAddrIsAligned = (((std::size_t)dest & kAVXAlignMask) == 0);
            }

            if (srcIsAligned || srcAddrCanAlign) {
            }
        }
    }
}

template <typename T, std::size_t N = 8>
static
JSTD_NO_INLINE
void avx_forward_move_N_load_aligned(T * JSTD_RESTRICT first, T * JSTD_RESTRICT mid, T * JSTD_RESTRICT last)
{
    static const std::size_t kValueSize = sizeof(T);
    static const bool kValueSizeIsPower2 = ((kValueSize & (kValueSize - 1)) == 0);
    static const bool kValueSizeIsDivisible =  (kValueSize < kAVXRegBytes) ?
                                              ((kAVXRegBytes % kValueSize) == 0) :
                                              ((kValueSize % kAVXRegBytes) == 0);
    // minimum AVX regs = 1, maximum AVX regs = 8
    static const std::size_t _N = (N == 0) ? 1 : ((N <= 8) ? N : 8);
    static const std::size_t kSingleLoopBytes = _N * kAVXRegBytes;

    std::size_t srcUnalignedBytes = (std::size_t)mid & kAVXAlignMask;
    bool srcAddrCanAlign;
    if (kValueSize < kAVXRegBytes)
        srcAddrCanAlign = (kValueSizeIsDivisible && ((srcUnalignedBytes % kValueSize) == 0));
    else
        srcAddrCanAlign = (kValueSizeIsDivisible && (srcUnalignedBytes == 0));

    if (likely(kValueSizeIsDivisible && srcAddrCanAlign)) {
        std::size_t srcPaddingBytes = (kAVXRegBytes - srcUnalignedBytes) & kAVXAlignMask;
        while (srcPaddingBytes != 0) {
            *first++ = *mid++;
            srcPaddingBytes -= kValueSize;
        }

        char * JSTD_RESTRICT dest = (char * JSTD_RESTRICT)first;
        char * JSTD_RESTRICT src = (char * JSTD_RESTRICT)mid;
        char * JSTD_RESTRICT end = (char * JSTD_RESTRICT)last;

        std::size_t totalMoveBytes = (last - mid) * kValueSize;
        std::size_t unalignedMoveBytes = (std::size_t)totalMoveBytes % kSingleLoopBytes;
        const char * JSTD_RESTRICT limit = (totalMoveBytes >= kSingleLoopBytes) ? (end - unalignedMoveBytes) : src;

        bool destAddrIsAligned = (((std::size_t)dest & kAVXAlignMask) == 0);
        if (likely(!destAddrIsAligned)) {
#if defined(JSTD_IS_ICC)
#pragma code_align(64)
#endif
            while (src < limit) {
                __m256i ymm0, ymm1, ymm2, ymm3, ymm4, ymm5, ymm6, ymm7;
                if (N >= 0)
                    ymm0 = _mm256_load_si256((const __m256i *)(src + 32 * 0));
                if (N >= 2)
                    ymm1 = _mm256_load_si256((const __m256i *)(src + 32 * 1));
                if (N >= 3)
                    ymm2 = _mm256_load_si256((const __m256i *)(src + 32 * 2));
                if (N >= 4)
                    ymm3 = _mm256_load_si256((const __m256i *)(src + 32 * 3));
                if (N >= 5)
                    ymm4 = _mm256_load_si256((const __m256i *)(src + 32 * 4));
                if (N >= 6)
                    ymm5 = _mm256_load_si256((const __m256i *)(src + 32 * 5));
                if (N >= 7)
                    ymm6 = _mm256_load_si256((const __m256i *)(src + 32 * 6));
                // Use "{" and "}" to avoid the gcc warnings
                if (N >= 8) {
                    ymm7 = _mm256_load_si256((const __m256i *)(src + 32 * 7));
                }

                //
                // See: https://blog.csdn.net/qq_43401808/article/details/87360789
                //
                if (kUsePrefetchHint) {
                    // Here, N would be best a multiple of 2.
                    _mm_prefetch((const char *)(src + kPrefetchOffset + 64 * 0), kPrefetchHintLevel);
                    if (N >= 3)
                    _mm_prefetch((const char *)(src + kPrefetchOffset + 64 * 1), kPrefetchHintLevel);
                    if (N >= 5)
                    _mm_prefetch((const char *)(src + kPrefetchOffset + 64 * 2), kPrefetchHintLevel);
                    if (N >= 7)
                    _mm_prefetch((const char *)(src + kPrefetchOffset + 64 * 3), kPrefetchHintLevel);
                }

                src += kSingleLoopBytes;

                if (N >= 0)
                    _mm256_storeu_si256((__m256i *)(dest + 32 * 0), ymm0);
                if (N >= 2)
                    _mm256_storeu_si256((__m256i *)(dest + 32 * 1), ymm1);
                if (N >= 3)
                    _mm256_storeu_si256((__m256i *)(dest + 32 * 2), ymm2);
                if (N >= 4)
                    _mm256_storeu_si256((__m256i *)(dest + 32 * 3), ymm3);
                if (N >= 5)
                    _mm256_storeu_si256((__m256i *)(dest + 32 * 4), ymm4);
                if (N >= 6)
                    _mm256_storeu_si256((__m256i *)(dest + 32 * 5), ymm5);
                if (N >= 7)
                    _mm256_storeu_si256((__m256i *)(dest + 32 * 6), ymm6);
                // Use "{" and "}" to avoid the gcc warnings
                if (N >= 8) {
                    _mm256_storeu_si256((__m256i *)(dest + 32 * 7), ymm7);
                }

                dest += kSingleLoopBytes;
            }

            avx_forward_move_N_tailing<T, kSrcIsAligned, kDestIsNotAligned, _N - 1>(dest, src, end);
        } else {
#if defined(JSTD_IS_ICC)
#pragma code_align(64)
#endif
            while (src < limit) {
                __m256i ymm0, ymm1, ymm2, ymm3, ymm4, ymm5, ymm6, ymm7;
                if (N >= 0)
                    ymm0 = _mm256_load_si256((const __m256i *)(src + 32 * 0));
                if (N >= 2)
                    ymm1 = _mm256_load_si256((const __m256i *)(src + 32 * 1));
                if (N >= 3)
                    ymm2 = _mm256_load_si256((const __m256i *)(src + 32 * 2));
                if (N >= 4)
                    ymm3 = _mm256_load_si256((const __m256i *)(src + 32 * 3));
                if (N >= 5)
                    ymm4 = _mm256_load_si256((const __m256i *)(src + 32 * 4));
                if (N >= 6)
                    ymm5 = _mm256_load_si256((const __m256i *)(src + 32 * 5));
                if (N >= 7)
                    ymm6 = _mm256_load_si256((const __m256i *)(src + 32 * 6));
                // Use "{" and "}" to avoid the gcc warnings
                if (N >= 8) {
                    ymm7 = _mm256_load_si256((const __m256i *)(src + 32 * 7));
                }

                if (kUsePrefetchHint) {
                    // Here, N would be best a multiple of 2.
                    _mm_prefetch((const char *)(src + kPrefetchOffset + 64 * 0), kPrefetchHintLevel);
                    if (N >= 3)
                    _mm_prefetch((const char *)(src + kPrefetchOffset + 64 * 1), kPrefetchHintLevel);
                    if (N >= 5)
                    _mm_prefetch((const char *)(src + kPrefetchOffset + 64 * 2), kPrefetchHintLevel);
                    if (N >= 7)
                    _mm_prefetch((const char *)(src + kPrefetchOffset + 64 * 3), kPrefetchHintLevel);
                }

                src += kSingleLoopBytes;

                if (N >= 0)
                    _mm256_store_si256((__m256i *)(dest + 32 * 0), ymm0);
                if (N >= 2)
                    _mm256_store_si256((__m256i *)(dest + 32 * 1), ymm1);
                if (N >= 3)
                    _mm256_store_si256((__m256i *)(dest + 32 * 2), ymm2);
                if (N >= 4)
                    _mm256_store_si256((__m256i *)(dest + 32 * 3), ymm3);
                if (N >= 5)
                    _mm256_store_si256((__m256i *)(dest + 32 * 4), ymm4);
                if (N >= 6)
                    _mm256_store_si256((__m256i *)(dest + 32 * 5), ymm5);
                if (N >= 7)
                    _mm256_store_si256((__m256i *)(dest + 32 * 6), ymm6);
                // Use "{" and "}" to avoid the gcc warnings
                if (N >= 8) {
                    _mm256_store_si256((__m256i *)(dest + 32 * 7), ymm7);
                }

                dest += kSingleLoopBytes;
            }

            avx_forward_move_N_tailing<T, kSrcIsAligned, kDestIsAligned, _N - 1>(dest, src, end);
        }
    }
    else {
        bool destAddrCanAlign = false;
        if (kValueSizeIsDivisible) {
            std::size_t destUnalignedBytes = (std::size_t)first & kAVXAlignMask;
            if (kValueSize < kAVXRegBytes)
                destAddrCanAlign = ((destUnalignedBytes % kValueSize) == 0);
            else
                destAddrCanAlign = (destUnalignedBytes == 0);

            if (destAddrCanAlign) {
                std::size_t destPaddingBytes = (kAVXRegBytes - destUnalignedBytes) & kAVXAlignMask;
                while (destPaddingBytes != 0) {
                    *first++ = *mid++;
                    destPaddingBytes -= kValueSize;
                }
            }
        }

        char * JSTD_RESTRICT dest = (char * JSTD_RESTRICT)first;
        char * JSTD_RESTRICT src = (char * JSTD_RESTRICT)mid;
        char * JSTD_RESTRICT end = (char * JSTD_RESTRICT)last;

        std::size_t totalMoveBytes = (last - mid) * kValueSize;
        std::size_t unalignedMoveBytes = (std::size_t)totalMoveBytes % kSingleLoopBytes;
        const char * JSTD_RESTRICT limit = (totalMoveBytes >= kSingleLoopBytes) ? (end - unalignedMoveBytes) : src;

        if (likely(destAddrCanAlign)) {
#if defined(JSTD_IS_ICC)
#pragma code_align(64)
#endif
            while (src < limit) {
                __m256i ymm0, ymm1, ymm2, ymm3, ymm4, ymm5, ymm6, ymm7;
                if (N >= 0)
                    ymm0 = _mm256_loadu_si256((const __m256i *)(src + 32 * 0));
                if (N >= 2)
                    ymm1 = _mm256_loadu_si256((const __m256i *)(src + 32 * 1));
                if (N >= 3)
                    ymm2 = _mm256_loadu_si256((const __m256i *)(src + 32 * 2));
                if (N >= 4)
                    ymm3 = _mm256_loadu_si256((const __m256i *)(src + 32 * 3));
                if (N >= 5)
                    ymm4 = _mm256_loadu_si256((const __m256i *)(src + 32 * 4));
                if (N >= 6)
                    ymm5 = _mm256_loadu_si256((const __m256i *)(src + 32 * 5));
                if (N >= 7)
                    ymm6 = _mm256_loadu_si256((const __m256i *)(src + 32 * 6));
                // Use "{" and "}" to avoid the gcc warnings
                if (N >= 8) {
                    ymm7 = _mm256_loadu_si256((const __m256i *)(src + 32 * 7));
                }

                if (kUsePrefetchHint) {
                    // Here, N would be best a multiple of 2.
                    _mm_prefetch((const char *)(src + kPrefetchOffset + 64 * 0), kPrefetchHintLevel);
                    if (N >= 3)
                    _mm_prefetch((const char *)(src + kPrefetchOffset + 64 * 1), kPrefetchHintLevel);
                    if (N >= 5)
                    _mm_prefetch((const char *)(src + kPrefetchOffset + 64 * 2), kPrefetchHintLevel);
                    if (N >= 7)
                    _mm_prefetch((const char *)(src + kPrefetchOffset + 64 * 3), kPrefetchHintLevel);
                }

                src += kSingleLoopBytes;

                if (N >= 0)
                    _mm256_store_si256((__m256i *)(dest + 32 * 0), ymm0);
                if (N >= 2)
                    _mm256_store_si256((__m256i *)(dest + 32 * 1), ymm1);
                if (N >= 3)
                    _mm256_store_si256((__m256i *)(dest + 32 * 2), ymm2);
                if (N >= 4)
                    _mm256_store_si256((__m256i *)(dest + 32 * 3), ymm3);
                if (N >= 5)
                    _mm256_store_si256((__m256i *)(dest + 32 * 4), ymm4);
                if (N >= 6)
                    _mm256_store_si256((__m256i *)(dest + 32 * 5), ymm5);
                if (N >= 7)
                    _mm256_store_si256((__m256i *)(dest + 32 * 6), ymm6);
                // Use "{" and "}" to avoid the gcc warnings
                if (N >= 8) {
                    _mm256_store_si256((__m256i *)(dest + 32 * 7), ymm7);
                }

                dest += kSingleLoopBytes;
            }

            avx_forward_move_N_tailing<T, kSrcIsNotAligned, kDestIsAligned, _N - 1>(dest, src, end);
        } else {
#if defined(JSTD_IS_ICC)
#pragma code_align(64)
#endif
            while (src < limit) {
                __m256i ymm0, ymm1, ymm2, ymm3, ymm4, ymm5, ymm6, ymm7;
                if (N >= 0)
                    ymm0 = _mm256_loadu_si256((const __m256i *)(src + 32 * 0));
                if (N >= 2)
                    ymm1 = _mm256_loadu_si256((const __m256i *)(src + 32 * 1));
                if (N >= 3)
                    ymm2 = _mm256_loadu_si256((const __m256i *)(src + 32 * 2));
                if (N >= 4)
                    ymm3 = _mm256_loadu_si256((const __m256i *)(src + 32 * 3));
                if (N >= 5)
                    ymm4 = _mm256_loadu_si256((const __m256i *)(src + 32 * 4));
                if (N >= 6)
                    ymm5 = _mm256_loadu_si256((const __m256i *)(src + 32 * 5));
                if (N >= 7)
                    ymm6 = _mm256_loadu_si256((const __m256i *)(src + 32 * 6));
                // Use "{" and "}" to avoid the gcc warnings
                if (N >= 8) {
                    ymm7 = _mm256_loadu_si256((const __m256i *)(src + 32 * 7));
                }

                if (kUsePrefetchHint) {
                    // Here, N would be best a multiple of 2.
                    _mm_prefetch((const char *)(src + kPrefetchOffset + 64 * 0), kPrefetchHintLevel);
                    if (N >= 3)
                    _mm_prefetch((const char *)(src + kPrefetchOffset + 64 * 1), kPrefetchHintLevel);
                    if (N >= 5)
                    _mm_prefetch((const char *)(src + kPrefetchOffset + 64 * 2), kPrefetchHintLevel);
                    if (N >= 7)
                    _mm_prefetch((const char *)(src + kPrefetchOffset + 64 * 3), kPrefetchHintLevel);
                }

                src += kSingleLoopBytes;

                if (N >= 0)
                    _mm256_storeu_si256((__m256i *)(dest + 32 * 0), ymm0);
                if (N >= 2)
                    _mm256_storeu_si256((__m256i *)(dest + 32 * 1), ymm1);
                if (N >= 3)
                    _mm256_storeu_si256((__m256i *)(dest + 32 * 2), ymm2);
                if (N >= 4)
                    _mm256_storeu_si256((__m256i *)(dest + 32 * 3), ymm3);
                if (N >= 5)
                    _mm256_storeu_si256((__m256i *)(dest + 32 * 4), ymm4);
                if (N >= 6)
                    _mm256_storeu_si256((__m256i *)(dest + 32 * 5), ymm5);
                if (N >= 7)
                    _mm256_storeu_si256((__m256i *)(dest + 32 * 6), ymm6);
                // Use "{" and "}" to avoid the gcc warnings
                if (N >= 8) {
                    _mm256_storeu_si256((__m256i *)(dest + 32 * 7), ymm7);
                }

                dest += kSingleLoopBytes;
            }

            avx_forward_move_N_tailing<T, kSrcIsNotAligned, kDestIsNotAligned, _N - 1>(dest, src, end);
        }
    }
}

template <typename T, std::size_t N = 8, std::size_t estimatedSize = sizeof(T)>
static
JSTD_NO_INLINE
void avx_forward_move_N_store_aligned(T * JSTD_RESTRICT first, T * JSTD_RESTRICT mid, T * JSTD_RESTRICT last)
{
    static const std::size_t kValueSize = sizeof(T);
    static const bool kValueSizeIsPower2 = ((kValueSize & (kValueSize - 1)) == 0);
    static const bool kValueSizeIsDivisible =  (kValueSize < kAVXRegBytes) ?
                                              ((kAVXRegBytes % kValueSize) == 0) :
                                              ((kValueSize % kAVXRegBytes) == 0);
    // minimum AVX regs = 1, maximum AVX regs = 8
    static const std::size_t _N = (N == 0) ? 1 : ((N <= 8) ? N : 8);
    static const std::size_t kSingleLoopBytes = _N * kAVXRegBytes;

    std::size_t destUnalignedBytes = (std::size_t)first & kAVXAlignMask;
    bool destAddrCanAlign;
    if (kValueSize < kAVXRegBytes)
        destAddrCanAlign = (kValueSizeIsDivisible && ((destUnalignedBytes % kValueSize) == 0));
    else
        destAddrCanAlign = (kValueSizeIsDivisible && (destUnalignedBytes == 0));

    if (likely(kValueSizeIsDivisible && destAddrCanAlign)) {
        std::size_t destPaddingBytes = (kAVXRegBytes - destUnalignedBytes) & kAVXAlignMask;
        while (destPaddingBytes != 0) {
            *first++ = *mid++;
            destPaddingBytes -= kValueSize;
        }

        char * JSTD_RESTRICT dest = (char * JSTD_RESTRICT)first;
        char * JSTD_RESTRICT src = (char * JSTD_RESTRICT)mid;
        char * JSTD_RESTRICT end = (char * JSTD_RESTRICT)last;

        std::size_t totalMoveBytes = (last - mid) * kValueSize;
        std::size_t unalignedMoveBytes = (std::size_t)totalMoveBytes % kSingleLoopBytes;
        const char * JSTD_RESTRICT limit = (totalMoveBytes >= kSingleLoopBytes) ? (end - unalignedMoveBytes) : src;

        bool srcAddrIsAligned = (((std::size_t)src & kAVXAlignMask) == 0);
        if (likely(!srcAddrIsAligned)) {
#if defined(JSTD_IS_ICC)
#pragma code_align(64)
#endif
            while (src < limit) {
                __m256i ymm0, ymm1, ymm2, ymm3, ymm4, ymm5, ymm6, ymm7;
                if (N >= 0) {
                    ymm0 = _mm256_loadu_si256((const __m256i *)(src + 32 * 0));
                    jstd_compiler_barrier();
                }
                if (N >= 2) {
                    ymm1 = _mm256_loadu_si256((const __m256i *)(src + 32 * 1));
                    jstd_compiler_barrier();
                }
                if (N >= 3) {
                    ymm2 = _mm256_loadu_si256((const __m256i *)(src + 32 * 2));
                    jstd_compiler_barrier();
                }
                if (N >= 4) {
                    ymm3 = _mm256_loadu_si256((const __m256i *)(src + 32 * 3));
                    jstd_compiler_barrier();
                }
                if (N >= 5) {
                    ymm4 = _mm256_loadu_si256((const __m256i *)(src + 32 * 4));
                    jstd_compiler_barrier();
                }
                if (N >= 6) {
                    ymm5 = _mm256_loadu_si256((const __m256i *)(src + 32 * 5));
                    jstd_compiler_barrier();
                }
                if (N >= 7) {
                    ymm6 = _mm256_loadu_si256((const __m256i *)(src + 32 * 6));
                    jstd_compiler_barrier();
                }
                // Use "{" and "}" to avoid the gcc warnings
                if (N >= 8) {
                    ymm7 = _mm256_loadu_si256((const __m256i *)(src + 32 * 7));
                    jstd_compiler_barrier();
                }

                //
                // See: https://blog.csdn.net/qq_43401808/article/details/87360789
                //
                if (kUsePrefetchHint) {
                    // Here, N would be best a multiple of 2.
                    _mm_prefetch((const char *)(src + kPrefetchOffset + 64 * 0), kPrefetchHintLevel);
                    if (N >= 3)
                    _mm_prefetch((const char *)(src + kPrefetchOffset + 64 * 1), kPrefetchHintLevel);
                    if (N >= 5)
                    _mm_prefetch((const char *)(src + kPrefetchOffset + 64 * 2), kPrefetchHintLevel);
                    if (N >= 7)
                    _mm_prefetch((const char *)(src + kPrefetchOffset + 64 * 3), kPrefetchHintLevel);
                }

                src += kSingleLoopBytes;

                if (N >= 0) {
                    _mm256_store_si256((__m256i *)(dest + 32 * 0), ymm0);
                    jstd_compiler_barrier();
                }
                if (N >= 2) {
                    _mm256_store_si256((__m256i *)(dest + 32 * 1), ymm1);
                    jstd_compiler_barrier();
                }
                if (N >= 3) {
                    _mm256_store_si256((__m256i *)(dest + 32 * 2), ymm2);
                    jstd_compiler_barrier();
                }
                if (N >= 4) {
                    _mm256_store_si256((__m256i *)(dest + 32 * 3), ymm3);
                    jstd_compiler_barrier();
                }
                if (N >= 5) {
                    _mm256_store_si256((__m256i *)(dest + 32 * 4), ymm4);
                    jstd_compiler_barrier();
                }
                if (N >= 6) {
                    _mm256_store_si256((__m256i *)(dest + 32 * 5), ymm5);
                    jstd_compiler_barrier();
                }
                if (N >= 7) {
                    _mm256_store_si256((__m256i *)(dest + 32 * 6), ymm6);
                    jstd_compiler_barrier();
                }
                // Use "{" and "}" to avoid the gcc warnings
                if (N >= 8) {
                    _mm256_store_si256((__m256i *)(dest + 32 * 7), ymm7);
                    jstd_compiler_barrier();
                }

                dest += kSingleLoopBytes;
            }

            avx_forward_move_N_tailing<T, kSrcIsNotAligned, kDestIsAligned, _N - 1>(dest, src, end);
        } else {
#if defined(JSTD_IS_ICC)
#pragma code_align(64)
#endif
            while (src < limit) {
                __m256i ymm0, ymm1, ymm2, ymm3, ymm4, ymm5, ymm6, ymm7;
                if (N >= 0) {
                    ymm0 = _mm256_load_si256((const __m256i *)(src + 32 * 0));
                    jstd_compiler_barrier();
                }
                if (N >= 2) {
                    ymm1 = _mm256_load_si256((const __m256i *)(src + 32 * 1));
                    jstd_compiler_barrier();
                }
                if (N >= 3) {
                    ymm2 = _mm256_load_si256((const __m256i *)(src + 32 * 2));
                    jstd_compiler_barrier();
                }
                if (N >= 4) {
                    ymm3 = _mm256_load_si256((const __m256i *)(src + 32 * 3));
                    jstd_compiler_barrier();
                }
                if (N >= 5) {
                    ymm4 = _mm256_load_si256((const __m256i *)(src + 32 * 4));
                    jstd_compiler_barrier();
                }
                if (N >= 6) {
                    ymm5 = _mm256_load_si256((const __m256i *)(src + 32 * 5));
                    jstd_compiler_barrier();
                }
                if (N >= 7) {
                    ymm6 = _mm256_load_si256((const __m256i *)(src + 32 * 6));
                    jstd_compiler_barrier();
                }
                // Use "{" and "}" to avoid the gcc warnings
                if (N >= 8) {
                    ymm7 = _mm256_load_si256((const __m256i *)(src + 32 * 7));
                    jstd_compiler_barrier();
                }

                if (kUsePrefetchHint) {
                    // Here, N would be best a multiple of 2.
                    _mm_prefetch((const char *)(src + kPrefetchOffset + 64 * 0), kPrefetchHintLevel);
                    if (N >= 3)
                    _mm_prefetch((const char *)(src + kPrefetchOffset + 64 * 1), kPrefetchHintLevel);
                    if (N >= 5)
                    _mm_prefetch((const char *)(src + kPrefetchOffset + 64 * 2), kPrefetchHintLevel);
                    if (N >= 7)
                    _mm_prefetch((const char *)(src + kPrefetchOffset + 64 * 3), kPrefetchHintLevel);
                }

                src += kSingleLoopBytes;

                if (N >= 0) {
                    _mm256_store_si256((__m256i *)(dest + 32 * 0), ymm0);
                    jstd_compiler_barrier();
                }
                if (N >= 2) {
                    _mm256_store_si256((__m256i *)(dest + 32 * 1), ymm1);
                    jstd_compiler_barrier();
                }
                if (N >= 3) {
                    _mm256_store_si256((__m256i *)(dest + 32 * 2), ymm2);
                    jstd_compiler_barrier();
                }
                if (N >= 4) {
                    _mm256_store_si256((__m256i *)(dest + 32 * 3), ymm3);
                    jstd_compiler_barrier();
                }
                if (N >= 5) {
                    _mm256_store_si256((__m256i *)(dest + 32 * 4), ymm4);
                    jstd_compiler_barrier();
                }
                if (N >= 6) {
                    _mm256_store_si256((__m256i *)(dest + 32 * 5), ymm5);
                    jstd_compiler_barrier();
                }
                if (N >= 7) {
                    _mm256_store_si256((__m256i *)(dest + 32 * 6), ymm6);
                    jstd_compiler_barrier();
                }
                // Use "{" and "}" to avoid the gcc warnings
                if (N >= 8) {
                    _mm256_store_si256((__m256i *)(dest + 32 * 7), ymm7);
                    jstd_compiler_barrier();
                }

                dest += kSingleLoopBytes;
            }

            avx_forward_move_N_tailing<T, kSrcIsAligned, kDestIsAligned, _N - 1>(dest, src, end);
        }
    }
    else {
        bool srcAddrCanAlign = false;
        if (kValueSizeIsDivisible) {
            std::size_t srcUnalignedBytes = (std::size_t)mid & kAVXAlignMask;
            if (kValueSize < kAVXRegBytes)
                srcAddrCanAlign = ((srcUnalignedBytes % kValueSize) == 0);
            else
                srcAddrCanAlign = (srcUnalignedBytes == 0);

            if (srcAddrCanAlign) {
                std::size_t srcPaddingBytes = (kAVXRegBytes - srcUnalignedBytes) & kAVXAlignMask;
                while (srcPaddingBytes != 0) {
                    *first++ = *mid++;
                    srcPaddingBytes -= kValueSize;
                }
            }
        }

        char * JSTD_RESTRICT dest = (char * JSTD_RESTRICT)first;
        char * JSTD_RESTRICT src = (char * JSTD_RESTRICT)mid;
        char * JSTD_RESTRICT end = (char * JSTD_RESTRICT)last;

        std::size_t totalMoveBytes = (last - mid) * kValueSize;
        std::size_t unalignedMoveBytes = (std::size_t)totalMoveBytes % kSingleLoopBytes;
        const char * JSTD_RESTRICT limit = (totalMoveBytes >= kSingleLoopBytes) ? (end - unalignedMoveBytes) : src;

        if (likely(srcAddrCanAlign)) {
#if defined(JSTD_IS_ICC)
#pragma code_align(64)
#endif
            while (src < limit) {
                __m256i ymm0, ymm1, ymm2, ymm3, ymm4, ymm5, ymm6, ymm7;
                if (N >= 0) {
                    ymm0 = _mm256_load_si256((const __m256i *)(src + 32 * 0));
                    jstd_compiler_barrier();
                }
                if (N >= 2) {
                    ymm1 = _mm256_load_si256((const __m256i *)(src + 32 * 1));
                    jstd_compiler_barrier();
                }
                if (N >= 3) {
                    ymm2 = _mm256_load_si256((const __m256i *)(src + 32 * 2));
                    jstd_compiler_barrier();
                }
                if (N >= 4) {
                    ymm3 = _mm256_load_si256((const __m256i *)(src + 32 * 3));
                    jstd_compiler_barrier();
                }
                if (N >= 5) {
                    ymm4 = _mm256_load_si256((const __m256i *)(src + 32 * 4));
                    jstd_compiler_barrier();
                }
                if (N >= 6) {
                    ymm5 = _mm256_load_si256((const __m256i *)(src + 32 * 5));
                    jstd_compiler_barrier();
                }
                if (N >= 7) {
                    ymm6 = _mm256_load_si256((const __m256i *)(src + 32 * 6));
                    jstd_compiler_barrier();
                }
                // Use "{" and "}" to avoid the gcc warnings
                if (N >= 8) {
                    ymm7 = _mm256_load_si256((const __m256i *)(src + 32 * 7));
                    jstd_compiler_barrier();
                }

                if (kUsePrefetchHint) {
                    // Here, N would be best a multiple of 2.
                    _mm_prefetch((const char *)(src + kPrefetchOffset + 64 * 0), kPrefetchHintLevel);
                    if (N >= 3)
                    _mm_prefetch((const char *)(src + kPrefetchOffset + 64 * 1), kPrefetchHintLevel);
                    if (N >= 5)
                    _mm_prefetch((const char *)(src + kPrefetchOffset + 64 * 2), kPrefetchHintLevel);
                    if (N >= 7)
                    _mm_prefetch((const char *)(src + kPrefetchOffset + 64 * 3), kPrefetchHintLevel);
                }

                src += kSingleLoopBytes;

                if (N >= 0) {
                    _mm256_storeu_si256((__m256i *)(dest + 32 * 0), ymm0);
                    jstd_compiler_barrier();
                }
                if (N >= 2) {
                    _mm256_storeu_si256((__m256i *)(dest + 32 * 1), ymm1);
                    jstd_compiler_barrier();
                }
                if (N >= 3) {
                    _mm256_storeu_si256((__m256i *)(dest + 32 * 2), ymm2);
                    jstd_compiler_barrier();
                }
                if (N >= 4) {
                    _mm256_storeu_si256((__m256i *)(dest + 32 * 3), ymm3);
                    jstd_compiler_barrier();
                }
                if (N >= 5) {
                    _mm256_storeu_si256((__m256i *)(dest + 32 * 4), ymm4);
                    jstd_compiler_barrier();
                }
                if (N >= 6) {
                    _mm256_storeu_si256((__m256i *)(dest + 32 * 5), ymm5);
                    jstd_compiler_barrier();
                }
                if (N >= 7) {
                    _mm256_storeu_si256((__m256i *)(dest + 32 * 6), ymm6);
                    jstd_compiler_barrier();
                }
                // Use "{" and "}" to avoid the gcc warnings
                if (N >= 8) {
                    _mm256_storeu_si256((__m256i *)(dest + 32 * 7), ymm7);
                    jstd_compiler_barrier();
                }

                dest += kSingleLoopBytes;
            }

            avx_forward_move_N_tailing<T, kSrcIsAligned, kDestIsNotAligned, _N - 1>(dest, src, end);
        } else {
#if defined(JSTD_IS_ICC)
#pragma code_align(64)
#endif
            while (src < limit) {
                __m256i ymm0, ymm1, ymm2, ymm3, ymm4, ymm5, ymm6, ymm7;
                if (N >= 0) {
                    ymm0 = _mm256_loadu_si256((const __m256i *)(src + 32 * 0));
                    jstd_compiler_barrier();
                }
                if (N >= 2) {
                    ymm1 = _mm256_loadu_si256((const __m256i *)(src + 32 * 1));
                    jstd_compiler_barrier();
                }
                if (N >= 3) {
                    ymm2 = _mm256_loadu_si256((const __m256i *)(src + 32 * 2));
                    jstd_compiler_barrier();
                }
                if (N >= 4) {
                    ymm3 = _mm256_loadu_si256((const __m256i *)(src + 32 * 3));
                    jstd_compiler_barrier();
                }
                if (N >= 5) {
                    ymm4 = _mm256_loadu_si256((const __m256i *)(src + 32 * 4));
                    jstd_compiler_barrier();
                }
                if (N >= 6) {
                    ymm5 = _mm256_loadu_si256((const __m256i *)(src + 32 * 5));
                    jstd_compiler_barrier();
                }
                if (N >= 7) {
                    ymm6 = _mm256_loadu_si256((const __m256i *)(src + 32 * 6));
                    jstd_compiler_barrier();
                }
                // Use "{" and "}" to avoid the gcc warnings
                if (N >= 8) {
                    ymm7 = _mm256_loadu_si256((const __m256i *)(src + 32 * 7));
                    jstd_compiler_barrier();
                }

                if (kUsePrefetchHint) {
                    // Here, N would be best a multiple of 2.
                    _mm_prefetch((const char *)(src + kPrefetchOffset + 64 * 0), kPrefetchHintLevel);
                    if (N >= 3)
                    _mm_prefetch((const char *)(src + kPrefetchOffset + 64 * 1), kPrefetchHintLevel);
                    if (N >= 5)
                    _mm_prefetch((const char *)(src + kPrefetchOffset + 64 * 2), kPrefetchHintLevel);
                    if (N >= 7)
                    _mm_prefetch((const char *)(src + kPrefetchOffset + 64 * 3), kPrefetchHintLevel);
                }

                src += kSingleLoopBytes;

                if (N >= 0) {
                    _mm256_storeu_si256((__m256i *)(dest + 32 * 0), ymm0);
                    jstd_compiler_barrier();
                }
                if (N >= 2) {
                    _mm256_storeu_si256((__m256i *)(dest + 32 * 1), ymm1);
                    jstd_compiler_barrier();
                }
                if (N >= 3) {
                    _mm256_storeu_si256((__m256i *)(dest + 32 * 2), ymm2);
                    jstd_compiler_barrier();
                }
                if (N >= 4) {
                    _mm256_storeu_si256((__m256i *)(dest + 32 * 3), ymm3);
                    jstd_compiler_barrier();
                }
                if (N >= 5) {
                    _mm256_storeu_si256((__m256i *)(dest + 32 * 4), ymm4);
                    jstd_compiler_barrier();
                }
                if (N >= 6) {
                    _mm256_storeu_si256((__m256i *)(dest + 32 * 5), ymm5);
                    jstd_compiler_barrier();
                }
                if (N >= 7) {
                    _mm256_storeu_si256((__m256i *)(dest + 32 * 6), ymm6);
                    jstd_compiler_barrier();
                }
                // Use "{" and "}" to avoid the gcc warnings
                if (N >= 8) {
                    _mm256_storeu_si256((__m256i *)(dest + 32 * 7), ymm7);
                    jstd_compiler_barrier();
                }

                dest += kSingleLoopBytes;
            }

            avx_forward_move_N_tailing<T, kSrcIsNotAligned, kDestIsNotAligned, _N - 1>(dest, src, end);
        }
    }
}

template <typename T, std::size_t N = 8>
static
JSTD_NO_INLINE
void avx_forward_move_N_store_aligned_nt(T * JSTD_RESTRICT first, T * JSTD_RESTRICT mid, T * JSTD_RESTRICT last)
{
    static const std::size_t kValueSize = sizeof(T);
    static const bool kValueSizeIsPower2 = ((kValueSize & (kValueSize - 1)) == 0);
    static const bool kValueSizeIsDivisible =  (kValueSize < kAVXRegBytes) ?
                                              ((kAVXRegBytes % kValueSize) == 0) :
                                              ((kValueSize % kAVXRegBytes) == 0);
    // minimum AVX regs = 1, maximum AVX regs = 8
    static const std::size_t _N = (N == 0) ? 1 : ((N <= 8) ? N : 8);
    static const std::size_t kSingleLoopBytes = _N * kAVXRegBytes;

    std::size_t destUnalignedBytes = (std::size_t)first & kAVXAlignMask;
    bool destAddrCanAlign;
    if (kValueSize < kAVXRegBytes)
        destAddrCanAlign = (kValueSizeIsDivisible && ((destUnalignedBytes % kValueSize) == 0));
    else
        destAddrCanAlign = (kValueSizeIsDivisible && (destUnalignedBytes == 0));

    if (likely(kValueSizeIsDivisible && destAddrCanAlign)) {
        std::size_t destPaddingBytes = (kAVXRegBytes - destUnalignedBytes) & kAVXAlignMask;
        while (destPaddingBytes != 0) {
            *first++ = *mid++;
            destPaddingBytes -= kValueSize;
        }

        char * JSTD_RESTRICT dest = (char * JSTD_RESTRICT)first;
        char * JSTD_RESTRICT src = (char * JSTD_RESTRICT)mid;
        char * JSTD_RESTRICT end = (char * JSTD_RESTRICT)last;

        std::size_t totalMoveBytes = (last - mid) * kValueSize;
        std::size_t unalignedMoveBytes = (std::size_t)totalMoveBytes % kSingleLoopBytes;
        const char * JSTD_RESTRICT limit = (totalMoveBytes >= kSingleLoopBytes) ? (end - unalignedMoveBytes) : src;

        bool srcAddrIsAligned = (((std::size_t)src & kAVXAlignMask) == 0);
        if (likely(!srcAddrIsAligned)) {
            while (src < limit) {
                __m256i ymm0, ymm1, ymm2, ymm3, ymm4, ymm5, ymm6, ymm7;
                if (N >= 0)
                    ymm0 = _mm256_loadu_si256((const __m256i *)(src + 32 * 0));
                if (N >= 2)
                    ymm1 = _mm256_loadu_si256((const __m256i *)(src + 32 * 1));
                if (N >= 3)
                    ymm2 = _mm256_loadu_si256((const __m256i *)(src + 32 * 2));
                if (N >= 4)
                    ymm3 = _mm256_loadu_si256((const __m256i *)(src + 32 * 3));
                if (N >= 5)
                    ymm4 = _mm256_loadu_si256((const __m256i *)(src + 32 * 4));
                if (N >= 6)
                    ymm5 = _mm256_loadu_si256((const __m256i *)(src + 32 * 5));
                if (N >= 7)
                    ymm6 = _mm256_loadu_si256((const __m256i *)(src + 32 * 6));
                // Use "{" and "}" to avoid the gcc warnings
                if (N >= 8) {
                    ymm7 = _mm256_loadu_si256((const __m256i *)(src + 32 * 7));
                }

                //
                // See: https://blog.csdn.net/qq_43401808/article/details/87360789
                //
                if (kUsePrefetchHint) {
                    // Here, N would be best a multiple of 2.
                    _mm_prefetch((const char *)(src + kPrefetchOffset + 64 * 0), kPrefetchHintLevel);
                    if (N >= 3)
                    _mm_prefetch((const char *)(src + kPrefetchOffset + 64 * 1), kPrefetchHintLevel);
                    if (N >= 5)
                    _mm_prefetch((const char *)(src + kPrefetchOffset + 64 * 2), kPrefetchHintLevel);
                    if (N >= 7)
                    _mm_prefetch((const char *)(src + kPrefetchOffset + 64 * 3), kPrefetchHintLevel);
                }

                src += kSingleLoopBytes;

                if (N >= 0)
                    _mm256_store_si256((__m256i *)(dest + 32 * 0), ymm0);
                if (N >= 2)
                    _mm256_store_si256((__m256i *)(dest + 32 * 1), ymm1);
                if (N >= 3)
                    _mm256_store_si256((__m256i *)(dest + 32 * 2), ymm2);
                if (N >= 4)
                    _mm256_store_si256((__m256i *)(dest + 32 * 3), ymm3);
                if (N >= 5)
                    _mm256_store_si256((__m256i *)(dest + 32 * 4), ymm4);
                if (N >= 6)
                    _mm256_store_si256((__m256i *)(dest + 32 * 5), ymm5);
                if (N >= 7)
                    _mm256_store_si256((__m256i *)(dest + 32 * 6), ymm6);
                // Use "{" and "}" to avoid the gcc warnings
                if (N >= 8) {
                    _mm256_store_si256((__m256i *)(dest + 32 * 7), ymm7);
                }

                dest += kSingleLoopBytes;
            }

            avx_forward_move_N_tailing<T, kSrcIsNotAligned, kDestIsAligned, _N - 1>(dest, src, end);
        } else {
            while (src < limit) {
                __m256i ymm0, ymm1, ymm2, ymm3, ymm4, ymm5, ymm6, ymm7;
                if (N >= 0)
                    ymm0 = _mm256_load_si256((const __m256i *)(src + 32 * 0));
                if (N >= 2)
                    ymm1 = _mm256_load_si256((const __m256i *)(src + 32 * 1));
                if (N >= 3)
                    ymm2 = _mm256_load_si256((const __m256i *)(src + 32 * 2));
                if (N >= 4)
                    ymm3 = _mm256_load_si256((const __m256i *)(src + 32 * 3));
                if (N >= 5)
                    ymm4 = _mm256_load_si256((const __m256i *)(src + 32 * 4));
                if (N >= 6)
                    ymm5 = _mm256_load_si256((const __m256i *)(src + 32 * 5));
                if (N >= 7)
                    ymm6 = _mm256_load_si256((const __m256i *)(src + 32 * 6));
                // Use "{" and "}" to avoid the gcc warnings
                if (N >= 8) {
                    ymm7 = _mm256_load_si256((const __m256i *)(src + 32 * 7));
                }

                if (kUsePrefetchHint) {
                    // Here, N would be best a multiple of 2.
                    _mm_prefetch((const char *)(src + kPrefetchOffset + 64 * 0), kPrefetchHintLevel);
                    if (N >= 3)
                    _mm_prefetch((const char *)(src + kPrefetchOffset + 64 * 1), kPrefetchHintLevel);
                    if (N >= 5)
                    _mm_prefetch((const char *)(src + kPrefetchOffset + 64 * 2), kPrefetchHintLevel);
                    if (N >= 7)
                    _mm_prefetch((const char *)(src + kPrefetchOffset + 64 * 3), kPrefetchHintLevel);
                }

                src += kSingleLoopBytes;

                if (N >= 0)
                    _mm256_stream_si256((__m256i *)(dest + 32 * 0), ymm0);
                if (N >= 2)
                    _mm256_stream_si256((__m256i *)(dest + 32 * 1), ymm1);
                if (N >= 3)
                    _mm256_stream_si256((__m256i *)(dest + 32 * 2), ymm2);
                if (N >= 4)
                    _mm256_stream_si256((__m256i *)(dest + 32 * 3), ymm3);
                if (N >= 5)
                    _mm256_stream_si256((__m256i *)(dest + 32 * 4), ymm4);
                if (N >= 6)
                    _mm256_stream_si256((__m256i *)(dest + 32 * 5), ymm5);
                if (N >= 7)
                    _mm256_stream_si256((__m256i *)(dest + 32 * 6), ymm6);
                // Use "{" and "}" to avoid the gcc warnings
                if (N >= 8) {
                    _mm256_stream_si256((__m256i *)(dest + 32 * 7), ymm7);
                }

                dest += kSingleLoopBytes;
            }

            avx_forward_move_N_tailing_nt<T, kSrcIsAligned, kDestIsAligned, _N - 1>(dest, src, end);

            _mm_sfence();
        }
    }
    else {
        bool srcAddrCanAlign = false;
        if (kValueSizeIsDivisible) {
            std::size_t srcUnalignedBytes = (std::size_t)mid & kAVXAlignMask;
            if (kValueSize < kAVXRegBytes)
                srcAddrCanAlign = ((srcUnalignedBytes % kValueSize) == 0);
            else
                srcAddrCanAlign = (srcUnalignedBytes == 0);

            if (srcAddrCanAlign) {
                std::size_t srcPaddingBytes = (kAVXRegBytes - srcUnalignedBytes) & kAVXAlignMask;
                while (srcPaddingBytes != 0) {
                    *first++ = *mid++;
                    srcPaddingBytes -= kValueSize;
                }
            }
        }

        char * JSTD_RESTRICT dest = (char * JSTD_RESTRICT)first;
        char * JSTD_RESTRICT src = (char * JSTD_RESTRICT)mid;
        char * JSTD_RESTRICT end = (char * JSTD_RESTRICT)last;

        std::size_t totalMoveBytes = (last - mid) * kValueSize;
        std::size_t unalignedMoveBytes = (std::size_t)totalMoveBytes % kSingleLoopBytes;
        const char * JSTD_RESTRICT limit = (totalMoveBytes >= kSingleLoopBytes) ? (end - unalignedMoveBytes) : src;

        if (likely(srcAddrCanAlign)) {
            while (src < limit) {
                __m256i ymm0, ymm1, ymm2, ymm3, ymm4, ymm5, ymm6, ymm7;
                if (N >= 0)
                    ymm0 = _mm256_load_si256((const __m256i *)(src + 32 * 0));
                if (N >= 2)
                    ymm1 = _mm256_load_si256((const __m256i *)(src + 32 * 1));
                if (N >= 3)
                    ymm2 = _mm256_load_si256((const __m256i *)(src + 32 * 2));
                if (N >= 4)
                    ymm3 = _mm256_load_si256((const __m256i *)(src + 32 * 3));
                if (N >= 5)
                    ymm4 = _mm256_load_si256((const __m256i *)(src + 32 * 4));
                if (N >= 6)
                    ymm5 = _mm256_load_si256((const __m256i *)(src + 32 * 5));
                if (N >= 7)
                    ymm6 = _mm256_load_si256((const __m256i *)(src + 32 * 6));
                // Use "{" and "}" to avoid the gcc warnings
                if (N >= 8) {
                    ymm7 = _mm256_load_si256((const __m256i *)(src + 32 * 7));
                }

                if (kUsePrefetchHint) {
                    // Here, N would be best a multiple of 2.
                    _mm_prefetch((const char *)(src + kPrefetchOffset + 64 * 0), kPrefetchHintLevel);
                    if (N >= 3)
                    _mm_prefetch((const char *)(src + kPrefetchOffset + 64 * 1), kPrefetchHintLevel);
                    if (N >= 5)
                    _mm_prefetch((const char *)(src + kPrefetchOffset + 64 * 2), kPrefetchHintLevel);
                    if (N >= 7)
                    _mm_prefetch((const char *)(src + kPrefetchOffset + 64 * 3), kPrefetchHintLevel);
                }

                src += kSingleLoopBytes;

                if (N >= 0)
                    _mm256_storeu_si256((__m256i *)(dest + 32 * 0), ymm0);
                if (N >= 2)
                    _mm256_storeu_si256((__m256i *)(dest + 32 * 1), ymm1);
                if (N >= 3)
                    _mm256_storeu_si256((__m256i *)(dest + 32 * 2), ymm2);
                if (N >= 4)
                    _mm256_storeu_si256((__m256i *)(dest + 32 * 3), ymm3);
                if (N >= 5)
                    _mm256_storeu_si256((__m256i *)(dest + 32 * 4), ymm4);
                if (N >= 6)
                    _mm256_storeu_si256((__m256i *)(dest + 32 * 5), ymm5);
                if (N >= 7)
                    _mm256_storeu_si256((__m256i *)(dest + 32 * 6), ymm6);
                // Use "{" and "}" to avoid the gcc warnings
                if (N >= 8) {
                    _mm256_storeu_si256((__m256i *)(dest + 32 * 7), ymm7);
                }

                dest += kSingleLoopBytes;
            }

            avx_forward_move_N_tailing<T, kSrcIsAligned, kDestIsNotAligned, _N - 1>(dest, src, end);
        } else {
            while (src < limit) {
                __m256i ymm0, ymm1, ymm2, ymm3, ymm4, ymm5, ymm6, ymm7;
                if (N >= 0)
                    ymm0 = _mm256_loadu_si256((const __m256i *)(src + 32 * 0));
                if (N >= 2)
                    ymm1 = _mm256_loadu_si256((const __m256i *)(src + 32 * 1));
                if (N >= 3)
                    ymm2 = _mm256_loadu_si256((const __m256i *)(src + 32 * 2));
                if (N >= 4)
                    ymm3 = _mm256_loadu_si256((const __m256i *)(src + 32 * 3));
                if (N >= 5)
                    ymm4 = _mm256_loadu_si256((const __m256i *)(src + 32 * 4));
                if (N >= 6)
                    ymm5 = _mm256_loadu_si256((const __m256i *)(src + 32 * 5));
                if (N >= 7)
                    ymm6 = _mm256_loadu_si256((const __m256i *)(src + 32 * 6));
                // Use "{" and "}" to avoid the gcc warnings
                if (N >= 8) {
                    ymm7 = _mm256_loadu_si256((const __m256i *)(src + 32 * 7));
                }

                if (kUsePrefetchHint) {
                    // Here, N would be best a multiple of 2.
                    _mm_prefetch((const char *)(src + kPrefetchOffset + 64 * 0), kPrefetchHintLevel);
                    if (N >= 3)
                    _mm_prefetch((const char *)(src + kPrefetchOffset + 64 * 1), kPrefetchHintLevel);
                    if (N >= 5)
                    _mm_prefetch((const char *)(src + kPrefetchOffset + 64 * 2), kPrefetchHintLevel);
                    if (N >= 7)
                    _mm_prefetch((const char *)(src + kPrefetchOffset + 64 * 3), kPrefetchHintLevel);
                }

                src += kSingleLoopBytes;

                if (N >= 0)
                    _mm256_storeu_si256((__m256i *)(dest + 32 * 0), ymm0);
                if (N >= 2)
                    _mm256_storeu_si256((__m256i *)(dest + 32 * 1), ymm1);
                if (N >= 3)
                    _mm256_storeu_si256((__m256i *)(dest + 32 * 2), ymm2);
                if (N >= 4)
                    _mm256_storeu_si256((__m256i *)(dest + 32 * 3), ymm3);
                if (N >= 5)
                    _mm256_storeu_si256((__m256i *)(dest + 32 * 4), ymm4);
                if (N >= 6)
                    _mm256_storeu_si256((__m256i *)(dest + 32 * 5), ymm5);
                if (N >= 7)
                    _mm256_storeu_si256((__m256i *)(dest + 32 * 6), ymm6);
                // Use "{" and "}" to avoid the gcc warnings
                if (N >= 8) {
                    _mm256_storeu_si256((__m256i *)(dest + 32 * 7), ymm7);
                }

                dest += kSingleLoopBytes;
            }

            avx_forward_move_N_tailing<T, kSrcIsNotAligned, kDestIsNotAligned, _N - 1>(dest, src, end);
        }
    }
}

template <typename T, std::size_t N = 8>
static
JSTD_NO_INLINE
void avx_forward_move_Nx2_load_aligned(T * JSTD_RESTRICT first, T * JSTD_RESTRICT mid, T * JSTD_RESTRICT last)
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

    std::size_t srcUnalignedBytes = (std::size_t)mid & kAVXAlignMask;
    bool srcAddrCanAlign;
    if (kValueSize < kAVXRegBytes)
        srcAddrCanAlign = (kValueSizeIsDivisible && ((srcUnalignedBytes % kValueSize) == 0));
    else
        srcAddrCanAlign = (kValueSizeIsDivisible && (srcUnalignedBytes == 0));

    if (likely(kValueSizeIsDivisible && srcAddrCanAlign)) {
        std::size_t srcPaddingBytes = (kAVXRegBytes - srcUnalignedBytes) & kAVXAlignMask;
        while (srcPaddingBytes != 0) {
            *first++ = *mid++;
            srcPaddingBytes -= kValueSize;
        }

        char * JSTD_RESTRICT dest = (char * JSTD_RESTRICT)first;
        char * JSTD_RESTRICT src = (char * JSTD_RESTRICT)mid;
        char * JSTD_RESTRICT end = (char * JSTD_RESTRICT)last;

        std::size_t totalMoveBytes = (last - mid) * kValueSize;
        std::size_t unalignedMoveBytes = (std::size_t)totalMoveBytes % kSingleLoopBytes;
        const char * JSTD_RESTRICT limit = (totalMoveBytes >= kSingleLoopBytes) ? (end - unalignedMoveBytes) : src;

        bool destAddrIsAligned = (((std::size_t)dest & kAVXAlignMask) == 0);
        if (likely(!destAddrIsAligned)) {
#if defined(JSTD_IS_ICC)
#pragma code_align(64)
#endif
            while (src < limit) {
                __m256i ymm0, ymm1, ymm2, ymm3, ymm4, ymm5, ymm6, ymm7;
                if (N >= 0)
                    ymm0 = _mm256_load_si256((const __m256i *)(src + 32 * 0));
                if (N >= 2)
                    ymm1 = _mm256_load_si256((const __m256i *)(src + 32 * 1));
                if (N >= 3)
                    ymm2 = _mm256_load_si256((const __m256i *)(src + 32 * 2));
                if (N >= 4)
                    ymm3 = _mm256_load_si256((const __m256i *)(src + 32 * 3));
                if (N >= 5)
                    ymm4 = _mm256_load_si256((const __m256i *)(src + 32 * 4));
                if (N >= 6)
                    ymm5 = _mm256_load_si256((const __m256i *)(src + 32 * 5));
                if (N >= 7)
                    ymm6 = _mm256_load_si256((const __m256i *)(src + 32 * 6));
                // Use "{" and "}" to avoid the gcc warnings
                if (N >= 8) {
                    ymm7 = _mm256_load_si256((const __m256i *)(src + 32 * 7));
                }

                //
                // See: https://blog.csdn.net/qq_43401808/article/details/87360789
                //
                if (kUsePrefetchHint) {
                    // Here, N would be best a multiple of 2.
                    _mm_prefetch((const char *)(src + kPrefetchOffset + 64 * 0), kPrefetchHintLevel);
                    if (N >= 3)
                    _mm_prefetch((const char *)(src + kPrefetchOffset + 64 * 1), kPrefetchHintLevel);
                    if (N >= 5)
                    _mm_prefetch((const char *)(src + kPrefetchOffset + 64 * 2), kPrefetchHintLevel);
                    if (N >= 7)
                    _mm_prefetch((const char *)(src + kPrefetchOffset + 64 * 3), kPrefetchHintLevel);
                }

                src += kHalfLoopBytes;

                if (N >= 0)
                    _mm256_storeu_si256((__m256i *)(dest + 32 * 0), ymm0);
                if (N >= 2)
                    _mm256_storeu_si256((__m256i *)(dest + 32 * 1), ymm1);
                if (N >= 3)
                    _mm256_storeu_si256((__m256i *)(dest + 32 * 2), ymm2);
                if (N >= 4)
                    _mm256_storeu_si256((__m256i *)(dest + 32 * 3), ymm3);
                if (N >= 5)
                    _mm256_storeu_si256((__m256i *)(dest + 32 * 4), ymm4);
                if (N >= 6)
                    _mm256_storeu_si256((__m256i *)(dest + 32 * 5), ymm5);
                if (N >= 7)
                    _mm256_storeu_si256((__m256i *)(dest + 32 * 6), ymm6);
                // Use "{" and "}" to avoid the gcc warnings
                if (N >= 8) {
                    _mm256_storeu_si256((__m256i *)(dest + 32 * 7), ymm7);
                }

                dest += kHalfLoopBytes;

                /////////////////////////////// Half loop //////////////////////////////////

                if (N >= 0)
                    ymm0 = _mm256_load_si256((const __m256i *)(src + 32 * 0));
                if (N >= 2)
                    ymm1 = _mm256_load_si256((const __m256i *)(src + 32 * 1));
                if (N >= 3)
                    ymm2 = _mm256_load_si256((const __m256i *)(src + 32 * 2));
                if (N >= 4)
                    ymm3 = _mm256_load_si256((const __m256i *)(src + 32 * 3));
                if (N >= 5)
                    ymm4 = _mm256_load_si256((const __m256i *)(src + 32 * 4));
                if (N >= 6)
                    ymm5 = _mm256_load_si256((const __m256i *)(src + 32 * 5));
                if (N >= 7)
                    ymm6 = _mm256_load_si256((const __m256i *)(src + 32 * 6));
                // Use "{" and "}" to avoid the gcc warnings
                if (N >= 8) {
                    ymm7 = _mm256_load_si256((const __m256i *)(src + 32 * 7));
                }

                //
                // See: https://blog.csdn.net/qq_43401808/article/details/87360789
                //
                if (kUsePrefetchHint) {
                    // Here, N would be best a multiple of 2.
                    _mm_prefetch((const char *)(src + kPrefetchOffset + 64 * 0), kPrefetchHintLevel);
                    if (N >= 3)
                    _mm_prefetch((const char *)(src + kPrefetchOffset + 64 * 1), kPrefetchHintLevel);
                    if (N >= 5)
                    _mm_prefetch((const char *)(src + kPrefetchOffset + 64 * 2), kPrefetchHintLevel);
                    if (N >= 7)
                    _mm_prefetch((const char *)(src + kPrefetchOffset + 64 * 3), kPrefetchHintLevel);
                }

                src += kHalfLoopBytes;

                if (N >= 0)
                    _mm256_storeu_si256((__m256i *)(dest + 32 * 0), ymm0);
                if (N >= 2)
                    _mm256_storeu_si256((__m256i *)(dest + 32 * 1), ymm1);
                if (N >= 3)
                    _mm256_storeu_si256((__m256i *)(dest + 32 * 2), ymm2);
                if (N >= 4)
                    _mm256_storeu_si256((__m256i *)(dest + 32 * 3), ymm3);
                if (N >= 5)
                    _mm256_storeu_si256((__m256i *)(dest + 32 * 4), ymm4);
                if (N >= 6)
                    _mm256_storeu_si256((__m256i *)(dest + 32 * 5), ymm5);
                if (N >= 7)
                    _mm256_storeu_si256((__m256i *)(dest + 32 * 6), ymm6);
                // Use "{" and "}" to avoid the gcc warnings
                if (N >= 8) {
                    _mm256_storeu_si256((__m256i *)(dest + 32 * 7), ymm7);
                }

                dest += kHalfLoopBytes;
            }

            avx_forward_move_N_tailing<T, kSrcIsAligned, kDestIsNotAligned, (_N * 2 - 1)>(dest, src, end);
            //////////////////////////////////////////////////////////////////////////////////////////////////////
        } else {
#if defined(JSTD_IS_ICC)
#pragma code_align(64)
#endif
            while (src < limit) {
                __m256i ymm0, ymm1, ymm2, ymm3, ymm4, ymm5, ymm6, ymm7;
                if (N >= 0)
                    ymm0 = _mm256_load_si256((const __m256i *)(src + 32 * 0));
                if (N >= 2)
                    ymm1 = _mm256_load_si256((const __m256i *)(src + 32 * 1));
                if (N >= 3)
                    ymm2 = _mm256_load_si256((const __m256i *)(src + 32 * 2));
                if (N >= 4)
                    ymm3 = _mm256_load_si256((const __m256i *)(src + 32 * 3));
                if (N >= 5)
                    ymm4 = _mm256_load_si256((const __m256i *)(src + 32 * 4));
                if (N >= 6)
                    ymm5 = _mm256_load_si256((const __m256i *)(src + 32 * 5));
                if (N >= 7)
                    ymm6 = _mm256_load_si256((const __m256i *)(src + 32 * 6));
                // Use "{" and "}" to avoid the gcc warnings
                if (N >= 8) {
                    ymm7 = _mm256_load_si256((const __m256i *)(src + 32 * 7));
                }

                if (kUsePrefetchHint) {
                    // Here, N would be best a multiple of 2.
                    _mm_prefetch((const char *)(src + kPrefetchOffset + 64 * 0), kPrefetchHintLevel);
                    if (N >= 3)
                    _mm_prefetch((const char *)(src + kPrefetchOffset + 64 * 1), kPrefetchHintLevel);
                    if (N >= 5)
                    _mm_prefetch((const char *)(src + kPrefetchOffset + 64 * 2), kPrefetchHintLevel);
                    if (N >= 7)
                    _mm_prefetch((const char *)(src + kPrefetchOffset + 64 * 3), kPrefetchHintLevel);
                }

                src += kHalfLoopBytes;

                if (N >= 0)
                    _mm256_store_si256((__m256i *)(dest + 32 * 0), ymm0);
                if (N >= 2)
                    _mm256_store_si256((__m256i *)(dest + 32 * 1), ymm1);
                if (N >= 3)
                    _mm256_store_si256((__m256i *)(dest + 32 * 2), ymm2);
                if (N >= 4)
                    _mm256_store_si256((__m256i *)(dest + 32 * 3), ymm3);
                if (N >= 5)
                    _mm256_store_si256((__m256i *)(dest + 32 * 4), ymm4);
                if (N >= 6)
                    _mm256_store_si256((__m256i *)(dest + 32 * 5), ymm5);
                if (N >= 7)
                    _mm256_store_si256((__m256i *)(dest + 32 * 6), ymm6);
                // Use "{" and "}" to avoid the gcc warnings
                if (N >= 8) {
                    _mm256_store_si256((__m256i *)(dest + 32 * 7), ymm7);
                }

                dest += kHalfLoopBytes;

                /////////////////////////////// Half loop //////////////////////////////////

                if (N >= 0)
                    ymm0 = _mm256_load_si256((const __m256i *)(src + 32 * 0));
                if (N >= 2)
                    ymm1 = _mm256_load_si256((const __m256i *)(src + 32 * 1));
                if (N >= 3)
                    ymm2 = _mm256_load_si256((const __m256i *)(src + 32 * 2));
                if (N >= 4)
                    ymm3 = _mm256_load_si256((const __m256i *)(src + 32 * 3));
                if (N >= 5)
                    ymm4 = _mm256_load_si256((const __m256i *)(src + 32 * 4));
                if (N >= 6)
                    ymm5 = _mm256_load_si256((const __m256i *)(src + 32 * 5));
                if (N >= 7)
                    ymm6 = _mm256_load_si256((const __m256i *)(src + 32 * 6));
                // Use "{" and "}" to avoid the gcc warnings
                if (N >= 8) {
                    ymm7 = _mm256_load_si256((const __m256i *)(src + 32 * 7));
                }

                if (kUsePrefetchHint) {
                    // Here, N would be best a multiple of 2.
                    _mm_prefetch((const char *)(src + kPrefetchOffset + 64 * 0), kPrefetchHintLevel);
                    if (N >= 3)
                    _mm_prefetch((const char *)(src + kPrefetchOffset + 64 * 1), kPrefetchHintLevel);
                    if (N >= 5)
                    _mm_prefetch((const char *)(src + kPrefetchOffset + 64 * 2), kPrefetchHintLevel);
                    if (N >= 7)
                    _mm_prefetch((const char *)(src + kPrefetchOffset + 64 * 3), kPrefetchHintLevel);
                }

                src += kHalfLoopBytes;

                if (N >= 0)
                    _mm256_store_si256((__m256i *)(dest + 32 * 0), ymm0);
                if (N >= 2)
                    _mm256_store_si256((__m256i *)(dest + 32 * 1), ymm1);
                if (N >= 3)
                    _mm256_store_si256((__m256i *)(dest + 32 * 2), ymm2);
                if (N >= 4)
                    _mm256_store_si256((__m256i *)(dest + 32 * 3), ymm3);
                if (N >= 5)
                    _mm256_store_si256((__m256i *)(dest + 32 * 4), ymm4);
                if (N >= 6)
                    _mm256_store_si256((__m256i *)(dest + 32 * 5), ymm5);
                if (N >= 7)
                    _mm256_store_si256((__m256i *)(dest + 32 * 6), ymm6);
                // Use "{" and "}" to avoid the gcc warnings
                if (N >= 8) {
                    _mm256_store_si256((__m256i *)(dest + 32 * 7), ymm7);
                }

                dest += kHalfLoopBytes;
            }

            avx_forward_move_N_tailing<T, kSrcIsAligned, kDestIsAligned, (_N * 2 - 1)>(dest, src, end);
        }
    }
    else {
        bool destAddrCanAlign = false;
        if (kValueSizeIsDivisible) {
            std::size_t destUnalignedBytes = (std::size_t)first & kAVXAlignMask;
            if (kValueSize < kAVXRegBytes)
                destAddrCanAlign = ((destUnalignedBytes % kValueSize) == 0);
            else
                destAddrCanAlign = (destUnalignedBytes == 0);

            if (destAddrCanAlign) {
                std::size_t destPaddingBytes = (kAVXRegBytes - destUnalignedBytes) & kAVXAlignMask;
                while (destPaddingBytes != 0) {
                    *first++ = *mid++;
                    destPaddingBytes -= kValueSize;
                }
            }
        }

        char * JSTD_RESTRICT dest = (char * JSTD_RESTRICT)first;
        char * JSTD_RESTRICT src = (char * JSTD_RESTRICT)mid;
        char * JSTD_RESTRICT end = (char * JSTD_RESTRICT)last;

        std::size_t totalMoveBytes = (last - mid) * kValueSize;
        std::size_t unalignedMoveBytes = (std::size_t)totalMoveBytes % kSingleLoopBytes;
        const char * JSTD_RESTRICT limit = (totalMoveBytes >= kSingleLoopBytes) ? (end - unalignedMoveBytes) : src;

        if (likely(destAddrCanAlign)) {
#if defined(JSTD_IS_ICC)
#pragma code_align(64)
#endif
            while (src < limit) {
                __m256i ymm0, ymm1, ymm2, ymm3, ymm4, ymm5, ymm6, ymm7;
                if (N >= 0)
                    ymm0 = _mm256_loadu_si256((const __m256i *)(src + 32 * 0));
                if (N >= 2)
                    ymm1 = _mm256_loadu_si256((const __m256i *)(src + 32 * 1));
                if (N >= 3)
                    ymm2 = _mm256_loadu_si256((const __m256i *)(src + 32 * 2));
                if (N >= 4)
                    ymm3 = _mm256_loadu_si256((const __m256i *)(src + 32 * 3));
                if (N >= 5)
                    ymm4 = _mm256_loadu_si256((const __m256i *)(src + 32 * 4));
                if (N >= 6)
                    ymm5 = _mm256_loadu_si256((const __m256i *)(src + 32 * 5));
                if (N >= 7)
                    ymm6 = _mm256_loadu_si256((const __m256i *)(src + 32 * 6));
                // Use "{" and "}" to avoid the gcc warnings
                if (N >= 8) {
                    ymm7 = _mm256_loadu_si256((const __m256i *)(src + 32 * 7));
                }

                if (kUsePrefetchHint) {
                    // Here, N would be best a multiple of 2.
                    _mm_prefetch((const char *)(src + kPrefetchOffset + 64 * 0), kPrefetchHintLevel);
                    if (N >= 3)
                    _mm_prefetch((const char *)(src + kPrefetchOffset + 64 * 1), kPrefetchHintLevel);
                    if (N >= 5)
                    _mm_prefetch((const char *)(src + kPrefetchOffset + 64 * 2), kPrefetchHintLevel);
                    if (N >= 7)
                    _mm_prefetch((const char *)(src + kPrefetchOffset + 64 * 3), kPrefetchHintLevel);
                }

                src += kHalfLoopBytes;

                if (N >= 0)
                    _mm256_store_si256((__m256i *)(dest + 32 * 0), ymm0);
                if (N >= 2)
                    _mm256_store_si256((__m256i *)(dest + 32 * 1), ymm1);
                if (N >= 3)
                    _mm256_store_si256((__m256i *)(dest + 32 * 2), ymm2);
                if (N >= 4)
                    _mm256_store_si256((__m256i *)(dest + 32 * 3), ymm3);
                if (N >= 5)
                    _mm256_store_si256((__m256i *)(dest + 32 * 4), ymm4);
                if (N >= 6)
                    _mm256_store_si256((__m256i *)(dest + 32 * 5), ymm5);
                if (N >= 7)
                    _mm256_store_si256((__m256i *)(dest + 32 * 6), ymm6);
                // Use "{" and "}" to avoid the gcc warnings
                if (N >= 8) {
                    _mm256_store_si256((__m256i *)(dest + 32 * 7), ymm7);
                }

                dest += kHalfLoopBytes;

                /////////////////////////////// Half loop //////////////////////////////////

                if (N >= 0)
                    ymm0 = _mm256_loadu_si256((const __m256i *)(src + 32 * 0));
                if (N >= 2)
                    ymm1 = _mm256_loadu_si256((const __m256i *)(src + 32 * 1));
                if (N >= 3)
                    ymm2 = _mm256_loadu_si256((const __m256i *)(src + 32 * 2));
                if (N >= 4)
                    ymm3 = _mm256_loadu_si256((const __m256i *)(src + 32 * 3));
                if (N >= 5)
                    ymm4 = _mm256_loadu_si256((const __m256i *)(src + 32 * 4));
                if (N >= 6)
                    ymm5 = _mm256_loadu_si256((const __m256i *)(src + 32 * 5));
                if (N >= 7)
                    ymm6 = _mm256_loadu_si256((const __m256i *)(src + 32 * 6));
                // Use "{" and "}" to avoid the gcc warnings
                if (N >= 8) {
                    ymm7 = _mm256_loadu_si256((const __m256i *)(src + 32 * 7));
                }

                if (kUsePrefetchHint) {
                    // Here, N would be best a multiple of 2.
                    _mm_prefetch((const char *)(src + kPrefetchOffset + 64 * 0), kPrefetchHintLevel);
                    if (N >= 3)
                    _mm_prefetch((const char *)(src + kPrefetchOffset + 64 * 1), kPrefetchHintLevel);
                    if (N >= 5)
                    _mm_prefetch((const char *)(src + kPrefetchOffset + 64 * 2), kPrefetchHintLevel);
                    if (N >= 7)
                    _mm_prefetch((const char *)(src + kPrefetchOffset + 64 * 3), kPrefetchHintLevel);
                }

                src += kHalfLoopBytes;

                if (N >= 0)
                    _mm256_store_si256((__m256i *)(dest + 32 * 0), ymm0);
                if (N >= 2)
                    _mm256_store_si256((__m256i *)(dest + 32 * 1), ymm1);
                if (N >= 3)
                    _mm256_store_si256((__m256i *)(dest + 32 * 2), ymm2);
                if (N >= 4)
                    _mm256_store_si256((__m256i *)(dest + 32 * 3), ymm3);
                if (N >= 5)
                    _mm256_store_si256((__m256i *)(dest + 32 * 4), ymm4);
                if (N >= 6)
                    _mm256_store_si256((__m256i *)(dest + 32 * 5), ymm5);
                if (N >= 7)
                    _mm256_store_si256((__m256i *)(dest + 32 * 6), ymm6);
                // Use "{" and "}" to avoid the gcc warnings
                if (N >= 8) {
                    _mm256_store_si256((__m256i *)(dest + 32 * 7), ymm7);
                }

                dest += kHalfLoopBytes;
            }

            avx_forward_move_N_tailing<T, kSrcIsNotAligned, kDestIsAligned, (_N * 2 - 1)>(dest, src, end);
            ////////////////////////////////////////////////////////////////////////////////////////////////
        } else {
#if defined(JSTD_IS_ICC)
#pragma code_align(64)
#endif
            while (src < limit) {
                __m256i ymm0, ymm1, ymm2, ymm3, ymm4, ymm5, ymm6, ymm7;
                if (N >= 0)
                    ymm0 = _mm256_loadu_si256((const __m256i *)(src + 32 * 0));
                if (N >= 2)
                    ymm1 = _mm256_loadu_si256((const __m256i *)(src + 32 * 1));
                if (N >= 3)
                    ymm2 = _mm256_loadu_si256((const __m256i *)(src + 32 * 2));
                if (N >= 4)
                    ymm3 = _mm256_loadu_si256((const __m256i *)(src + 32 * 3));
                if (N >= 5)
                    ymm4 = _mm256_loadu_si256((const __m256i *)(src + 32 * 4));
                if (N >= 6)
                    ymm5 = _mm256_loadu_si256((const __m256i *)(src + 32 * 5));
                if (N >= 7)
                    ymm6 = _mm256_loadu_si256((const __m256i *)(src + 32 * 6));
                // Use "{" and "}" to avoid the gcc warnings
                if (N >= 8) {
                    ymm7 = _mm256_loadu_si256((const __m256i *)(src + 32 * 7));
                }

                if (kUsePrefetchHint) {
                    // Here, N would be best a multiple of 2.
                    _mm_prefetch((const char *)(src + kPrefetchOffset + 64 * 0), kPrefetchHintLevel);
                    if (N >= 3)
                    _mm_prefetch((const char *)(src + kPrefetchOffset + 64 * 1), kPrefetchHintLevel);
                    if (N >= 5)
                    _mm_prefetch((const char *)(src + kPrefetchOffset + 64 * 2), kPrefetchHintLevel);
                    if (N >= 7)
                    _mm_prefetch((const char *)(src + kPrefetchOffset + 64 * 3), kPrefetchHintLevel);
                }

                src += kHalfLoopBytes;

                if (N >= 0)
                    _mm256_storeu_si256((__m256i *)(dest + 32 * 0), ymm0);
                if (N >= 2)
                    _mm256_storeu_si256((__m256i *)(dest + 32 * 1), ymm1);
                if (N >= 3)
                    _mm256_storeu_si256((__m256i *)(dest + 32 * 2), ymm2);
                if (N >= 4)
                    _mm256_storeu_si256((__m256i *)(dest + 32 * 3), ymm3);
                if (N >= 5)
                    _mm256_storeu_si256((__m256i *)(dest + 32 * 4), ymm4);
                if (N >= 6)
                    _mm256_storeu_si256((__m256i *)(dest + 32 * 5), ymm5);
                if (N >= 7)
                    _mm256_storeu_si256((__m256i *)(dest + 32 * 6), ymm6);
                // Use "{" and "}" to avoid the gcc warnings
                if (N >= 8) {
                    _mm256_storeu_si256((__m256i *)(dest + 32 * 7), ymm7);
                }

                dest += kHalfLoopBytes;

                /////////////////////////////// Half loop //////////////////////////////////

                if (N >= 0)
                    ymm0 = _mm256_loadu_si256((const __m256i *)(src + 32 * 0));
                if (N >= 2)
                    ymm1 = _mm256_loadu_si256((const __m256i *)(src + 32 * 1));
                if (N >= 3)
                    ymm2 = _mm256_loadu_si256((const __m256i *)(src + 32 * 2));
                if (N >= 4)
                    ymm3 = _mm256_loadu_si256((const __m256i *)(src + 32 * 3));
                if (N >= 5)
                    ymm4 = _mm256_loadu_si256((const __m256i *)(src + 32 * 4));
                if (N >= 6)
                    ymm5 = _mm256_loadu_si256((const __m256i *)(src + 32 * 5));
                if (N >= 7)
                    ymm6 = _mm256_loadu_si256((const __m256i *)(src + 32 * 6));
                // Use "{" and "}" to avoid the gcc warnings
                if (N >= 8) {
                    ymm7 = _mm256_loadu_si256((const __m256i *)(src + 32 * 7));
                }

                if (kUsePrefetchHint) {
                    // Here, N would be best a multiple of 2.
                    _mm_prefetch((const char *)(src + kPrefetchOffset + 64 * 0), kPrefetchHintLevel);
                    if (N >= 3)
                    _mm_prefetch((const char *)(src + kPrefetchOffset + 64 * 1), kPrefetchHintLevel);
                    if (N >= 5)
                    _mm_prefetch((const char *)(src + kPrefetchOffset + 64 * 2), kPrefetchHintLevel);
                    if (N >= 7)
                    _mm_prefetch((const char *)(src + kPrefetchOffset + 64 * 3), kPrefetchHintLevel);
                }

                src += kHalfLoopBytes;

                if (N >= 0)
                    _mm256_storeu_si256((__m256i *)(dest + 32 * 0), ymm0);
                if (N >= 2)
                    _mm256_storeu_si256((__m256i *)(dest + 32 * 1), ymm1);
                if (N >= 3)
                    _mm256_storeu_si256((__m256i *)(dest + 32 * 2), ymm2);
                if (N >= 4)
                    _mm256_storeu_si256((__m256i *)(dest + 32 * 3), ymm3);
                if (N >= 5)
                    _mm256_storeu_si256((__m256i *)(dest + 32 * 4), ymm4);
                if (N >= 6)
                    _mm256_storeu_si256((__m256i *)(dest + 32 * 5), ymm5);
                if (N >= 7)
                    _mm256_storeu_si256((__m256i *)(dest + 32 * 6), ymm6);
                // Use "{" and "}" to avoid the gcc warnings
                if (N >= 8) {
                    _mm256_storeu_si256((__m256i *)(dest + 32 * 7), ymm7);
                }

                dest += kHalfLoopBytes;
            }

            avx_forward_move_N_tailing<T, kSrcIsNotAligned, kDestIsNotAligned, (_N * 2 - 1)>(dest, src, end);
        }
    }
}

template <typename T, std::size_t N = 8>
static
JSTD_NO_INLINE
void avx_forward_move_Nx2_store_aligned(T * JSTD_RESTRICT first, T * JSTD_RESTRICT mid, T * JSTD_RESTRICT last)
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

    std::size_t destUnalignedBytes = (std::size_t)first & kAVXAlignMask;
    bool destAddrCanAlign;
    if (kValueSize < kAVXRegBytes)
        destAddrCanAlign = (kValueSizeIsDivisible && ((destUnalignedBytes % kValueSize) == 0));
    else
        destAddrCanAlign = (kValueSizeIsDivisible && (destUnalignedBytes == 0));

    if (likely(kValueSizeIsDivisible && destAddrCanAlign)) {
        std::size_t destPaddingBytes = (kAVXRegBytes - destUnalignedBytes) & kAVXAlignMask;
        while (destPaddingBytes != 0) {
            *first++ = *mid++;
            destPaddingBytes -= kValueSize;
        }

        char * JSTD_RESTRICT dest = (char * JSTD_RESTRICT)first;
        char * JSTD_RESTRICT src = (char * JSTD_RESTRICT)mid;
        char * JSTD_RESTRICT end = (char * JSTD_RESTRICT)last;

        std::size_t totalMoveBytes = (last - mid) * kValueSize;
        std::size_t unalignedMoveBytes = (std::size_t)totalMoveBytes % kSingleLoopBytes;
        const char * JSTD_RESTRICT limit = (totalMoveBytes >= kSingleLoopBytes) ? (end - unalignedMoveBytes) : src;

        bool srcAddrIsAligned = (((std::size_t)src & kAVXAlignMask) == 0);
        if (likely(!srcAddrIsAligned)) {
#if defined(JSTD_IS_ICC)
#pragma code_align(64)
#endif
            while (src < limit) {
                __m256i ymm0, ymm1, ymm2, ymm3, ymm4, ymm5, ymm6, ymm7;
                if (N >= 0)
                    ymm0 = _mm256_loadu_si256((const __m256i *)(src + 32 * 0));
                if (N >= 2)
                    ymm1 = _mm256_loadu_si256((const __m256i *)(src + 32 * 1));
                if (N >= 3)
                    ymm2 = _mm256_loadu_si256((const __m256i *)(src + 32 * 2));
                if (N >= 4)
                    ymm3 = _mm256_loadu_si256((const __m256i *)(src + 32 * 3));
                if (N >= 5)
                    ymm4 = _mm256_loadu_si256((const __m256i *)(src + 32 * 4));
                if (N >= 6)
                    ymm5 = _mm256_loadu_si256((const __m256i *)(src + 32 * 5));
                if (N >= 7)
                    ymm6 = _mm256_loadu_si256((const __m256i *)(src + 32 * 6));
                // Use "{" and "}" to avoid the gcc warnings
                if (N >= 8) {
                    ymm7 = _mm256_loadu_si256((const __m256i *)(src + 32 * 7));
                }

                //
                // See: https://blog.csdn.net/qq_43401808/article/details/87360789
                //
                if (kUsePrefetchHint) {
                    // Here, N would be best a multiple of 2.
                    _mm_prefetch((const char *)(src + kPrefetchOffset + 64 * 0), kPrefetchHintLevel);
                    if (N >= 3)
                    _mm_prefetch((const char *)(src + kPrefetchOffset + 64 * 1), kPrefetchHintLevel);
                    if (N >= 5)
                    _mm_prefetch((const char *)(src + kPrefetchOffset + 64 * 2), kPrefetchHintLevel);
                    if (N >= 7)
                    _mm_prefetch((const char *)(src + kPrefetchOffset + 64 * 3), kPrefetchHintLevel);
                }

                src += kHalfLoopBytes;

                if (N >= 0)
                    _mm256_store_si256((__m256i *)(dest + 32 * 0), ymm0);
                if (N >= 2)
                    _mm256_store_si256((__m256i *)(dest + 32 * 1), ymm1);
                if (N >= 3)
                    _mm256_store_si256((__m256i *)(dest + 32 * 2), ymm2);
                if (N >= 4)
                    _mm256_store_si256((__m256i *)(dest + 32 * 3), ymm3);
                if (N >= 5)
                    _mm256_store_si256((__m256i *)(dest + 32 * 4), ymm4);
                if (N >= 6)
                    _mm256_store_si256((__m256i *)(dest + 32 * 5), ymm5);
                if (N >= 7)
                    _mm256_store_si256((__m256i *)(dest + 32 * 6), ymm6);
                // Use "{" and "}" to avoid the gcc warnings
                if (N >= 8) {
                    _mm256_store_si256((__m256i *)(dest + 32 * 7), ymm7);
                }

                dest += kHalfLoopBytes;

                /////////////////////////////// Half loop //////////////////////////////////

                if (N >= 0)
                    ymm0 = _mm256_loadu_si256((const __m256i *)(src + 32 * 0));
                if (N >= 2)
                    ymm1 = _mm256_loadu_si256((const __m256i *)(src + 32 * 1));
                if (N >= 3)
                    ymm2 = _mm256_loadu_si256((const __m256i *)(src + 32 * 2));
                if (N >= 4)
                    ymm3 = _mm256_loadu_si256((const __m256i *)(src + 32 * 3));
                if (N >= 5)
                    ymm4 = _mm256_loadu_si256((const __m256i *)(src + 32 * 4));
                if (N >= 6)
                    ymm5 = _mm256_loadu_si256((const __m256i *)(src + 32 * 5));
                if (N >= 7)
                    ymm6 = _mm256_loadu_si256((const __m256i *)(src + 32 * 6));
                // Use "{" and "}" to avoid the gcc warnings
                if (N >= 8) {
                    ymm7 = _mm256_loadu_si256((const __m256i *)(src + 32 * 7));
                }

                if (kUsePrefetchHint) {
                    // Here, N would be best a multiple of 2.
                    _mm_prefetch((const char *)(src + kPrefetchOffset + 64 * 0), kPrefetchHintLevel);
                    if (N >= 3)
                    _mm_prefetch((const char *)(src + kPrefetchOffset + 64 * 1), kPrefetchHintLevel);
                    if (N >= 5)
                    _mm_prefetch((const char *)(src + kPrefetchOffset + 64 * 2), kPrefetchHintLevel);
                    if (N >= 7)
                    _mm_prefetch((const char *)(src + kPrefetchOffset + 64 * 3), kPrefetchHintLevel);
                }

                src += kHalfLoopBytes;

                if (N >= 0)
                    _mm256_store_si256((__m256i *)(dest + 32 * 0), ymm0);
                if (N >= 2)
                    _mm256_store_si256((__m256i *)(dest + 32 * 1), ymm1);
                if (N >= 3)
                    _mm256_store_si256((__m256i *)(dest + 32 * 2), ymm2);
                if (N >= 4)
                    _mm256_store_si256((__m256i *)(dest + 32 * 3), ymm3);
                if (N >= 5)
                    _mm256_store_si256((__m256i *)(dest + 32 * 4), ymm4);
                if (N >= 6)
                    _mm256_store_si256((__m256i *)(dest + 32 * 5), ymm5);
                if (N >= 7)
                    _mm256_store_si256((__m256i *)(dest + 32 * 6), ymm6);
                // Use "{" and "}" to avoid the gcc warnings
                if (N >= 8) {
                    _mm256_store_si256((__m256i *)(dest + 32 * 7), ymm7);
                }

                dest += kHalfLoopBytes;
            }

            avx_forward_move_N_tailing<T, kSrcIsNotAligned, kDestIsAligned, (_N * 2 - 1)>(dest, src, end);
            //////////////////////////////////////////////////////////////////////////////////////////////////////
        } else {
#if defined(JSTD_IS_ICC)
#pragma code_align(64)
#endif
            while (src < limit) {
                __m256i ymm0, ymm1, ymm2, ymm3, ymm4, ymm5, ymm6, ymm7;
                if (N >= 0)
                    ymm0 = _mm256_load_si256((const __m256i *)(src + 32 * 0));
                if (N >= 2)
                    ymm1 = _mm256_load_si256((const __m256i *)(src + 32 * 1));
                if (N >= 3)
                    ymm2 = _mm256_load_si256((const __m256i *)(src + 32 * 2));
                if (N >= 4)
                    ymm3 = _mm256_load_si256((const __m256i *)(src + 32 * 3));
                if (N >= 5)
                    ymm4 = _mm256_load_si256((const __m256i *)(src + 32 * 4));
                if (N >= 6)
                    ymm5 = _mm256_load_si256((const __m256i *)(src + 32 * 5));
                if (N >= 7)
                    ymm6 = _mm256_load_si256((const __m256i *)(src + 32 * 6));
                // Use "{" and "}" to avoid the gcc warnings
                if (N >= 8) {
                    ymm7 = _mm256_load_si256((const __m256i *)(src + 32 * 7));
                }

                if (kUsePrefetchHint) {
                    // Here, N would be best a multiple of 2.
                    _mm_prefetch((const char *)(src + kPrefetchOffset + 64 * 0), kPrefetchHintLevel);
                    if (N >= 3)
                    _mm_prefetch((const char *)(src + kPrefetchOffset + 64 * 1), kPrefetchHintLevel);
                    if (N >= 5)
                    _mm_prefetch((const char *)(src + kPrefetchOffset + 64 * 2), kPrefetchHintLevel);
                    if (N >= 7)
                    _mm_prefetch((const char *)(src + kPrefetchOffset + 64 * 3), kPrefetchHintLevel);
                }

                src += kHalfLoopBytes;

                if (N >= 0)
                    _mm256_store_si256((__m256i *)(dest + 32 * 0), ymm0);
                if (N >= 2)
                    _mm256_store_si256((__m256i *)(dest + 32 * 1), ymm1);
                if (N >= 3)
                    _mm256_store_si256((__m256i *)(dest + 32 * 2), ymm2);
                if (N >= 4)
                    _mm256_store_si256((__m256i *)(dest + 32 * 3), ymm3);
                if (N >= 5)
                    _mm256_store_si256((__m256i *)(dest + 32 * 4), ymm4);
                if (N >= 6)
                    _mm256_store_si256((__m256i *)(dest + 32 * 5), ymm5);
                if (N >= 7)
                    _mm256_store_si256((__m256i *)(dest + 32 * 6), ymm6);
                // Use "{" and "}" to avoid the gcc warnings
                if (N >= 8) {
                    _mm256_store_si256((__m256i *)(dest + 32 * 7), ymm7);
                }

                dest += kHalfLoopBytes;

                /////////////////////////////// Half loop //////////////////////////////////

                if (N >= 0)
                    ymm0 = _mm256_load_si256((const __m256i *)(src + 32 * 0));
                if (N >= 2)
                    ymm1 = _mm256_load_si256((const __m256i *)(src + 32 * 1));
                if (N >= 3)
                    ymm2 = _mm256_load_si256((const __m256i *)(src + 32 * 2));
                if (N >= 4)
                    ymm3 = _mm256_load_si256((const __m256i *)(src + 32 * 3));
                if (N >= 5)
                    ymm4 = _mm256_load_si256((const __m256i *)(src + 32 * 4));
                if (N >= 6)
                    ymm5 = _mm256_load_si256((const __m256i *)(src + 32 * 5));
                if (N >= 7)
                    ymm6 = _mm256_load_si256((const __m256i *)(src + 32 * 6));
                // Use "{" and "}" to avoid the gcc warnings
                if (N >= 8) {
                    ymm7 = _mm256_load_si256((const __m256i *)(src + 32 * 7));
                }

                if (kUsePrefetchHint) {
                    // Here, N would be best a multiple of 2.
                    _mm_prefetch((const char *)(src + kPrefetchOffset + 64 * 0), kPrefetchHintLevel);
                    if (N >= 3)
                    _mm_prefetch((const char *)(src + kPrefetchOffset + 64 * 1), kPrefetchHintLevel);
                    if (N >= 5)
                    _mm_prefetch((const char *)(src + kPrefetchOffset + 64 * 2), kPrefetchHintLevel);
                    if (N >= 7)
                    _mm_prefetch((const char *)(src + kPrefetchOffset + 64 * 3), kPrefetchHintLevel);
                }

                src += kHalfLoopBytes;

                if (N >= 0)
                    _mm256_store_si256((__m256i *)(dest + 32 * 0), ymm0);
                if (N >= 2)
                    _mm256_store_si256((__m256i *)(dest + 32 * 1), ymm1);
                if (N >= 3)
                    _mm256_store_si256((__m256i *)(dest + 32 * 2), ymm2);
                if (N >= 4)
                    _mm256_store_si256((__m256i *)(dest + 32 * 3), ymm3);
                if (N >= 5)
                    _mm256_store_si256((__m256i *)(dest + 32 * 4), ymm4);
                if (N >= 6)
                    _mm256_store_si256((__m256i *)(dest + 32 * 5), ymm5);
                if (N >= 7)
                    _mm256_store_si256((__m256i *)(dest + 32 * 6), ymm6);
                // Use "{" and "}" to avoid the gcc warnings
                if (N >= 8) {
                    _mm256_store_si256((__m256i *)(dest + 32 * 7), ymm7);
                }

                dest += kHalfLoopBytes;
            }

            avx_forward_move_N_tailing<T, kSrcIsAligned, kDestIsAligned, (_N * 2 - 1)>(dest, src, end);
        }
    }
    else {
        bool srcAddrCanAlign = false;
        if (kValueSizeIsDivisible) {
            std::size_t srcUnalignedBytes = (std::size_t)mid & kAVXAlignMask;
            if (kValueSize < kAVXRegBytes)
                srcAddrCanAlign = ((srcUnalignedBytes % kValueSize) == 0);
            else
                srcAddrCanAlign = (srcUnalignedBytes == 0);

            if (srcAddrCanAlign) {
                std::size_t srcPaddingBytes = (kAVXRegBytes - srcUnalignedBytes) & kAVXAlignMask;
                while (srcPaddingBytes != 0) {
                    *first++ = *mid++;
                    srcPaddingBytes -= kValueSize;
                }
            }
        }

        char * JSTD_RESTRICT dest = (char * JSTD_RESTRICT)first;
        char * JSTD_RESTRICT src = (char * JSTD_RESTRICT)mid;
        char * JSTD_RESTRICT end = (char * JSTD_RESTRICT)last;

        std::size_t totalMoveBytes = (last - mid) * kValueSize;
        std::size_t unalignedMoveBytes = (std::size_t)totalMoveBytes % kSingleLoopBytes;
        const char * JSTD_RESTRICT limit = (totalMoveBytes >= kSingleLoopBytes) ? (end - unalignedMoveBytes) : src;

        if (likely(srcAddrCanAlign)) {
#if defined(JSTD_IS_ICC)
#pragma code_align(64)
#endif
            while (src < limit) {
                __m256i ymm0, ymm1, ymm2, ymm3, ymm4, ymm5, ymm6, ymm7;
                if (N >= 0)
                    ymm0 = _mm256_load_si256((const __m256i *)(src + 32 * 0));
                if (N >= 2)
                    ymm1 = _mm256_load_si256((const __m256i *)(src + 32 * 1));
                if (N >= 3)
                    ymm2 = _mm256_load_si256((const __m256i *)(src + 32 * 2));
                if (N >= 4)
                    ymm3 = _mm256_load_si256((const __m256i *)(src + 32 * 3));
                if (N >= 5)
                    ymm4 = _mm256_load_si256((const __m256i *)(src + 32 * 4));
                if (N >= 6)
                    ymm5 = _mm256_load_si256((const __m256i *)(src + 32 * 5));
                if (N >= 7)
                    ymm6 = _mm256_load_si256((const __m256i *)(src + 32 * 6));
                // Use "{" and "}" to avoid the gcc warnings
                if (N >= 8) {
                    ymm7 = _mm256_load_si256((const __m256i *)(src + 32 * 7));
                }

                if (kUsePrefetchHint) {
                    // Here, N would be best a multiple of 2.
                    _mm_prefetch((const char *)(src + kPrefetchOffset + 64 * 0), kPrefetchHintLevel);
                    if (N >= 3)
                    _mm_prefetch((const char *)(src + kPrefetchOffset + 64 * 1), kPrefetchHintLevel);
                    if (N >= 5)
                    _mm_prefetch((const char *)(src + kPrefetchOffset + 64 * 2), kPrefetchHintLevel);
                    if (N >= 7)
                    _mm_prefetch((const char *)(src + kPrefetchOffset + 64 * 3), kPrefetchHintLevel);
                }

                src += kHalfLoopBytes;

                if (N >= 0)
                    _mm256_storeu_si256((__m256i *)(dest + 32 * 0), ymm0);
                if (N >= 2)
                    _mm256_storeu_si256((__m256i *)(dest + 32 * 1), ymm1);
                if (N >= 3)
                    _mm256_storeu_si256((__m256i *)(dest + 32 * 2), ymm2);
                if (N >= 4)
                    _mm256_storeu_si256((__m256i *)(dest + 32 * 3), ymm3);
                if (N >= 5)
                    _mm256_storeu_si256((__m256i *)(dest + 32 * 4), ymm4);
                if (N >= 6)
                    _mm256_storeu_si256((__m256i *)(dest + 32 * 5), ymm5);
                if (N >= 7)
                    _mm256_storeu_si256((__m256i *)(dest + 32 * 6), ymm6);
                // Use "{" and "}" to avoid the gcc warnings
                if (N >= 8) {
                    _mm256_storeu_si256((__m256i *)(dest + 32 * 7), ymm7);
                }

                dest += kHalfLoopBytes;

                /////////////////////////////// Half loop //////////////////////////////////

                if (N >= 0)
                    ymm0 = _mm256_load_si256((const __m256i *)(src + 32 * 0));
                if (N >= 2)
                    ymm1 = _mm256_load_si256((const __m256i *)(src + 32 * 1));
                if (N >= 3)
                    ymm2 = _mm256_load_si256((const __m256i *)(src + 32 * 2));
                if (N >= 4)
                    ymm3 = _mm256_load_si256((const __m256i *)(src + 32 * 3));
                if (N >= 5)
                    ymm4 = _mm256_load_si256((const __m256i *)(src + 32 * 4));
                if (N >= 6)
                    ymm5 = _mm256_load_si256((const __m256i *)(src + 32 * 5));
                if (N >= 7)
                    ymm6 = _mm256_load_si256((const __m256i *)(src + 32 * 6));
                // Use "{" and "}" to avoid the gcc warnings
                if (N >= 8) {
                    ymm7 = _mm256_load_si256((const __m256i *)(src + 32 * 7));
                }

                if (kUsePrefetchHint) {
                    // Here, N would be best a multiple of 2.
                    _mm_prefetch((const char *)(src + kPrefetchOffset + 64 * 0), kPrefetchHintLevel);
                    if (N >= 3)
                    _mm_prefetch((const char *)(src + kPrefetchOffset + 64 * 1), kPrefetchHintLevel);
                    if (N >= 5)
                    _mm_prefetch((const char *)(src + kPrefetchOffset + 64 * 2), kPrefetchHintLevel);
                    if (N >= 7)
                    _mm_prefetch((const char *)(src + kPrefetchOffset + 64 * 3), kPrefetchHintLevel);
                }

                src += kHalfLoopBytes;

                if (N >= 0)
                    _mm256_storeu_si256((__m256i *)(dest + 32 * 0), ymm0);
                if (N >= 2)
                    _mm256_storeu_si256((__m256i *)(dest + 32 * 1), ymm1);
                if (N >= 3)
                    _mm256_storeu_si256((__m256i *)(dest + 32 * 2), ymm2);
                if (N >= 4)
                    _mm256_storeu_si256((__m256i *)(dest + 32 * 3), ymm3);
                if (N >= 5)
                    _mm256_storeu_si256((__m256i *)(dest + 32 * 4), ymm4);
                if (N >= 6)
                    _mm256_storeu_si256((__m256i *)(dest + 32 * 5), ymm5);
                if (N >= 7)
                    _mm256_storeu_si256((__m256i *)(dest + 32 * 6), ymm6);
                // Use "{" and "}" to avoid the gcc warnings
                if (N >= 8) {
                    _mm256_storeu_si256((__m256i *)(dest + 32 * 7), ymm7);
                }

                dest += kHalfLoopBytes;
            }

            avx_forward_move_N_tailing<T, kSrcIsAligned, kDestIsNotAligned, (_N * 2 - 1)>(dest, src, end);
            ////////////////////////////////////////////////////////////////////////////////////////////////
        } else {
#if defined(JSTD_IS_ICC)
#pragma code_align(64)
#endif
            while (src < limit) {
                __m256i ymm0, ymm1, ymm2, ymm3, ymm4, ymm5, ymm6, ymm7;
                if (N >= 0)
                    ymm0 = _mm256_loadu_si256((const __m256i *)(src + 32 * 0));
                if (N >= 2)
                    ymm1 = _mm256_loadu_si256((const __m256i *)(src + 32 * 1));
                if (N >= 3)
                    ymm2 = _mm256_loadu_si256((const __m256i *)(src + 32 * 2));
                if (N >= 4)
                    ymm3 = _mm256_loadu_si256((const __m256i *)(src + 32 * 3));
                if (N >= 5)
                    ymm4 = _mm256_loadu_si256((const __m256i *)(src + 32 * 4));
                if (N >= 6)
                    ymm5 = _mm256_loadu_si256((const __m256i *)(src + 32 * 5));
                if (N >= 7)
                    ymm6 = _mm256_loadu_si256((const __m256i *)(src + 32 * 6));
                // Use "{" and "}" to avoid the gcc warnings
                if (N >= 8) {
                    ymm7 = _mm256_loadu_si256((const __m256i *)(src + 32 * 7));
                }

                if (kUsePrefetchHint) {
                    // Here, N would be best a multiple of 2.
                    _mm_prefetch((const char *)(src + kPrefetchOffset + 64 * 0), kPrefetchHintLevel);
                    if (N >= 3)
                    _mm_prefetch((const char *)(src + kPrefetchOffset + 64 * 1), kPrefetchHintLevel);
                    if (N >= 5)
                    _mm_prefetch((const char *)(src + kPrefetchOffset + 64 * 2), kPrefetchHintLevel);
                    if (N >= 7)
                    _mm_prefetch((const char *)(src + kPrefetchOffset + 64 * 3), kPrefetchHintLevel);
                }

                src += kHalfLoopBytes;

                if (N >= 0)
                    _mm256_storeu_si256((__m256i *)(dest + 32 * 0), ymm0);
                if (N >= 2)
                    _mm256_storeu_si256((__m256i *)(dest + 32 * 1), ymm1);
                if (N >= 3)
                    _mm256_storeu_si256((__m256i *)(dest + 32 * 2), ymm2);
                if (N >= 4)
                    _mm256_storeu_si256((__m256i *)(dest + 32 * 3), ymm3);
                if (N >= 5)
                    _mm256_storeu_si256((__m256i *)(dest + 32 * 4), ymm4);
                if (N >= 6)
                    _mm256_storeu_si256((__m256i *)(dest + 32 * 5), ymm5);
                if (N >= 7)
                    _mm256_storeu_si256((__m256i *)(dest + 32 * 6), ymm6);
                // Use "{" and "}" to avoid the gcc warnings
                if (N >= 8) {
                    _mm256_storeu_si256((__m256i *)(dest + 32 * 7), ymm7);
                }

                dest += kHalfLoopBytes;

                /////////////////////////////// Half loop //////////////////////////////////

                if (N >= 0)
                    ymm0 = _mm256_loadu_si256((const __m256i *)(src + 32 * 0));
                if (N >= 2)
                    ymm1 = _mm256_loadu_si256((const __m256i *)(src + 32 * 1));
                if (N >= 3)
                    ymm2 = _mm256_loadu_si256((const __m256i *)(src + 32 * 2));
                if (N >= 4)
                    ymm3 = _mm256_loadu_si256((const __m256i *)(src + 32 * 3));
                if (N >= 5)
                    ymm4 = _mm256_loadu_si256((const __m256i *)(src + 32 * 4));
                if (N >= 6)
                    ymm5 = _mm256_loadu_si256((const __m256i *)(src + 32 * 5));
                if (N >= 7)
                    ymm6 = _mm256_loadu_si256((const __m256i *)(src + 32 * 6));
                // Use "{" and "}" to avoid the gcc warnings
                if (N >= 8) {
                    ymm7 = _mm256_loadu_si256((const __m256i *)(src + 32 * 7));
                }

                if (kUsePrefetchHint) {
                    // Here, N would be best a multiple of 2.
                    _mm_prefetch((const char *)(src + kPrefetchOffset + 64 * 0), kPrefetchHintLevel);
                    if (N >= 3)
                    _mm_prefetch((const char *)(src + kPrefetchOffset + 64 * 1), kPrefetchHintLevel);
                    if (N >= 5)
                    _mm_prefetch((const char *)(src + kPrefetchOffset + 64 * 2), kPrefetchHintLevel);
                    if (N >= 7)
                    _mm_prefetch((const char *)(src + kPrefetchOffset + 64 * 3), kPrefetchHintLevel);
                }

                src += kHalfLoopBytes;

                if (N >= 0)
                    _mm256_storeu_si256((__m256i *)(dest + 32 * 0), ymm0);
                if (N >= 2)
                    _mm256_storeu_si256((__m256i *)(dest + 32 * 1), ymm1);
                if (N >= 3)
                    _mm256_storeu_si256((__m256i *)(dest + 32 * 2), ymm2);
                if (N >= 4)
                    _mm256_storeu_si256((__m256i *)(dest + 32 * 3), ymm3);
                if (N >= 5)
                    _mm256_storeu_si256((__m256i *)(dest + 32 * 4), ymm4);
                if (N >= 6)
                    _mm256_storeu_si256((__m256i *)(dest + 32 * 5), ymm5);
                if (N >= 7)
                    _mm256_storeu_si256((__m256i *)(dest + 32 * 6), ymm6);
                // Use "{" and "}" to avoid the gcc warnings
                if (N >= 8) {
                    _mm256_storeu_si256((__m256i *)(dest + 32 * 7), ymm7);
                }

                dest += kHalfLoopBytes;
            }

            avx_forward_move_N_tailing<T, kSrcIsNotAligned, kDestIsNotAligned, (_N * 2 - 1)>(dest, src, end);
        }
    }
}

#if defined(JSTD_IS_PURE_GCC)
#pragma GCC pop_options
#endif

template <typename T, std::size_t index>
JSTD_NO_INLINE
void _mm_storeu_last(__m128i * addr, __m128i src, std::size_t left_len)
{
    uint8_t * dest = (uint8_t *)addr;
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
            src = _mm_xor_si128(src, src);
            assert(false);
            break;
        case 1:
            value32_0 = (uint32_t)_mm_extract_epi32(src, 0);
            *dest = uint8_t(value32_0 & 0xFFu);
            break;
        case 2:
            value32_0 = (uint32_t)_mm_extract_epi16(src, 0);
            *(uint16_t *)(dest + 0) = value32_0;
            break;
        case 3:
            value32_0 = (uint32_t)_mm_extract_epi32(src, 0);
            *(uint16_t *)(dest + 0) = uint16_t(value32_0 & 0xFFFFu);
            *(uint8_t  *)(dest + 2) = uint8_t(value32_0 >> 16u);
            break;
        case 4:
            value32_0 = (uint32_t)_mm_extract_epi32(src, 0);
            *(uint32_t *)(dest + 0) = value32_0;
            break;
        case 5:
            value32_0 = (uint32_t)_mm_extract_epi32(src, 0);
            value32_1 = (uint32_t)_mm_extract_epi32(src, 1);
            *(uint32_t *)(dest + 0) = value32_0;
            *(uint8_t  *)(dest + 4) = uint8_t(value32_1 & 0xFFu);
            break;
        case 6:
            value32_0 = (uint32_t)_mm_extract_epi32(src, 0);
            value32_1 = (uint32_t)_mm_extract_epi16(src, 2);
            *(uint32_t *)(dest + 0) = value32_0;
            *(uint16_t *)(dest + 4) = value32_1;
            break;
        case 7:
            value32_0 = (uint32_t)_mm_extract_epi32(src, 0);
            value32_1 = (uint32_t)_mm_extract_epi32(src, 1);
            *(uint32_t *)(dest + 0) = value32_0;
            *(uint16_t *)(dest + 4) = uint16_t(value32_1 & 0xFFFFu);
            *(uint8_t  *)(dest + 6) = uint8_t(value32_1 >> 16u);
            break;
        case 8:
            value64_0 = (uint64_t)_mm_extract_epi64(src, 0);
            *(uint64_t *)(dest + 0) = value64_0;
            break;
        case 9:
            value64_0 = (uint64_t)_mm_extract_epi64(src, 0);
            value32_0 = (uint32_t)_mm_extract_epi32(src, 2);
            *(uint64_t *)(dest + 0) = value64_0;
            *(uint8_t  *)(dest + 8) = uint8_t(value32_0 & 0xFFu);
            break;
        case 10:
            value64_0 = (uint64_t)_mm_extract_epi64(src, 0);
            value32_0 = (uint32_t)_mm_extract_epi16(src, 2);
            *(uint64_t *)(dest + 0) = value64_0;
            *(uint16_t *)(dest + 8) = uint16_t(value32_0 & 0xFFFFu);
            break;
        case 11:
            value64_0 = (uint64_t)_mm_extract_epi64(src, 0);
            value32_0 = (uint32_t)_mm_extract_epi32(src, 2);
            *(uint64_t *)(dest + 0) = value64_0;
            *(uint16_t *)(dest + 8) = uint16_t(value32_0 & 0xFFFFu);
            *(uint8_t  *)(dest + 10) = uint8_t(value32_0 >> 16u);
            break;
        case 12:
            value64_0 = (uint64_t)_mm_extract_epi64(src, 0);
            value32_0 = (uint32_t)_mm_extract_epi32(src, 2);
            *(uint64_t *)(dest + 0) = value64_0;
            *(uint32_t *)(dest + 8) = value32_0;
            break;
        case 13:
            value64_0 = (uint64_t)_mm_extract_epi64(src, 0);
            value64_1 = (uint64_t)_mm_extract_epi64(src, 1);
            *(uint64_t *)(dest + 0) = value64_0;
            *(uint32_t *)(dest + 8) = (uint32_t)(value64_1 & 0xFFFFFFFFu);
            *(uint8_t  *)(dest + 12) = (uint8_t)((value64_1 >> 32u) & 0xFFu);
            break;
        case 14:
            value64_0 = (uint64_t)_mm_extract_epi64(src, 0);
            value64_1 = (uint64_t)_mm_extract_epi64(src, 1);
            *(uint64_t *)(dest + 0) = value64_0;
            *(uint32_t *)(dest + 8) = (uint32_t)(value64_1 & 0xFFFFFFFFu);
            *(uint16_t *)(dest + 12) = (uint16_t)((value64_1 >> 32u) & 0xFFFFu);
            break;
        case 15:
            value64_0 = (uint64_t)_mm_extract_epi64(src, 0);
            value64_1 = (uint64_t)_mm_extract_epi64(src, 1);
            *(uint64_t *)(dest + 0) = value64_0;
            *(uint32_t *)(dest + 8) = (uint32_t)(value64_1 & 0xFFFFFFFFu);
            *(uint16_t *)(dest + 12) = (uint16_t)((value64_1 >> 32u) & 0xFFFFu);
            *(uint8_t  *)(dest + 14) = (uint8_t)((value64_1 >> 48u) & 0xFFu);
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
JSTD_NO_INLINE
void _mm256_storeu_last(__m256i * addr, __m256i src, std::size_t left_len)
{
    uint8_t * dest = (uint8_t *)addr;
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
            src = _mm256_xor_si256(src, src);
            assert(false);
            break;
        case 1:
            value32_0 = (uint32_t)AVX::mm256_extract_epi32<0>(src);
            *dest = uint8_t(value32_0 & 0xFFu);
            break;
        case 2:
            value32_0 = (uint32_t)AVX::mm256_extract_epi16<0>(src);
            *(uint16_t *)(dest + 0) = value32_0;
            break;
        case 3:
            value32_0 = (uint32_t)AVX::mm256_extract_epi32<0>(src);
            *(uint16_t *)(dest + 0) = uint16_t(value32_0 & 0xFFFFu);
            *(uint8_t  *)(dest + 2) = uint8_t(value32_0 >> 16u);
            break;
        case 4:
            value32_0 = (uint32_t)AVX::mm256_extract_epi32<0>(src);
            *(uint32_t *)(dest + 0) = value32_0;
            break;
        case 5:
            value32_0 = (uint32_t)AVX::mm256_extract_epi32<0>(src);
            value32_1 = (uint32_t)AVX::mm256_extract_epi32<1>(src);
            *(uint32_t *)(dest + 0) = value32_0;
            *(uint8_t  *)(dest + 4) = uint8_t(value32_1 & 0xFFu);
            break;
        case 6:
            value32_0 = (uint32_t)AVX::mm256_extract_epi32<0>(src);
            value32_1 = (uint32_t)AVX::mm256_extract_epi16<2>(src);
            *(uint32_t *)(dest + 0) = value32_0;
            *(uint16_t *)(dest + 4) = value32_1;
            break;
        case 7:
            value32_0 = (uint32_t)AVX::mm256_extract_epi32<0>(src);
            value32_1 = (uint32_t)AVX::mm256_extract_epi32<1>(src);
            *(uint32_t *)(dest + 0) = value32_0;
            *(uint16_t *)(dest + 4) = uint16_t(value32_1 & 0xFFFFu);
            *(uint8_t  *)(dest + 6) = uint8_t(value32_1 >> 16u);
            break;
        case 8:
            value64_0 = (uint64_t)AVX::mm256_extract_epi64<0>(src);
            *(uint64_t *)(dest + 0) = value64_0;
            break;
        case 9:
            value64_0 = (uint64_t)AVX::mm256_extract_epi64<0>(src);
            value32_0 = (uint32_t)AVX::mm256_extract_epi32<2>(src);
            *(uint64_t *)(dest + 0) = value64_0;
            *(uint8_t  *)(dest + 8) = uint8_t(value32_0 & 0xFFu);
            break;
        case 10:
            value64_0 = (uint64_t)AVX::mm256_extract_epi64<0>(src);
            value32_0 = (uint32_t)AVX::mm256_extract_epi16<4>(src);
            *(uint64_t *)(dest + 0) = value64_0;
            *(uint16_t *)(dest + 8) = uint16_t(value32_0 & 0xFFFFu);
            break;
        case 11:
            value64_0 = (uint64_t)AVX::mm256_extract_epi64<0>(src);
            value32_0 = (uint32_t)AVX::mm256_extract_epi32<2>(src);
            *(uint64_t *)(dest + 0) = value64_0;
            *(uint16_t *)(dest + 8) = uint16_t(value32_0 & 0xFFFFu);
            *(uint8_t  *)(dest + 10) = uint8_t(value32_0 >> 16u);
            break;
        case 12:
            value64_0 = (uint64_t)AVX::mm256_extract_epi64<0>(src);
            value32_0 = (uint32_t)AVX::mm256_extract_epi32<2>(src);
            *(uint64_t *)(dest + 0) = value64_0;
            *(uint32_t *)(dest + 8) = value32_0;
            break;
        case 13:
            value64_0 = (uint64_t)AVX::mm256_extract_epi64<0>(src);
            value64_1 = (uint64_t)AVX::mm256_extract_epi64<1>(src);
            *(uint64_t *)(dest + 0) = value64_0;
            *(uint32_t *)(dest + 8) = (uint32_t)(value64_1 & 0xFFFFFFFFu);
            *(uint8_t  *)(dest + 12) = (uint8_t)((value64_1 >> 32u) & 0xFFu);
            break;
        case 14:
            value64_0 = (uint64_t)AVX::mm256_extract_epi64<0>(src);
            value64_1 = (uint64_t)AVX::mm256_extract_epi64<1>(src);
            *(uint64_t *)(dest + 0) = value64_0;
            *(uint32_t *)(dest + 8) = (uint32_t)(value64_1 & 0xFFFFFFFFu);
            *(uint16_t *)(dest + 12) = (uint16_t)((value64_1 >> 32u) & 0xFFFFu);
            break;
        case 15:
            value64_0 = (uint64_t)AVX::mm256_extract_epi64<0>(src);
            value64_1 = (uint64_t)AVX::mm256_extract_epi64<1>(src);
            *(uint64_t *)(dest + 0) = value64_0;
            *(uint32_t *)(dest + 8) = (uint32_t)(value64_1 & 0xFFFFFFFFu);
            *(uint16_t *)(dest + 12) = (uint16_t)((value64_1 >> 32u) & 0xFFFFu);
            *(uint8_t  *)(dest + 14) = (uint8_t)((value64_1 >> 48u) & 0xFFu);
            break;
        case 16:
            value64_0 = (uint64_t)AVX::mm256_extract_epi64<0>(src);
            value64_1 = (uint64_t)AVX::mm256_extract_epi64<1>(src);
            *(uint64_t *)(dest + 0) = value64_0;
            *(uint64_t *)(dest + 8) = value64_1;
            break;
        case 17:
            value64_0 = (uint64_t)AVX::mm256_extract_epi64<0>(src);
            value64_1 = (uint64_t)AVX::mm256_extract_epi64<1>(src);
            value32_0 = (uint32_t)AVX::mm256_extract_epi32<4>(src);

            *(uint64_t *)(dest + 0) = value64_0;
            *(uint64_t *)(dest + 8) = value64_1;
            *(uint8_t  *)(dest + 16) = uint8_t(value32_0 & 0xFFu);
            break;
        case 18:
            value64_0 = (uint64_t)AVX::mm256_extract_epi64<0>(src);
            value64_1 = (uint64_t)AVX::mm256_extract_epi64<1>(src);
            value32_0 = (uint32_t)AVX::mm256_extract_epi16<8>(src);

            *(uint64_t *)(dest + 0) = value64_0;
            *(uint64_t *)(dest + 8) = value64_1;
            *(uint16_t *)(dest + 16) = uint16_t(value32_0);
            break;
        case 19:
            value64_0 = (uint64_t)AVX::mm256_extract_epi64<0>(src);
            value64_1 = (uint64_t)AVX::mm256_extract_epi64<1>(src);
            value32_0 = (uint32_t)AVX::mm256_extract_epi32<4>(src);

            *(uint64_t *)(dest + 0) = value64_0;
            *(uint64_t *)(dest + 8) = value64_1;
            *(uint16_t *)(dest + 16) = uint16_t(value32_0 & 0xFFFFu);
            *(uint8_t  *)(dest + 18) = uint8_t((value32_0 >> 16u) & 0xFFu);
            break;
        case 20:
            value64_0 = (uint64_t)AVX::mm256_extract_epi64<0>(src);
            value64_1 = (uint64_t)AVX::mm256_extract_epi64<1>(src);
            value32_0 = (uint32_t)AVX::mm256_extract_epi32<4>(src);

            *(uint64_t *)(dest + 0) = value64_0;
            *(uint64_t *)(dest + 8) = value64_1;
            *(uint32_t *)(dest + 16) = value32_0;
            break;
        case 21:
            value64_0 = (uint64_t)AVX::mm256_extract_epi64<0>(src);
            value64_1 = (uint64_t)AVX::mm256_extract_epi64<1>(src);
            value32_0 = (uint32_t)AVX::mm256_extract_epi32<4>(src);
            value32_1 = (uint32_t)AVX::mm256_extract_epi32<5>(src);

            *(uint64_t *)(dest + 0) = value64_0;
            *(uint64_t *)(dest + 8) = value64_1;
            *(uint32_t *)(dest + 16) = value32_0;
            *(uint8_t  *)(dest + 20) = uint8_t(value32_1 & 0xFFu);
            break;
        case 22:
            value64_0 = (uint64_t)AVX::mm256_extract_epi64<0>(src);
            value64_1 = (uint64_t)AVX::mm256_extract_epi64<1>(src);
            value32_0 = (uint32_t)AVX::mm256_extract_epi32<4>(src);
            value32_1 = (uint32_t)AVX::mm256_extract_epi16<10>(src);

            *(uint64_t *)(dest + 0) = value64_0;
            *(uint64_t *)(dest + 8) = value64_1;
            *(uint32_t *)(dest + 16) = value32_0;
            *(uint16_t *)(dest + 20) = uint16_t(value32_1);
            break;
        case 23:
            value64_0 = (uint64_t)AVX::mm256_extract_epi64<0>(src);
            value64_1 = (uint64_t)AVX::mm256_extract_epi64<1>(src);
            value32_0 = (uint32_t)AVX::mm256_extract_epi32<4>(src);
            value32_1 = (uint32_t)AVX::mm256_extract_epi32<5>(src);

            *(uint64_t *)(dest + 0) = value64_0;
            *(uint64_t *)(dest + 8) = value64_1;
            *(uint32_t *)(dest + 16) = value32_0;
            *(uint16_t *)(dest + 20) = uint16_t(value32_1 & 0xFFFFu);
            *(uint8_t  *)(dest + 22) = uint8_t((value32_1 >> 16u) & 0xFFu);
            break;
        case 24:
            value64_0 = (uint64_t)AVX::mm256_extract_epi64<0>(src);
            value64_1 = (uint64_t)AVX::mm256_extract_epi64<1>(src);
            value64_2 = (uint64_t)AVX::mm256_extract_epi64<2>(src);

            *(uint64_t *)(dest + 0)  = value64_0;
            *(uint64_t *)(dest + 8)  = value64_1;
            *(uint64_t *)(dest + 16) = value64_2;
            break;
        case 25:
            value64_0 = (uint64_t)AVX::mm256_extract_epi64<0>(src);
            value64_1 = (uint64_t)AVX::mm256_extract_epi64<1>(src);
            value64_2 = (uint64_t)AVX::mm256_extract_epi64<2>(src);
            value32_0 = (uint32_t)AVX::mm256_extract_epi32<6>(src);

            *(uint64_t *)(dest + 0)  = value64_0;
            *(uint64_t *)(dest + 8)  = value64_1;
            *(uint64_t *)(dest + 16) = value64_2;
            *(uint8_t  *)(dest + 24) = uint8_t(value32_0 & 0xFFu);
            break;
        case 26:
            value64_0 = (uint64_t)AVX::mm256_extract_epi64<0>(src);
            value64_1 = (uint64_t)AVX::mm256_extract_epi64<1>(src);
            value64_2 = (uint64_t)AVX::mm256_extract_epi64<2>(src);
            value32_0 = (uint32_t)AVX::mm256_extract_epi16<12>(src);

            *(uint64_t *)(dest + 0)  = value64_0;
            *(uint64_t *)(dest + 8)  = value64_1;
            *(uint64_t *)(dest + 16) = value64_2;
            *(uint16_t *)(dest + 24) = uint16_t(value32_0);
            break;
        case 27:
            value64_0 = (uint64_t)AVX::mm256_extract_epi64<0>(src);
            value64_1 = (uint64_t)AVX::mm256_extract_epi64<1>(src);
            value64_2 = (uint64_t)AVX::mm256_extract_epi64<2>(src);
            value32_0 = (uint32_t)AVX::mm256_extract_epi32<6>(src);

            *(uint64_t *)(dest + 0)  = value64_0;
            *(uint64_t *)(dest + 8)  = value64_1;
            *(uint64_t *)(dest + 16) = value64_2;
            *(uint16_t *)(dest + 24) = uint16_t(value32_0 & 0xFFFFu);
            *(uint8_t  *)(dest + 26) = uint8_t((value32_0 >> 16u) & 0xFFu);
            break;
        case 28:
            value64_0 = (uint64_t)AVX::mm256_extract_epi64<0>(src);
            value64_1 = (uint64_t)AVX::mm256_extract_epi64<1>(src);
            value64_2 = (uint64_t)AVX::mm256_extract_epi64<2>(src);
            value32_0 = (uint32_t)AVX::mm256_extract_epi32<6>(src);

            *(uint64_t *)(dest + 0)  = value64_0;
            *(uint64_t *)(dest + 8)  = value64_1;
            *(uint64_t *)(dest + 16) = value64_2;
            *(uint32_t *)(dest + 24) = value32_0;
            break;
        case 29:
            value64_0 = (uint64_t)AVX::mm256_extract_epi64<0>(src);
            value64_1 = (uint64_t)AVX::mm256_extract_epi64<1>(src);
            value64_2 = (uint64_t)AVX::mm256_extract_epi64<2>(src);
            value32_0 = (uint32_t)AVX::mm256_extract_epi32<6>(src);
            value32_1 = (uint32_t)AVX::mm256_extract_epi32<7>(src);

            *(uint64_t *)(dest + 0)  = value64_0;
            *(uint64_t *)(dest + 8)  = value64_1;
            *(uint64_t *)(dest + 16) = value64_2;
            *(uint32_t *)(dest + 24) = value32_0;
            *(uint8_t  *)(dest + 28) = uint8_t(value32_1 & 0xFFu);
            break;
        case 30:
            value64_0 = (uint64_t)AVX::mm256_extract_epi64<0>(src);
            value64_1 = (uint64_t)AVX::mm256_extract_epi64<1>(src);
            value64_2 = (uint64_t)AVX::mm256_extract_epi64<2>(src);
            value32_0 = (uint32_t)AVX::mm256_extract_epi32<6>(src);
            value32_1 = (uint32_t)AVX::mm256_extract_epi16<14>(src);

            *(uint64_t *)(dest + 0)  = value64_0;
            *(uint64_t *)(dest + 8)  = value64_1;
            *(uint64_t *)(dest + 16) = value64_2;
            *(uint32_t *)(dest + 24) = value32_0;
            *(uint16_t *)(dest + 28) = uint16_t(value32_1);
            break;
        case 31:
            value64_0 = (uint64_t)AVX::mm256_extract_epi64<0>(src);
            value64_1 = (uint64_t)AVX::mm256_extract_epi64<1>(src);
            value64_2 = (uint64_t)AVX::mm256_extract_epi64<2>(src);
            value32_0 = (uint32_t)AVX::mm256_extract_epi32<6>(src);
            value32_1 = (uint32_t)AVX::mm256_extract_epi32<7>(src);

            *(uint64_t *)(dest + 0)  = value64_0;
            *(uint64_t *)(dest + 8)  = value64_1;
            *(uint64_t *)(dest + 16) = value64_2;
            *(uint32_t *)(dest + 24) = value32_0;
            *(uint16_t *)(dest + 28) = uint16_t(value32_1 & 0xFFFFu);
            *(uint8_t  *)(dest + 30) = uint8_t((value32_1 >> 16u) & 0xFFu);
            break;
        case 32:
            _mm256_storeu_si256(addr, src);
            break;
        default:
            break;
    }
#endif
}

template <typename T>
JSTD_FORCE_INLINE
void left_rotate_sse_1_regs(T * first, T * mid, T * last, std::size_t left_len)
{
    const __m128i * stash_src = (const __m128i *)first;
    __m128i stash0 = _mm_loadu_si128(stash_src);

    avx_forward_move_N_load_aligned<T, 8>(first, mid, last);

    __m128i * stash_dest = (__m128i *)(last - left_len);
    _mm_storeu_last<T, 0>(stash_dest, stash0, left_len);
}

template <typename T, std::size_t N>
JSTD_FORCE_INLINE
void left_rotate_avx_N_regs(T * first, T * mid, T * last, std::size_t left_len)
{
    static const std::size_t kEstimatedSize = (N != 0) ? ((N - 1) * kAVXRegBytes) : 0;

    __m256i stash0, stash1, stash2, stash3, stash4, stash5;
    __m256i stash6, stash7, stash8, stash9, stash10, stash11;

    const __m256i * stash_src = (const __m256i *)first;
    if (N >= 1)
        stash0 = _mm256_loadu_si256(stash_src + 0);
    if (N >= 2)
        stash1 = _mm256_loadu_si256(stash_src + 1);
    if (N >= 3)
        stash2 = _mm256_loadu_si256(stash_src + 2);
    if (N >= 4)
        stash3 = _mm256_loadu_si256(stash_src + 3);
    if (N >= 5)
        stash4 = _mm256_loadu_si256(stash_src + 4);
    if (N >= 6)
        stash5 = _mm256_loadu_si256(stash_src + 5);
    if (N >= 7)
        stash6 = _mm256_loadu_si256(stash_src + 6);
    if (N >= 8)
        stash7 = _mm256_loadu_si256(stash_src + 7);
    if (N >= 9)
        stash8 = _mm256_loadu_si256(stash_src + 8);
    if (N >= 10)
        stash9 = _mm256_loadu_si256(stash_src + 9);
    if (N >= 11)
        stash10 = _mm256_loadu_si256(stash_src + 10);
    // Use "{" and "}" to avoid the gcc warnings
    if (N >= 12) {
        stash11 = _mm256_loadu_si256(stash_src + 11);
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
  #elif 1
    if (N <= 6)         // 1 -- 6,
        avx_forward_move_N_store_aligned<T, 8, kEstimatedSize>(first, mid, last);
    else if (N <= 8)    // 7, 8
        avx_forward_move_N_store_aligned<T, 6, kEstimatedSize>(first, mid, last);
    else                // 9, 10, 11, 12
        avx_forward_move_N_store_aligned<T, 4, kEstimatedSize>(first, mid, last);
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

    __m256i * stash_dest = (__m256i *)(last - left_len);
    if (N == 1)
        _mm256_storeu_last<T, 0>(stash_dest + 0, stash0, left_len);
    if (N > 1)
        _mm256_storeu_si256(stash_dest + 0, stash0);
    if (N == 2)
        _mm256_storeu_last<T, 1>(stash_dest + 1, stash1, left_len);
    if (N > 2)
        _mm256_storeu_si256(stash_dest + 1, stash1);
    if (N == 3)
        _mm256_storeu_last<T, 2>(stash_dest + 2, stash2, left_len);
    if (N > 3)
        _mm256_storeu_si256(stash_dest + 2, stash2);
    if (N == 4)
        _mm256_storeu_last<T, 3>(stash_dest + 3, stash3, left_len);
    if (N > 4)
        _mm256_storeu_si256(stash_dest + 3, stash3);
    if (N == 5)
        _mm256_storeu_last<T, 4>(stash_dest + 4, stash4, left_len);
    if (N > 5)
        _mm256_storeu_si256(stash_dest + 4, stash4);
    if (N == 6)
        _mm256_storeu_last<T, 5>(stash_dest + 5, stash5, left_len);
    if (N > 6)
        _mm256_storeu_si256(stash_dest + 5, stash5);
    if (N == 7)
        _mm256_storeu_last<T, 6>(stash_dest + 6, stash6, left_len);
    if (N > 7)
        _mm256_storeu_si256(stash_dest + 6, stash6);
    if (N == 8)
        _mm256_storeu_last<T, 7>(stash_dest + 7, stash7, left_len);
    if (N > 8)
        _mm256_storeu_si256(stash_dest + 7, stash7);
    if (N == 9)
        _mm256_storeu_last<T, 8>(stash_dest + 8, stash8, left_len);
    if (N > 9)
        _mm256_storeu_si256(stash_dest + 8, stash8);
    if (N == 10)
        _mm256_storeu_last<T, 9>(stash_dest + 9, stash9, left_len);
    if (N > 10)
        _mm256_storeu_si256(stash_dest + 9, stash9);
    if (N == 11)
        _mm256_storeu_last<T, 10>(stash_dest + 10, stash10, left_len);
    if (N > 11)
        _mm256_storeu_si256(stash_dest + 10, stash10);
    if (N == 12)
        _mm256_storeu_last<T, 11>(stash_dest + 11, stash11, left_len);
}

template <typename T>
JSTD_NO_INLINE
T * left_rotate_avx_chunk_swap(T * first, T * mid, T * last, std::size_t left_len, std::size_t right_len)
{
    //typedef T * pointer;
    static const std::size_t kCacheAlignment = kMaxCacheLineSize - 1;
    static const std::size_t kStackChunkSize = 8192;
    static const std::size_t kActualStackChunkSize = kStackChunkSize + kMaxCacheLineSize * 2;

    JSTD_STATIC_ASSERT(((kMaxCacheLineSize & (kMaxCacheLineSize - 1)) == 0),
                       "kMaxCacheLineSize must be power of 2.");

    // Chunk buffer on stack
    char orig_stack_chunk[kActualStackChunkSize];
    // Chunk buffer align to 128 bytes (kMaxCacheLineSize)
    char * stack_chunk = pointer_align_to<kCacheAlignment>(&orig_stack_chunk[0]);

    char * src = (char *)mid;
    char * dest = (char *)first;
    char * end = (char *)last;

    std::size_t left_bytes = left_len * sizeof(T);
    JSTD_ASSERT(left_bytes > kMaxAVXStashBytes);
    if (left_bytes <= kStackChunkSize) {
        //
        avx_mem_copy_forward_store_aligned<T, 8, false, true, kMaxAVXStashBytes>(stack_chunk, dest, left_bytes);
    } else {
        //
    }
    return 0;
}

template <typename T>
JSTD_FORCE_INLINE
T * left_rotate_avx_impl(T * first, T * mid, T * last, std::size_t left_len, std::size_t right_len)
{
    typedef T * pointer;

    std::size_t length = left_len + right_len;
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
            return right_rotate_simple_impl(first, mid, last, left_len, right_len);
        }
        else {
            return right_rotate_simple_impl(first, mid, last, left_len, right_len);
        }
    }

    return result;
}

template <typename T>
inline
T * left_rotate_avx(T * first, T * mid, T * last)
{
    if (kUsePrefetchHint) {
        _mm_prefetch((const char *)first, kPrefetchHintLevel);
        _mm_prefetch((const char *)first + 64, kPrefetchHintLevel);
        _mm_prefetch((const char *)mid, kPrefetchHintLevel);
        _mm_prefetch((const char *)mid + 64, kPrefetchHintLevel);
    }

    // If (first > mid), it's a error under DEBUG mode.
    JSTD_ASSERT_EX((first <= mid), "simd::left_rotate_avx(): Error, first > mid.");

    std::ptrdiff_t s_left_len = mid - first;
    if (first >= mid) return first;

    // If (mid > last), it's a error under DEBUG mode.
    JSTD_ASSERT_EX((mid <= last), "simd::left_rotate_avx(): Error, mid > last.");

    std::ptrdiff_t s_right_len = last - mid;
    if (mid >= last) return last;

    std::size_t left_len = std::size_t(s_left_len);
    std::size_t right_len = std::size_t(s_right_len);

    return left_rotate_avx_impl(first, mid, last, left_len, right_len);
}

template <typename T>
inline
T * left_rotate_avx(T * data, std::size_t length, std::size_t offset)
{
    typedef T * pointer;

    pointer first = data;
    pointer mid   = data + offset;
    pointer last  = data + length;

    if (kUsePrefetchHint) {
        _mm_prefetch((const char *)first, kPrefetchHintLevel);
        _mm_prefetch((const char *)first + 64, kPrefetchHintLevel);
        _mm_prefetch((const char *)mid, kPrefetchHintLevel);
        _mm_prefetch((const char *)mid + 64, kPrefetchHintLevel);
    }

    std::size_t left_len = offset;
    if (left_len == 0) return first;

    // If (offset > length), it's a error under DEBUG mode.
    JSTD_ASSERT_EX((offset <= length), "simd::left_rotate_avx(): Error, offset > length.");

    // If offset is bigger than length, directly return last.
    std::intptr_t s_right_len = length - offset;
    if (offset >= length) return last;

    std::size_t right_len = (std::size_t)(s_right_len);

    return left_rotate_avx_impl(first, mid, last, left_len, right_len);
}

template <typename T>
inline
T * rotate(T * first, T * mid, T * last)
{
    return left_rotate_avx(first, mid, last);
}

template <typename T>
inline
T * rotate(T * data, std::size_t length, std::size_t offset)
{
    return left_rotate_avx(data, length, offset);
}

} // namespace simd
} // namespace jstd

#undef PREFETCH_HINT_LEVEL

#endif // JSTD_ARRAY_ROTATE_SIMD_H
