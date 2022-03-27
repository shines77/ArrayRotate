
#ifndef JSTD_ARRAY_ROTATE_H
#define JSTD_ARRAY_ROTATE_H

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
#pragma once
#endif

#include <algorithm>

namespace jstd {

template <typename ForwardIt>
ForwardIt // void until C++11
std_rotate(ForwardIt first, ForwardIt n_first, ForwardIt last)
{
    if (first == n_first) return last;
    if (n_first == last) return first;

    ForwardIt read = n_first;
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

} // namespace jstd

#endif // JSTD_ARRAY_ROTATE_H
