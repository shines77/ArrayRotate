
#include <stdlib.h>
#include <stdio.h>
#include <cstdlib>
#include <cstdio>
#include <vector>
#include <algorithm>

#include "CPUWarmUp.h"
#include "StopWatch.h"

#include "jstd/ArrayRotate.h"

void run_rotate_benchmark()
{
    test::StopWatch sw;
    double elapsedTime;

    std::vector<int> array;
    array.resize(100000000);

    sw.start();
    std::rotate(array.begin(), array.begin() + 32, array.end());
    sw.stop();

    elapsedTime = sw.getElapsedMillisec();
    printf("std::rotate: %0.2f ms\n\n", elapsedTime);

    sw.start();
    jstd::std_rotate(array.begin(), array.begin() + 32, array.end());
    sw.stop();

    elapsedTime = sw.getElapsedMillisec();
    printf("jstd::std_rotate: %0.2f ms\n\n", elapsedTime);
}

int main(int argn, char * argv[])
{
    printf("\n");
    run_rotate_benchmark();
    return 0;
}
