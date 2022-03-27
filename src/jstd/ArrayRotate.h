
#ifndef JSTD_ARRAY_ROTATE_H
#define JSTD_ARRAY_ROTATE_H

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
#pragma once
#endif

#include <cstdint>
#include <cstddef>
#include <cstdbool>
#include <algorithm>

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

static inline std::size_t fast_mod(std::size_t a, std::size_t b)
{
    return ((a < b) ? a : ((a < b * 2) ? (a - b) : (a % b)));
}

template <typename ForwardIt, typename ItemType = void>
ForwardIt // void until C++11
left_rotate(ForwardIt first, ForwardIt mid, ForwardIt last)
{
    if (first == mid) return last;
    if (mid == last) return first;

    // Rotate the remaining sequence into place
    const std::size_t length0 = last - first;
    const std::size_t shift0 = mid - first;
    const std::size_t remain0 = fast_mod(length0, shift0);

    ForwardIt read = mid;
    ForwardIt write = first;

    do {
        std::iter_swap(write++, read++);
    } while (read != last);

    if (remain0 != 0) {
        std::size_t length = shift0;
        std::size_t shift = shift0 - remain0;
        std::size_t remain;
        if (shift != 1) {
            read = write + shift;
            remain = fast_mod(length, shift);
            while (read != last) {
                std::iter_swap(write++, read++);
            }
            
            while (remain != 0) {
                length = shift;
                shift = shift - remain;
                if (true || shift != 1) {
                    read = write + shift;
                    remain = fast_mod(length, shift);
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

    return write;
}

template <typename ForwardIt, typename ItemType = void>
ForwardIt // void until C++11
right_rotate(ForwardIt first, ForwardIt mid, ForwardIt last)
{
    if (first == mid) return last;
    if (mid == last) return first;

    ForwardIt read = mid;
    ForwardIt write = last;

    do {
        std::iter_swap(--read, --write);
    } while (read != first);

    return write;
}

template <typename ForwardIt, typename ItemType = void>
inline
ForwardIt // void until C++11
rotate(ForwardIt first, ForwardIt mid, ForwardIt last)
{
    return left_rotate<ForwardIt, ItemType>(first, mid, last);
}

} // namespace jstd

#endif // JSTD_ARRAY_ROTATE_H
