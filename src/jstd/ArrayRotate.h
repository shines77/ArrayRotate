
#ifndef JSTD_ARRAY_ROTATE_H
#define JSTD_ARRAY_ROTATE_H

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

#include <cstdint>
#include <cstddef>
#include <cstdbool>
#include <algorithm>
#include <type_traits>

#include "jstd/FastMod.h"

#ifndef ROTATE_USE_FAST_MOD
#define ROTATE_USE_FAST_MOD     0
#endif

namespace jstd {

//
// Recursive version, not practical in actual use.
//
// From: https://en.cppreference.com/w/cpp/algorithm/rotate
//
template <typename ForwardIter>
ForwardIter // void until C++11
std_rotate(ForwardIter first, ForwardIter mid, ForwardIter last)
{
    if (first == mid) return last;
    if (mid == last) return first;

    ForwardIter read = mid;
    ForwardIter write = first;
    // Read position for when "read" hits "last"
    ForwardIter next_read = first;

    while (read != last) {
        // Track where "first" went
        if (write == next_read) next_read = read;
        std::iter_swap(write++, read++);
    }

    // Rotate the remaining sequence into place
    (std_rotate)(write, next_read, last);
    return write;
}

template <typename _Integral>
inline
_Integral __gcd(_Integral __x, _Integral __y)
{
    do {
        _Integral __t = __x % __y;
        __x = __y;
        __y = __t;
    } while (__y);
    return __x;
}

//
// When left_len is much smaller than right_len, it's too slow.
//
template <typename _RandomAccessIterator>
_RandomAccessIterator
libgxx_rotate_gcd(_RandomAccessIterator __first, _RandomAccessIterator __middle, _RandomAccessIterator __last)
{
    typedef typename std::iterator_traits<_RandomAccessIterator>::difference_type   difference_type;
    typedef typename std::iterator_traits<_RandomAccessIterator>::value_type        value_type;

    const difference_type __m1 = __middle - __first;
    const difference_type __m2 = __last - __middle;
    if (__m1 == __m2) {
        std::swap_ranges(__first, __middle, __middle);
        return __middle;
    }

    const difference_type __g = __gcd(__m1, __m2);
    for (_RandomAccessIterator __p = __first + __g; __p != __first;) {
        value_type __t(std::move(*--__p));
        _RandomAccessIterator __p1 = __p;
        _RandomAccessIterator __p2 = __p1 + __m1;

        do {
            *__p1 = std::move(*__p2);
            __p1 = __p2;
            const difference_type __d = __last - __p2;
            if (__m1 < __d)
                __p2 += __m1;
            else
                __p2 = __first + (__m1 - __d);
        } while (__p2 != __p);

        *__p1 = std::move(__t);
    }

    return __first + __m2;
}

template <typename RandomAccessIterator>
RandomAccessIterator
right_rotate(RandomAccessIterator first, RandomAccessIterator mid, RandomAccessIterator last)
{
    typedef RandomAccessIterator iterator;
    typedef typename std::iterator_traits<iterator>::difference_type    difference_type;
    typedef typename std::iterator_traits<iterator>::value_type         value_type;

    std::size_t left_len = (std::size_t)difference_type(mid - first);
    if (left_len == 0) return first;

    std::size_t right_len = (std::size_t)difference_type(last - mid);
    if (right_len == 0) return last;

    RandomAccessIterator result = first + right_len;

    do {
        if (right_len <= left_len) {
            RandomAccessIterator read = mid;
            RandomAccessIterator write = last;
            if (right_len != 1) {
                while (read != first) {
                    --write;
                    --read;
                    std::iter_swap(read, write);
                }
#if ROTATE_USE_FAST_MOD
                left_len = fast_mod_u32((std::uint32_t)left_len, (std::uint32_t)right_len);
#else
                left_len %= right_len;
#endif
                last = write;
                right_len -= left_len;
                mid = first + left_len;
                if (left_len == 0 || right_len == 0)
                    break;
            }
            else {
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
        else {
            RandomAccessIterator read = mid;
            RandomAccessIterator write = first;
            if (left_len != 1) {
                while (read != last) {
                    std::iter_swap(write, read);
                    ++write;
                    ++read;
                }
#if ROTATE_USE_FAST_MOD
                right_len = fast_mod_u32((std::uint32_t)right_len, (std::uint32_t)left_len);
#else
                right_len %= left_len;
#endif
                first = write;
                left_len -= right_len;
                mid = last - right_len;
                if (right_len == 0 || left_len == 0)
                    break;
            }
            else {
                value_type tmp(std::move(*write));
                while (read != last) {
                    *write = *read;
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

template <typename RandomAccessIterator>
RandomAccessIterator
left_rotate(RandomAccessIterator first, RandomAccessIterator mid, RandomAccessIterator last)
{
    typedef RandomAccessIterator iterator;
    typedef typename std::iterator_traits<iterator>::difference_type    difference_type;
    typedef typename std::iterator_traits<iterator>::value_type         value_type;

    std::size_t left_len = (std::size_t)difference_type(mid - first);
    if (left_len == 0) return first;

    std::size_t right_len = (std::size_t)difference_type(last - mid);
    if (right_len == 0) return last;

    RandomAccessIterator result = first + right_len;

    do {
        if (left_len <= right_len) {
            RandomAccessIterator read = mid;
            RandomAccessIterator write = first;
            if (left_len != 1) {
                while (read != last) {
                    std::iter_swap(write, read);
                    ++write;
                    ++read;
                }
#if ROTATE_USE_FAST_MOD
                right_len = fast_mod_u32((std::uint32_t)right_len, (std::uint32_t)left_len);
#else
                right_len %= left_len;
#endif
                first = write;
                left_len -= right_len;
                mid = last - right_len;
                if (right_len == 0 || left_len == 0)
                    break;
            }
            else {
                value_type tmp(std::move(*write));
                while (read != last) {
                    *write = *read;
                    ++write;
                    ++read;
                }
                *write = std::move(tmp);
                break;
            }
        }
        else {
            RandomAccessIterator read = mid;
            RandomAccessIterator write = last;
            if (right_len != 1) {
                while (read != first) {
                    --write;
                    --read;
                    std::iter_swap(read, write);
                }
#if ROTATE_USE_FAST_MOD
                left_len = fast_mod_u32((std::uint32_t)left_len, (std::uint32_t)right_len);
#else
                left_len %= right_len;
#endif
                last = write;
                right_len -= left_len;
                mid = first + left_len;
                if (left_len == 0 || right_len == 0)
                    break;
            }
            else {
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

template <typename RandomAccessIterator>
inline
RandomAccessIterator
rotate(RandomAccessIterator first, RandomAccessIterator mid, RandomAccessIterator last)
{
    return left_rotate(first, mid, last);
}

} // namespace jstd

#ifdef ROTATE_USE_FAST_MOD
#undef ROTATE_USE_FAST_MOD
#endif

#endif // JSTD_ARRAY_ROTATE_H
