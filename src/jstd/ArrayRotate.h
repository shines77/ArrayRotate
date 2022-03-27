
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

template <typename ForwardIt, typename ItemType = void>
ForwardIt // void until C++11
rotate(ForwardIt first, ForwardIt mid, ForwardIt last)
{
    if (first == mid) return last;
    if (mid == last) return first;

    ForwardIt read = mid;
    ForwardIt write = first;

    while (read != last) {
        std::iter_swap(write++, read++);
    }

    // Rotate the remaining sequence into place
    std::size_t length = last - first;
    std::size_t offset = mid - first;
    std::size_t remain = length % offset;

    if (remain != 0) {
        first = write;
        mid = write + offset - remain;
        read = mid;
        while (read != last) {
            std::iter_swap(write++, read++);
        }

        while (write != last) {
            length = offset;
            offset = remain;
            remain = length % offset;
            if (remain != 0) {
                read = write + offset - remain;
                while (read != last) {
                    std::iter_swap(write++, read++);
                }
            }
            else break;
        }
    }

    return write;
}

} // namespace jstd

#endif // JSTD_ARRAY_ROTATE_H
