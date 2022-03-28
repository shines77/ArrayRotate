
#ifndef JSTD_ARRAY_ROTATE_H
#define JSTD_ARRAY_ROTATE_H

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
#pragma once
#endif

#include <cstdint>
#include <cstddef>
#include <cstdbool>
#include <algorithm>

#include "jstd/FastDiv.h"
#include "jstd/FastMod.h"

namespace jstd {

template <typename ForwardIt>
ForwardIt // void until C++11
std_rotate(ForwardIt first, ForwardIt mid, ForwardIt last)
{
    if (first == mid) return last;
    if (mid == last) return first;

    ForwardIt read = mid;
    ForwardIt write = first;
    // Read position for when "read" hits "last"
    ForwardIt next_read = first;

    while (read != last) {
        // Track where "first" went
        if (write == next_read) next_read = read;
        std::iter_swap(write++, read++);
    }

    // Rotate the remaining sequence into place
    (std_rotate)(write, next_read, last);
    return write;
}

#if !defined(_MSC_VER)
template <typename ForwardIt, typename ItemType = void>
ForwardIt // void until C++11
left_rotate_impl(ForwardIt first, ForwardIt mid, ForwardIt last);
#endif

template <typename ForwardIt, typename ItemType = void>
ForwardIt // void until C++11
right_rotate_impl(ForwardIt first, ForwardIt mid, ForwardIt last)
{
    if (first == mid) return last;
    if (mid == last) return first;

    const std::size_t length0 = last - first;
    const std::size_t shift0 = last - mid;
    const std::size_t remain0 = length0 % shift0;

    ForwardIt read = mid;
    ForwardIt write = last;

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

template <typename ForwardIt, typename ItemType = void>
inline
ForwardIt // void until C++11
right_rotate(ForwardIt first, ForwardIt mid, ForwardIt last)
{
    if (first == mid) return last;
    if (mid == last) return first;

    return right_rotate_impl(first, mid, last);
}

template <typename ForwardIt, typename ItemType = void>
ForwardIt // void until C++11
left_rotate_impl(ForwardIt first, ForwardIt mid, ForwardIt last)
{
    const std::size_t length0 = last - first;
    const std::size_t shift0 = mid - first;
    const std::size_t remain0 = length0 % shift0;

    ForwardIt read = mid;
    ForwardIt write = first;

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

template <typename ForwardIt, typename ItemType = void>
inline
ForwardIt // void until C++11
left_rotate(ForwardIt first, ForwardIt mid, ForwardIt last)
{
    if (first == mid) return last;
    if (mid == last) return first;

    return left_rotate_impl(first, mid, last);
}

template <typename ForwardIt, typename ItemType = void>
inline
ForwardIt // void until C++11
rotate(ForwardIt first, ForwardIt mid, ForwardIt last)
{
    return left_rotate(first, mid, last);
}

} // namespace jstd

#endif // JSTD_ARRAY_ROTATE_H
