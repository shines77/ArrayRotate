
#if defined(_MSC_VER)
#define _CRT_SECURE_NO_WARNINGS
#endif

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

#define USE_KERBAL_ROTATE   1

#include "CPUWarmUp.h"
#include "StopWatch.h"

#include "jstd/ArrayRotate.h"
#include "jstd/ArrayRotate_v1.h"
#include "jstd/ArrayRotate_SIMD.h"

extern void print_marcos();

#if USE_KERBAL_ROTATE
#if !defined(_MSC_VER) || (defined(_MSC_VER) && (_MSC_VER >= 2000))
#include "kerbal/algorithm/modifier.hpp"
#endif
#endif // USE_KERBAL_ROTATE

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
    int error_pos;

    printf("--------------------------------------------------------------\n\n");

    //////////////////////////////////////////////////////////////

    for (size_t i = 0; i < length; i++) {
        array_std[i] = (int)i;
    }

    std::rotate(array_std.begin(), array_std.begin() + offset, array_std.end());

    //////////////////////////////////////////////////////////////

    for (size_t i = 0; i < length; i++) {
        array[i] = (int)i;
    }

    jstd::rotate(array.begin(), array.begin() + offset, array.end());

    printf(" jstd::rotate(%u, %2u):              ", (uint32_t)length, (uint32_t)offset);
    error_pos = verify_array(array, array_std);
    if (error_pos == -1)
        printf("Pass");
    else
        printf("Failed (pos = %d)", error_pos);
    printf("\n");

    //////////////////////////////////////////////////////////////

#if USE_KERBAL_ROTATE
#if !defined(_MSC_VER) || (defined(_MSC_VER) && (_MSC_VER >= 2000))
    for (size_t i = 0; i < length; i++) {
        array[i] = (int)i;
    }

    kerbal::algorithm::rotate(array.begin(), array.begin() + offset, array.end());

    printf(" kerbal::rotate(%u, %2u):            ", (uint32_t)length, (uint32_t)offset);
    error_pos = verify_array(array, array_std);
    if (error_pos == -1)
        printf("Pass");
    else
        printf("Failed (pos = %d)", error_pos);
    printf("\n");
#endif
#endif // USE_KERBAL_ROTATE

    //////////////////////////////////////////////////////////////

    for (size_t i = 0; i < length; i++) {
        array[i] = (int)i;
    }

    jstd::simd::rotate(&array[0], &array[0] + offset, &array[0] + array.size());

    printf(" jstd::simd::rotate(%u, %2u):        ", (uint32_t)length, (uint32_t)offset);
    error_pos = verify_array(array, array_std);
    if (error_pos == -1)
        printf("Pass");
    else
        printf("Failed (pos = %d)", error_pos);
    printf("\n");

    //////////////////////////////////////////////////////////////

    printf("\n");
}

template < typename ItemType, std::size_t Length, std::size_t Offset,
           typename Container = std::vector<ItemType> >
void run_rotate_benchmark(Container & array)
{
    static const std::size_t length = (Length <= 0) ? 100000000 : Length;
    static const std::size_t offset = Offset % length;

    test::StopWatch sw;
    double elapsedTime;

    printf("//////////////////////////////////////////////////////////////////\n\n");

    //////////////////////////////////////////////////////////////

    sw.start();
    std::rotate(array.begin(), array.begin() + offset, array.end());
    sw.stop();

    elapsedTime = sw.getElapsedMillisec();
    printf(" std::rotate(%u, %2u):               %0.2f ms\n", (uint32_t)length, (uint32_t)offset, elapsedTime);

    //////////////////////////////////////////////////////////////

    sw.start();
    jstd::rotate(array.begin(), array.begin() + offset, array.end());
    sw.stop();

    elapsedTime = sw.getElapsedMillisec();
    printf(" jstd::rotate(%u, %2u):              %0.2f ms\n", (uint32_t)length, (uint32_t)offset, elapsedTime);

    //////////////////////////////////////////////////////////////

#if USE_KERBAL_ROTATE
#if !defined(_MSC_VER) || (defined(_MSC_VER) && (_MSC_VER >= 2000))
    sw.start();
    kerbal::algorithm::rotate(array.begin(), array.begin() + offset, array.end());
    sw.stop();

    elapsedTime = sw.getElapsedMillisec();
    printf(" kerbal::rotate(%u, %2u):            %0.2f ms\n", (uint32_t)length, (uint32_t)offset, elapsedTime);
#endif
#endif // USE_KERBAL_ROTATE

    //////////////////////////////////////////////////////////////

    sw.start();
    jstd::simd::rotate(&array[0], &array[0] + offset, &array[0] + array.size());
    sw.stop();

    elapsedTime = sw.getElapsedMillisec();
    printf(" jstd::simd::rotate(%u, %2u):        %0.2f ms\n", (uint32_t)length, (uint32_t)offset, elapsedTime);

    printf("\n");
    //////////////////////////////////////////////////////////////
}

void validate_test()
{
    static const size_t test_length = 100000000;

    std::vector<int> array_std;
    array_std.resize(test_length);

    std::vector<int> array;
    array.resize(test_length);

    run_rotate_test<int, test_length, 1>(array_std, array);
    run_rotate_test<int, test_length, 2>(array_std, array);
    run_rotate_test<int, test_length, 3>(array_std, array);
    run_rotate_test<int, test_length, 4>(array_std, array);
    run_rotate_test<int, test_length, 7>(array_std, array);
    run_rotate_test<int, test_length, 8>(array_std, array);
    run_rotate_test<int, test_length, 9>(array_std, array);
    run_rotate_test<int, test_length, 10>(array_std, array);
    run_rotate_test<int, test_length, 15>(array_std, array);
    run_rotate_test<int, test_length, 32>(array_std, array);
    run_rotate_test<int, test_length, 33>(array_std, array);
    run_rotate_test<int, test_length, 33333333>(array_std, array);
    run_rotate_test<int, test_length, 50000000>(array_std, array);

    printf("--------------------------------------------------------------\n\n");
}

void benchmark()
{
    static const size_t test_length = 100000000;

    std::vector<int> array;
    array.resize(test_length);
    for (size_t i = 0; i < test_length; i++) {
        array[i] = (int)i;
    }

    run_rotate_benchmark<int, test_length, 1>(array);
    run_rotate_benchmark<int, test_length, 2>(array);
    run_rotate_benchmark<int, test_length, 3>(array);
    run_rotate_benchmark<int, test_length, 4>(array);
    run_rotate_benchmark<int, test_length, 7>(array);
    run_rotate_benchmark<int, test_length, 8>(array);
    run_rotate_benchmark<int, test_length, 9>(array);
    run_rotate_benchmark<int, test_length, 10>(array);
    run_rotate_benchmark<int, test_length, 15>(array);
    run_rotate_benchmark<int, test_length, 32>(array);
    run_rotate_benchmark<int, test_length, 33>(array);
    run_rotate_benchmark<int, test_length, 33333333>(array);
    run_rotate_benchmark<int, test_length, 50000000>(array);

    printf("//////////////////////////////////////////////////////////////////\n\n");
}

int main(int argn, char * argv[])
{
    printf("\n");
    print_marcos();

#if 0
    validate_test();
    benchmark();
#endif
    return 0;
}
