
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <cstdlib>
#include <cstdio>
#include <vector>
#include <algorithm>

#include "CPUWarmUp.h"
#include "StopWatch.h"

#include "jstd/ArrayRotate.h"

template < typename ItemType, std::size_t Length, std::size_t Offset,
           typename Container = std::vector<ItemType> >
void run_rotate_benchmark(Container & array)
{
    static const std::size_t length = (Length <= 0) ? 100000000 : Length;
    static const std::size_t offset = Offset % length;

    test::StopWatch sw;
    double elapsedTime;

    //////////////////////////////////////////////////////////////

    sw.start();
    std::rotate(array.begin(), array.begin() + offset, array.end());
    sw.stop();

    elapsedTime = sw.getElapsedMillisec();
    printf("std::rotate(%u, %u): %0.2f ms\n\n", (uint32_t)length, (uint32_t)offset, elapsedTime);

    //////////////////////////////////////////////////////////////

    sw.start();
    jstd::std_rotate(array.begin(), array.begin() + offset, array.end());
    sw.stop();

    elapsedTime = sw.getElapsedMillisec();
    printf("jstd::std_rotate(%u, %u): %0.2f ms\n\n", (uint32_t)length, (uint32_t)offset, elapsedTime);

    //////////////////////////////////////////////////////////////

    sw.start();
    jstd::rotate(array.begin(), array.begin() + offset, array.end());
    sw.stop();

    elapsedTime = sw.getElapsedMillisec();
    printf("jstd::rotate(%u, %u): %0.2f ms\n\n", (uint32_t)length, (uint32_t)offset, elapsedTime);

    //////////////////////////////////////////////////////////////
}

void benchmark()
{
    static const size_t test_length = 100000000;

    std::vector<int> array;
    array.resize(test_length);
    for (int i = 0; i < test_length; i++) {
        array[i] = i;
    }

    run_rotate_benchmark<int, test_length, 32>(array);
}

int main(int argn, char * argv[])
{
    printf("\n");

    benchmark();
    return 0;
}
