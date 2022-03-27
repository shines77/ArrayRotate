
#include <stdlib.h>
#include <stdio.h>
#include <cstdlib>
#include <cstdio>
#include <vector>
#include <algorithm>

#include "jstd/ArrayRotate.h"

void rotate_unit_test()
{
    std::vector<int> array;
    array.resize(10);

    std::rotate(array.begin(), array.begin() + 32, array.end());

    jstd::std_rotate(array.begin(), array.begin() + 32, array.end());
}

int main(int argn, char * argv[])
{
    rotate_unit_test();
    return 0;
}
