
#ifndef JSTD_ARRAY_ROTATE_V1_H
#define JSTD_ARRAY_ROTATE_V1_H

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
#pragma once
#endif

#include <cstdint>
#include <cstddef>
#include <cstdbool>
#include <algorithm>

#include "jstd/FastMod.h"

namespace jstd {
namespace v1 {

#if !defined(_MSC_VER)
template <typename RandomAccessIterator>
RandomAccessIterator
left_rotate_impl(RandomAccessIterator first, RandomAccessIterator mid, RandomAccessIterator last);
#endif

template <typename RandomAccessIterator>
RandomAccessIterator
right_rotate_impl(RandomAccessIterator first, RandomAccessIterator mid, RandomAccessIterator last)
{
    if (first == mid) return last;
    if (mid == last) return first;

    const std::size_t length0 = last - first;
    const std::size_t shift0 = last - mid;
    const std::size_t remain0 = length0 % shift0;

    RandomAccessIterator read = mid;
    RandomAccessIterator write = last;

    do {
        std::iter_swap(--read, --write);
    } while (read != first);
    write--;

    // Rotate the remaining sequence into place
    if (remain0 != 0) {
        if (shift0 / 2 <= remain0 || shift0 < 8) {
            std::uint32_t length = (std::uint32_t)shift0;
            std::uint32_t shift = (std::uint32_t)(shift0 - remain0);
            std::uint32_t remain;
            if (shift != 1) {
                read = write - shift;
                remain = fast_mod_u32(length, shift);
                while (read != first) {
                    std::iter_swap(read--, write--);
                }
                std::iter_swap(read, write--);
            
                while (remain != 0) {
                    length = shift;
                    shift = shift - remain;
                    if (true || shift != 1) {
                        read = write - shift;
                        remain = fast_mod_u32(length, shift);
                        while (read != first) {
                            std::iter_swap(read--, write--);
                        }
                        std::iter_swap(read, write--);
                    }
                    else {
                        read = write - shift;
                        while (read != first) {
                            std::iter_swap(read--, write--);
                        }
                        std::iter_swap(read, write--);
                        break;
                    }
                }
            }
            else {
                read = write - shift;
                while (read != first) {
                    std::iter_swap(read--, write--);
                }
                std::iter_swap(read, write--);
            }
        }
        else {
            return left_rotate_impl(read, read + remain0, write + 1);
        }
    }

    return write;
}

template <typename RandomAccessIterator>
inline
RandomAccessIterator
right_rotate(RandomAccessIterator first, RandomAccessIterator mid, RandomAccessIterator last)
{
    if (first == mid) return last;
    if (mid == last) return first;

    return right_rotate_impl(first, mid, last);
}

template <typename RandomAccessIterator>
RandomAccessIterator
left_rotate_impl(RandomAccessIterator first, RandomAccessIterator mid, RandomAccessIterator last)
{
    const std::size_t length0 = last - first;
    const std::size_t shift0 = mid - first;
    const std::size_t remain0 = length0 % shift0;

    RandomAccessIterator read = mid;
    RandomAccessIterator write = first;

    do {
        std::iter_swap(write++, read++);
    } while (read != last);

    // Rotate the remaining sequence into place
    if (remain0 != 0) {
        //std::int32_t dir = (shift0 / 2 <= remain0) ? 1 : -1;
        if (shift0 / 2 <= remain0 || shift0 < 8) {
            std::uint32_t length = (std::uint32_t)shift0;
            std::uint32_t shift = (std::uint32_t)(shift0 - remain0);
            std::uint32_t remain;
            if (shift != 1) {
                read = write + shift;
                remain = fast_mod_u32(length, shift);
                while (read != last) {
                    std::iter_swap(write++, read++);
                }
            
                while (remain != 0) {
                    length = shift;
                    shift = shift - remain;
                    if (true || shift != 1) {
                        read = write + shift;
                        remain = fast_mod_u32(length, shift);
                        while (read != last) {
                            std::iter_swap(write++, read++);
                        }
                    }
                    else {
                        read = write + shift;
                        while (read != last) {
                            std::iter_swap(write++, read++);
                        }
                        break;
                    }
                }
            }
            else {
                read = write + shift;
                while (read != last) {
                    std::iter_swap(write++, read++);
                }
            }
        }
        else {
            return right_rotate_impl(write, last - remain0, last);
        }
    }

    return write;
}

template <typename RandomAccessIterator>
inline
RandomAccessIterator
left_rotate(RandomAccessIterator first, RandomAccessIterator mid, RandomAccessIterator last)
{
    if (first == mid) return last;
    if (mid == last) return first;

    return left_rotate_impl(first, mid, last);
}

template <typename RandomAccessIterator>
inline
RandomAccessIterator
rotate(RandomAccessIterator first, RandomAccessIterator mid, RandomAccessIterator last)
{
    return left_rotate(first, mid, last);
}

} // namespace v1
} // namespace jstd

#endif // JSTD_ARRAY_ROTATE_V1_H
