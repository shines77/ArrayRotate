
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>
#include <cstdlib>
#include <cstdio>
#include <string>
#include <cstring>
#include <vector>
#include <algorithm>

#include "CPUWarmUp.h"
#include "StopWatch.h"

#include "jstd/ArrayRotate.h"
#if !defined(_MSC_VER) || (defined(_MSC_VER) && (_MSC_VER >= 2000))
#include "kerbal/algorithm/modifier.hpp"
#endif

template < typename ItemType, std::size_t Length, std::size_t Offset,
           typename Container = std::vector<ItemType> >
void run_rotate_benchmark(Container & array)
{
    static const std::size_t length = (Length <= 0) ? 100000000 : Length;
    static const std::size_t offset = Offset % length;

    test::StopWatch sw;
    double elapsedTime;

    //////////////////////////////////////////////////////////////
#if 0
    sw.start();
    jstd::std_rotate(array.begin(), array.begin() + offset, array.end());
    sw.stop();

    elapsedTime = sw.getElapsedMillisec();
    printf(" jstd::std_rotate(%u, %u): %0.2f ms\n\n", (uint32_t)length, (uint32_t)offset, elapsedTime);
#endif
    //////////////////////////////////////////////////////////////

    sw.start();
    std::rotate(array.begin(), array.begin() + offset, array.end());
    sw.stop();

    elapsedTime = sw.getElapsedMillisec();
    printf(" std::rotate(%u, %u): %0.2f ms\n\n", (uint32_t)length, (uint32_t)offset, elapsedTime);

    //////////////////////////////////////////////////////////////

    sw.start();
    jstd::rotate(array.begin(), array.begin() + offset, array.end());
    sw.stop();

    elapsedTime = sw.getElapsedMillisec();
    printf(" jstd::rotate(%u, %u): %0.2f ms\n\n", (uint32_t)length, (uint32_t)offset, elapsedTime);

    //////////////////////////////////////////////////////////////

#if !defined(_MSC_VER) || (defined(_MSC_VER) && (_MSC_VER >= 2000))
    sw.start();
    kerbal::algorithm::rotate(array.begin(), array.begin() + offset, array.end());
    sw.stop();

    elapsedTime = sw.getElapsedMillisec();
    printf(" kerbal::algorithm::rotate(%u, %u): %0.2f ms\n\n", (uint32_t)length, (uint32_t)offset, elapsedTime);
#endif

    //////////////////////////////////////////////////////////////
}

template <typename Container>
int verify_array(Container & container1, Container & container2)
{
    int offset = 0;
    auto iter1 = container1.cbegin();
    auto iter2 = container2.cbegin();
    for ( ; iter1 != container1.cend(); iter1++, iter2++) {
        if (*iter1 != * iter2)
            return offset;
        offset++;
    }
    return -1;
}

template < typename ItemType, std::size_t Length, std::size_t Offset,
           typename Container = std::vector<ItemType> >
void run_rotate_test(Container & array_std, Container & array)
{
    static const std::size_t length = (Length <= 0) ? 100000000 : Length;
    static const std::size_t offset = Offset % length;

    for (size_t i = 0; i < length; i++) {
        array_std[i] = (int)i;
    }

    std::rotate(array_std.begin(), array_std.begin() + offset, array_std.end());

    for (size_t i = 0; i < length; i++) {
        array[i] = (int)i;
    }

    jstd::rotate(array.begin(), array.begin() + offset, array.end());

    printf(" jstd::rotate(%u, %u): ", (uint32_t)length, (uint32_t)offset);
    int error_pos = verify_array(array, array_std);
    if (error_pos == -1)
        printf("Passed");
    else
        printf("Failed (pos = %d)", error_pos);
    printf("\n\n");

#if !defined(_MSC_VER) || (defined(_MSC_VER) && (_MSC_VER >= 2000))
    for (size_t i = 0; i < length; i++) {
        array[i] = (int)i;
    }

    kerbal::algorithm::rotate(array.begin(), array.begin() + offset, array.end());

    printf(" kerbal::algorithm::rotate(%u, %u): ", (uint32_t)length, (uint32_t)offset);
    error_pos = verify_array(array, array_std);
    if (error_pos == -1)
        printf("Passed");
    else
        printf("Failed (pos = %d)", error_pos);
    printf("\n\n");
#endif
}

void benchmark()
{
    static const size_t test_length = 100000000;

    std::vector<int> array;
    array.resize(test_length);
    for (size_t i = 0; i < test_length; i++) {
        array[i] = (int)i;
    }

    printf("/////////////////////////////////////////////////////\n\n");

    run_rotate_benchmark<int, test_length, 32>(array);

    printf("/////////////////////////////////////////////////////\n\n");

    run_rotate_benchmark<int, test_length, 33>(array);

    printf("/////////////////////////////////////////////////////\n\n");

    run_rotate_benchmark<int, test_length, 33333333>(array);

    printf("/////////////////////////////////////////////////////\n\n");

    run_rotate_benchmark<int, test_length, 50000000>(array);

    printf("/////////////////////////////////////////////////////\n\n");
}

void validate_test()
{
    static const size_t test_length = 100000000;

    std::vector<int> array_std;
    array_std.resize(test_length);

    std::vector<int> array;
    array.resize(test_length);

    printf("--------------------------------------------------------------\n\n");

    run_rotate_test<int, test_length, 33>(array_std, array);

    printf("--------------------------------------------------------------\n\n");

    run_rotate_test<int, test_length, 33333333>(array_std, array);

    printf("--------------------------------------------------------------\n\n");

    run_rotate_test<int, test_length, 50000000>(array_std, array);

    printf("--------------------------------------------------------------\n\n");
}

int main(int argn, char * argv[])
{
    printf("\n");

    validate_test();
    benchmark();
    return 0;
}
