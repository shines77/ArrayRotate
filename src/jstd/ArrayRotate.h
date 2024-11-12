
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

namespace detail {

//
// Recursive version, not practical in actual use.
//
// From: https://en.cppreference.com/w/cpp/algorithm/rotate
//
template <typename ForwardIterator>
ForwardIterator // void until C++11
std_rotate(ForwardIterator first, ForwardIterator middle, ForwardIterator last, std::forward_iterator_tag)
{
    if (first == middle) return last;
    if (middle == last) return first;

    ForwardIterator read = middle;
    ForwardIterator write = first;
    // Read position for when "read" hits "last"
    ForwardIterator next_read = first;

    while (read != last) {
        // Track where "first" went
        if (write == next_read)
            next_read = read;
        std::iter_swap(write++, read++);
    }

    // Rotate the remaining sequence into place
    (detail::std_rotate)(write, next_read, last, std::forward_iterator_tag());
    return write;
}

template <typename RandomAccessIterator>
RandomAccessIterator
std_rotate(RandomAccessIterator first, RandomAccessIterator middle, RandomAccessIterator last, std::random_access_iterator_tag)
{
    return detail::std_rotate(first, middle, last, std::forward_iterator_tag());
}

template <typename BidirectionalIterator >
BidirectionalIterator
std_rotate(BidirectionalIterator  first, BidirectionalIterator  middle, BidirectionalIterator  last, std::bidirectional_iterator_tag)
{
    return detail::std_rotate(first, middle, last, std::forward_iterator_tag());
}

} // namespace detail

template <typename AnyIterator>
AnyIterator // void until C++11
std_rotate(AnyIterator first, AnyIterator middle, AnyIterator last)
{
    typedef typename std::iterator_traits<AnyIterator>::iterator_category iterator_category;
    return detail::std_rotate(first, middle, last, iterator_category());
}

template <typename _Integral>
inline
_Integral __gcd(_Integral __m, _Integral __n)
{
    while (__n != 0) {
        _Integral __t = __m % __n;
        __m = __n;
        __n = __t;
    }
    return __m;
}

//
// When left_len is much smaller than right_len, it's too slow.
//
// See: https://www.zhihu.com/question/35500094
//
template <typename _RandomAccessIterator>
_RandomAccessIterator
libcxx_rotate(_RandomAccessIterator __first, _RandomAccessIterator __middle, _RandomAccessIterator __last)
{
    typedef typename std::iterator_traits<_RandomAccessIterator>::difference_type   difference_type;
    typedef typename std::iterator_traits<_RandomAccessIterator>::value_type        value_type;

    const difference_type __left  = __middle - __first;
    const difference_type __right = __last - __middle;
    if (__left == __right) {
        std::swap_ranges(__first, __middle, __middle);
        return __middle;
    }

    const difference_type __g = __gcd(__left, __right);
    for (_RandomAccessIterator __p = __first + __g; __p != __first;) {
        value_type __t(std::move(*--__p));
        _RandomAccessIterator __p1 = __p;
        _RandomAccessIterator __p2 = __p1 + __left;

        do {
            *__p1 = std::move(*__p2);
            __p1 = __p2;
            const difference_type __d = __last - __p2;
            if (__left < __d)
                __p2 += __left;
            else
                __p2 = __first + (__left - __d);
        } while (__p2 != __p);

        *__p1 = std::move(__t);
    }

    return __first + __right;
}

namespace detail {

template <typename RandomAccessIterator>
RandomAccessIterator
right_rotate(RandomAccessIterator first, RandomAccessIterator middle, RandomAccessIterator last,
             std::random_access_iterator_tag)
{
    typedef RandomAccessIterator iterator;
    typedef typename std::iterator_traits<iterator>::difference_type    difference_type;
    typedef typename std::iterator_traits<iterator>::value_type         value_type;

    std::size_t left_len = (std::size_t)difference_type(middle - first);
    if (left_len == 0) return first;

    std::size_t right_len = (std::size_t)difference_type(last - middle);
    if (right_len == 0) return last;

    RandomAccessIterator result = first + right_len;

    do {
        if (right_len <= left_len) {
            RandomAccessIterator read = middle;
            RandomAccessIterator write = last;
            if (right_len != 1) {
                while (read != first) {
                    --read;
                    --write;
                    std::iter_swap(read, write);
                }
#if ROTATE_USE_FAST_MOD
                left_len = fast_mod_u32((std::uint32_t)left_len, (std::uint32_t)right_len);
#else
                left_len %= right_len;
#endif
                last = write;
                right_len -= left_len;
                middle = first + left_len;
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
            RandomAccessIterator read = middle;
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
                middle = last - right_len;
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

template <typename RandomAccessIterator>
RandomAccessIterator
left_rotate(RandomAccessIterator first, RandomAccessIterator middle, RandomAccessIterator last,
            std::random_access_iterator_tag)
{
    typedef RandomAccessIterator iterator;
    typedef typename std::iterator_traits<iterator>::difference_type    difference_type;
    typedef typename std::iterator_traits<iterator>::value_type         value_type;

    std::size_t left_len = (std::size_t)difference_type(middle - first);
    if (left_len == 0) return first;

    std::size_t right_len = (std::size_t)difference_type(last - middle);
    if (right_len == 0) return last;

    RandomAccessIterator result = first + right_len;

    do {
        if (left_len <= right_len) {
            RandomAccessIterator read = middle;
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
                middle = last - right_len;
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
        else {
            RandomAccessIterator read = middle;
            RandomAccessIterator write = last;
            if (right_len != 1) {
                while (read != first) {
                    --read;
                    --write;
                    std::iter_swap(read, write);
                }
#if ROTATE_USE_FAST_MOD
                left_len = fast_mod_u32((std::uint32_t)left_len, (std::uint32_t)right_len);
#else
                left_len %= right_len;
#endif
                last = write;
                right_len -= left_len;
                middle = first + left_len;
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
    } while (1);

    return result;
}

template <typename RandomAccessIterator>
inline
RandomAccessIterator
rotate(RandomAccessIterator first, RandomAccessIterator middle, RandomAccessIterator last, std::random_access_iterator_tag)
{
    return left_rotate(first, middle, last, std::random_access_iterator_tag());
}

} // namespace detail

template <typename AnyIterator>
AnyIterator // void until C++11
right_rotate(AnyIterator first, AnyIterator middle, AnyIterator last)
{
    typedef typename std::iterator_traits<AnyIterator>::iterator_category iterator_category;
    return detail::right_rotate(first, middle, last, iterator_category());
}

template <typename AnyIterator>
AnyIterator // void until C++11
left_rotate(AnyIterator first, AnyIterator middle, AnyIterator last)
{
    typedef typename std::iterator_traits<AnyIterator>::iterator_category iterator_category;
    return detail::left_rotate(first, middle, last, iterator_category());
}

template <typename AnyIterator>
AnyIterator // void until C++11
rotate(AnyIterator first, AnyIterator middle, AnyIterator last)
{
    typedef typename std::iterator_traits<AnyIterator>::iterator_category iterator_category;
    return detail::rotate(first, middle, last, iterator_category());
}

} // namespace jstd

#ifdef ROTATE_USE_FAST_MOD
#undef ROTATE_USE_FAST_MOD
#endif

#endif // JSTD_ARRAY_ROTATE_H
