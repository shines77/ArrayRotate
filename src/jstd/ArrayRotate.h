
#ifndef JSTD_ARRAY_ROTATE_H
#define JSTD_ARRAY_ROTATE_H

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
#pragma once
#endif

#include <cstdint>
#include <cstddef>
#include <cstdbool>
#include <algorithm>
#include <type_traits>

#include "jstd/FastDiv.h"
#include "jstd/FastMod.h"

#define ROTATE_USE_FAST_MOD     1

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

template <typename ForwardIter, typename ItemType = void>
ForwardIter // void until C++11
right_rotate(ForwardIter first, ForwardIter mid, ForwardIter last)
{
    typedef ForwardIter iterator;
    typedef typename std::iterator_traits<iterator>::difference_type    difference_type;
    typedef typename std::iterator_traits<iterator>::value_type         value_type;

    std::size_t left_len = (std::size_t)difference_type(mid - first);
    if (left_len == 0) return first;

    std::size_t right_len = (std::size_t)difference_type(last - mid);
    if (right_len == 0) return last;

    ForwardIter result = first + right_len;

    do {
        if (right_len <= left_len) {
            ForwardIter read = mid;
            ForwardIter write = last;
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
            ForwardIter read = mid;
            ForwardIter write = first;
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

template <typename ForwardIter, typename ItemType = void>
ForwardIter // void until C++11
left_rotate(ForwardIter first, ForwardIter mid, ForwardIter last)
{
    typedef ForwardIter iterator;
    typedef typename std::iterator_traits<iterator>::difference_type    difference_type;
    typedef typename std::iterator_traits<iterator>::value_type         value_type;

    std::size_t left_len = (std::size_t)difference_type(mid - first);
    if (left_len == 0) return first;

    std::size_t right_len = (std::size_t)difference_type(last - mid);
    if (right_len == 0) return last;

    ForwardIter result = first + right_len;

    do {
        if (left_len <= right_len) {
            ForwardIter read = mid;
            ForwardIter write = first;
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
            ForwardIter read = mid;
            ForwardIter write = last;
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

template <typename ForwardIter, typename ItemType = void>
inline
ForwardIter // void until C++11
rotate(ForwardIter first, ForwardIter mid, ForwardIter last)
{
    return left_rotate(first, mid, last);
}

} // namespace jstd

#endif // JSTD_ARRAY_ROTATE_H
