
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <cstdlib>
#include <cstdio>
#include <vector>
#include <algorithm>

#include "jstd/ArrayRotate.h"

static const char base64_str[65] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz+=";

template <typename ItemType, typename Container = void>
void print_array_impl(Container & container)
{
    for (auto iter = container.cbegin(); iter != container.cend(); iter++) {
        if (sizeof(ItemType) == 8)
            printf("%lld, ", static_cast<int64_t>(*iter));
        else if (sizeof(ItemType) == 4)
            printf("%d, ", static_cast<int>(*iter));
        else if (sizeof(ItemType) == 2)
            printf("%d, ", static_cast<int>(*iter));
        else if (sizeof(ItemType) == 1)
            printf("%c", static_cast<char>(*iter));
    }
}

template <typename ItemType, typename Container = void>
void print_array(const std::string & fmt, std::size_t length, Container & container)
{
    printf("\n");
    printf(fmt.c_str(), (uint32_t)length);
    printf(" = {\n\n");
    printf("  ");
    print_array_impl<ItemType, Container>(container);
    printf("\n\n");
    printf("};\n");
}

template <typename ItemType, typename Container = void>
void print_array(const std::string & fmt, std::size_t length, std::size_t offset, Container & container)
{
    printf("\n");
    printf(fmt.c_str(), (uint32_t)length, (uint32_t)offset);
    printf(" = {\n\n");
    printf("  ");
    print_array_impl<ItemType, Container>(container);
    printf("\n\n");
    printf("};\n");
}

template <std::size_t Length, std::size_t Offset>
void rotate_test()
{
    std::vector<int> array;
    static const std::size_t length = (Length <= 64) ? Length : 64;
    static const std::size_t offset = Offset % length;
    array.resize(length);
    for (size_t i = 0; i < length; i++) {
        array[i] = base64_str[i];
    }

    printf("-----------------------------------------------------\n");
    print_array<char>("array(%u)", length, array);

    std::rotate(array.begin(), array.begin() + Offset, array.end());
    print_array<char>("std::rotate(%u, %u)", length, offset, array);

    for (size_t i = 0; i < length; i++) {
        array[i] = base64_str[i];
    }

    jstd::std_rotate(array.begin(), array.begin() + Offset, array.end());
    print_array<char>("jstd::std_rotate(%u, %u)", length, offset, array);

    for (size_t i = 0; i < length; i++) {
        array[i] = base64_str[i];
    }

    jstd::rotate(array.begin(), array.begin() + Offset, array.end());
    print_array<char>("jstd::rotate(%u, %u)", length, offset, array);

    printf("\n");
}

void rotate_unit_test()
{
    rotate_test<20, 5>();
    rotate_test<18, 5>();
    //rotate_test<18, 0>();

    printf("-----------------------------------------------------\n");
}

int main(int argn, char * argv[])
{
    rotate_unit_test();
    return 0;
}
