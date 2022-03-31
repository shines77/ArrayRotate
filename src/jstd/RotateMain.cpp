
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <inttypes.h>
#include <string.h>
#include <cstdlib>
#include <cstdio>
#include <string>
#include <cstring>
#include <vector>
#include <algorithm>

#include "jstd/FastDiv.h"
#include "jstd/FastMod.h"

#include "jstd/ArrayRotate.h"
#include "jstd/ArrayRotate_v1.h"
#include "jstd/ArrayRotate_SIMD.h"

static const char dict_str[] =
    "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz+="
    "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz+="
    "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz+="
    "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz+=";

static const size_t kDictMaxLen = sizeof(dict_str) - 1;

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
            printf("%" PRIu64 ", ", static_cast<int64_t>(*iter));
        else if (sizeof(ItemType) == 4)
            printf("%d, ", static_cast<int>(*iter));
        else if (sizeof(ItemType) == 2)
            printf("%d, ", static_cast<int>(*iter));
        else if (sizeof(ItemType) == 1)
            printf("%c", static_cast<char>(*iter));
    }
}

template <typename ItemType, typename Container>
void print_array_impl(Container & container, std::size_t max_length)
{
    for (auto iter = container.cbegin(); iter != container.cend(); iter++) {
        if (sizeof(ItemType) == 8)
            printf("%" PRIu64 ", ", static_cast<int64_t>(*iter));
        else if (sizeof(ItemType) == 4)
            printf("%d, ", static_cast<int>(*iter));
        else if (sizeof(ItemType) == 2)
            printf("%d, ", static_cast<int>(*iter));
        else if (sizeof(ItemType) == 1) {
            char ch = static_cast<char>(*iter);
            if (ch < 32)
                printf(".");
            else if (ch > 127)
                printf("?");
            else if (ch == 32)
                printf("#");
            else
                printf("%c", ch);
        }
        max_length--;
        if (max_length == 0)
            break;
    }
}

template < typename ItemType, typename Container = std::vector<ItemType> >
void print_array(const std::string & fmt, std::size_t length, Container & container)
{
    printf("\n");
    printf(fmt.c_str(), (uint32_t)length);
    printf(" = {\n\n");
    printf("  ");
    print_array_impl<ItemType, Container>(container, 100);
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
    print_array_impl<ItemType, Container>(container, 100);
    printf("\n\n");
    printf("};\n");
}

template <std::size_t Length, std::size_t Offset>
void rotate_test()
{
    static const std::size_t length = (Length <= kDictMaxLen) ? Length : kDictMaxLen;
    static const std::size_t offset = Offset % length;

    std::vector<int> array;
    array.resize(length);
    for (size_t i = 0; i < length; i++) {
        array[i] = dict_str[i];
    }

    printf("-----------------------------------------------------\n");
    print_array<char>("array(%u)", length, array);

    std::rotate(array.begin(), array.begin() + Offset, array.end());
    print_array<char>("std::rotate(%u, %u)", length, offset, array);

    for (size_t i = 0; i < length; i++) {
        array[i] = dict_str[i];
    }

    jstd::std_rotate(array.begin(), array.begin() + Offset, array.end());
    print_array<char>("jstd::std_rotate(%u, %u)", length, offset, array);

    for (size_t i = 0; i < length; i++) {
        array[i] = dict_str[i];
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
    static const std::size_t length = (Length <= kDictMaxLen) ? Length : kDictMaxLen;
    static const std::size_t offset = Offset % length;

    std::vector<int> array_std;
    array_std.resize(length);
    for (size_t i = 0; i < length; i++) {
        array_std[i] = dict_str[i];
    }

    std::rotate(array_std.begin(), array_std.begin() + offset, array_std.end());

    std::vector<int> array;
    array.resize(length);
    for (size_t i = 0; i < length; i++) {
        array[i] = dict_str[i];
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

    for (size_t i = 0; i < length; i++) {
        array[i] = dict_str[i];
    }

    jstd::right_rotate(array.begin(), array.begin() + offset, array.end());
    print_array<char>("jstd::right_rotate(%u, %u)", length, (length - offset), array);

    printf("\n");
    printf("jstd::right_rotate(%u, %u): ", (uint32_t)length, (uint32_t)(length - offset));
    error_pos = verify_array(array, array_std);
    if (error_pos == -1)
        printf("Passed");
    else
        printf("Failed (pos = %d)", error_pos);
    printf("\n\n");
}

void rotate_test()
{
    printf("-----------------------------------------------------\n");
    jstd_rotate_test<34, 33>();
    printf("-----------------------------------------------------\n");
    jstd_rotate_test<10, 3>();
    printf("-----------------------------------------------------\n");
    jstd_rotate_test<10, 7>();
    printf("-----------------------------------------------------\n");
    jstd_rotate_test<100, 31>();
    printf("-----------------------------------------------------\n");
    jstd_rotate_test<100, 33>();
    printf("-----------------------------------------------------\n");
}

template <std::size_t Length, std::size_t Offset>
void jstd_simd_rotate_test()
{
    static const std::size_t length = Length;
    static const std::size_t offset = Offset % length;

    std::vector<int> array_std;
    array_std.resize(length);
    for (size_t i = 0; i < length; i++) {
        array_std[i] = (int)i;
    }

    std::rotate(array_std.begin(), array_std.begin() + offset, array_std.end());

    std::vector<int> array;
    array.resize(length);
    for (size_t i = 0; i < length; i++) {
        array[i] = (int)i;
    }

    jstd::simd::rotate<int>(&array[0], array.size(), offset);
    print_array<char>("jstd::simd::rotate(%u, %u)", length, offset, array);

    printf("\n");
    printf("jstd::simd::rotate(%u, %u): ", (uint32_t)length, (uint32_t)offset);
    int error_pos = verify_array(array, array_std);
    if (error_pos == -1)
        printf("Passed");
    else
        printf("Failed (pos = %d)", error_pos);
    printf("\n\n");

    for (size_t i = 0; i < length; i++) {
        array[i] = (int)i;
    }

    jstd::simd::rotate_simple(&array[0], array.size(), offset);
    print_array<char>("jstd::simd::rotate_simple(%u, %u)", length, offset, array);

    printf("\n");
    printf("jstd::simd::rotate_simple(%u, %u): ", (uint32_t)length, (uint32_t)offset);
    error_pos = verify_array(array, array_std);
    if (error_pos == -1)
        printf("Passed");
    else
        printf("Failed (pos = %d)", error_pos);
    printf("\n\n");
}

void simd_rotate_test()
{
    printf("-----------------------------------------------------\n");
    /*
    jstd_simd_rotate_test<34, 33>();
    printf("-----------------------------------------------------\n");
    jstd_simd_rotate_test<10, 3>();
    printf("-----------------------------------------------------\n");
    jstd_simd_rotate_test<10, 7>();
    printf("-----------------------------------------------------\n");
    jstd_simd_rotate_test<100, 31>();
    printf("-----------------------------------------------------\n");
    jstd_simd_rotate_test<100, 33>();
    printf("-----------------------------------------------------\n");
    //*/
#ifdef _DEBUG
    jstd_simd_rotate_test<1000000, 33>();
#else
    jstd_simd_rotate_test<100000000, 33>();
#endif
    printf("-----------------------------------------------------\n");
}

void fast_div_verify()
{
    printf("fast_div_verify():\n\n");
    //for (uint32_t d = 1; d < (uint32_t)jstd::kMaxDivTable; d++) {
    for (uint32_t d = 1; d < 32; d++) {
        if ((d & (d - 1)) != 0) {
            uint32_t first_err = 0, errors = 0, no_errors = 0;
            for (uint32_t n = 0x7FFFFFFFul; n != 0; n++) {
                uint32_t q = jstd::fast_div_u32(n, d);
                uint32_t q0 = n / d;
                if (q != q0) {
                    if (first_err == 0) {
                        first_err = n;
                    }
                    errors++;
                }
                else if (first_err != 0) {
                    no_errors++;
                }
            }
            if (errors != 0) {
                printf("d = %-4u : first_err = 0x%08X, errors = %u, no_errors = %u\n",
                       d, first_err, errors, no_errors);
            }
            else {
                printf("d = %-4u : no errors\n", d);
            }
        } else {
            printf("d = %-4u : skip\n", d);
        }

    }
    printf("\n\n");
}

int main(int argn, char * argv[])
{
#ifdef _DEBUG
    jstd::genDivRatioTbl();
#endif
    //jstd::genModRatioTbl();
    //rotate_test();
    //rotate_unit_test();
#ifndef _DEBUG
    fast_div_verify();
#endif
    //simd_rotate_test();
    return 0;
}
