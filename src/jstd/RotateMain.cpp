
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

#include "jstd/ArrayRotate.h"

static const char base64_str[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz+=0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz+=";

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

template <typename ItemType, typename Container>
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

template < typename ItemType, typename Container = std::vector<ItemType> >
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

template < typename ItemType, typename Container = std::vector<ItemType> >
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
    static const std::size_t length = (Length <= 64) ? Length : 64;
    static const std::size_t offset = Offset % length;

    std::vector<int> array;
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
    rotate_test<10, 3>();
    rotate_test<34, 33>();
    //rotate_test<18, 0>();

    printf("-----------------------------------------------------\n");
}

template <std::size_t Length, std::size_t Offset>
void jstd_rotate_test()
{
    static const std::size_t length = (Length <= 64) ? Length : 64;
    static const std::size_t offset = Offset % length;

    std::vector<int> array_std;
    array_std.resize(length);
    for (size_t i = 0; i < length; i++) {
        array_std[i] = base64_str[i];
    }

    std::rotate(array_std.begin(), array_std.begin() + offset, array_std.end());

    std::vector<int> array;
    array.resize(length);
    for (size_t i = 0; i < length; i++) {
        array[i] = base64_str[i];
    }

    jstd::rotate(array.begin(), array.begin() + offset, array.end());
    print_array<char>("jstd::rotate(%u, %u)", length, offset, array);

    printf("\n");
    printf("jstd::rotate(%u, %u): ", (uint32_t)length, (uint32_t)offset);
    int error_pos = verify_array(array, array_std);
    if (error_pos == -1)
        printf("Passed");
    else
        printf("Failed (pos = %d)", error_pos);
    printf("\n\n");
}

void rotate_test()
{
    //jstd_rotate_test<34, 33>();
    //jstd_rotate_test<10, 3>();
    jstd_rotate_test<100, 31>();
    jstd_rotate_test<100, 33>();
}

int main(int argn, char * argv[])
{
    rotate_test();
    //rotate_unit_test();
    return 0;
}
