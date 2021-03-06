
#if defined(_MSC_VER)
#define _CRT_SECURE_NO_WARNINGS
#endif

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

#include "benchmark/CPUWarmUp.h"
#include "benchmark/StopWatch.h"

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

uint32_t next_random_u32()
{
#if (RAND_MAX == 0x7FFF)
    uint32_t rnd32 = (((uint32_t)rand() & 0x03) << 30) |
                      ((uint32_t)rand() << 15) |
                       (uint32_t)rand();
#else
    uint32_t rnd32 = ((uint32_t)rand() << 16) | (uint32_t)rand();
#endif
    return rnd32;
}

uint64_t next_random_u64()
{
#if (RAND_MAX == 0x7FFF)
    uint64_t rnd64 = (((uint64_t)rand() & 0x0F) << 60) |
                      ((uint64_t)rand() << 45) |
                      ((uint64_t)rand() << 30) |
                      ((uint64_t)rand() << 15) |
                       (uint64_t)rand();
#else
    uint64_t rnd64 = ((uint64_t)rand() << 32) | (uint64_t)rand();
#endif
    return rnd64;
}

template <typename Container>
int verify_array(Container & container1, Container & container2)
{
    int offset = 0;
    auto iter1 = container1.cbegin();
    auto iter2 = container2.cbegin();
    for ( ; iter1 != container1.cend(); ++iter1, ++iter2) {
        if (*iter1 != *iter2)
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

    for (size_t i = 0; i < length; i++) {
        array[i] = dict_str[i];
    }

    jstd::simd::rotate(&array[0], &array[0] + Offset, &array[0] + array.size());
    print_array<char>("jstd::simd::rotate(%u, %u)", length, offset, array);

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
        printf("Pass");
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
        printf("Pass");
    else
        printf("Failed (pos = %d)", error_pos);
    printf("\n\n");

    for (size_t i = 0; i < length; i++) {
        array[i] = dict_str[i];
    }

    jstd::simd::rotate(&array[0], &array[0] + offset, &array[0] + array.size());
    print_array<char>("jstd::simd::rotate(%u, %u)", length, (length - offset), array);

    printf("\n");
    printf("jstd::simd::rotate(%u, %u): ", (uint32_t)length, (uint32_t)(length - offset));
    error_pos = verify_array(array, array_std);
    if (error_pos == -1)
        printf("Pass");
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
        printf("Pass");
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
        printf("Pass");
    else
        printf("Failed (pos = %d)", error_pos);
    printf("\n\n");
}

void simd_rotate_test()
{
    printf("-----------------------------------------------------\n");
#if 0
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
#endif

#ifdef _DEBUG
    jstd_simd_rotate_test<1000000, 33>();
#else
    jstd_simd_rotate_test<100000000, 33>();
#endif
    printf("-----------------------------------------------------\n");
}

void fast_div_verify_fast()
{
    printf("fast_div_verify_fast():\n\n");
    for (uint32_t d = 0; d < (uint32_t)jstd::kMaxDivTable; d++) {
    //for (uint32_t d = 0; d < 64; d++) {
        if ((d & (d - 1)) != 0) {
            uint32_t first_err = 0, errors = 0, no_errors = 0;
            uint32_t first_n = 0x7FFFFFFFul - 1000;
            // Round to N times of d and sub 1
            first_n = first_n - first_n % d - 1;
            uint32_t q0 = first_n / d;
            for (uint32_t n = first_n; n >= first_n; n += d) {
                uint32_t q = jstd::fast_div_u32(n, d);
                if (q != q0) {
                    if (first_err == 0) {
                        first_err = n;
                    }
                    errors++;
                    no_errors += (d - 1);
                }
                q0++;
            }
            if (errors != 0) {
                printf("d = %-4u : first_err = 0x%08X, errors = %u, no_errors = %u\n",
                       d, first_err, errors, no_errors);
            }
            else {
                printf("d = %-4u : no errors\n", d);
            }
        } else {
            if (d == 0) {
                printf("d = %-4u : skip\n", d);
                continue;
            }
            uint32_t first_err = 0, errors = 0, no_errors = 0;
            uint32_t first_n = 0x7FFFFFFFul - 1000;
            for (uint32_t n = first_n; n < first_n + (1u << 20); n++) {
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
        }
    }
    printf("\n\n");
}

void fast_div_verify()
{
    printf("fast_div_verify():\n\n");
    //for (uint32_t d = 0; d < (uint32_t)jstd::kMaxDivTable; d++) {
    for (uint32_t d = 0; d < 32; d++) {
        if ((d & (d - 1)) != 0) {
            uint32_t first_err = 0, errors = 0, no_errors = 0;
            uint32_t first_n = 0x7FFFFFFFul - 1000;
            // Round to N times of d and
            first_n = first_n - first_n % d;
            uint32_t q0 = first_n / d;
            for (uint32_t n = first_n; n >= first_n; n += d) {
                uint32_t max_i = ((n + d) >= first_n) ? d : ((0xFFFFFFFFul - n) + 1);
                for (uint32_t i = 0; i < max_i; i++) {
                    uint32_t q = jstd::fast_div_u32(n + i, d);
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
                q0++;
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

void fast_div_verify_msvc()
{
    printf("fast_div_verify_msvc():\n\n");
    //for (uint32_t d = 0; d < (uint32_t)jstd::kMaxDivTable; d++) {
    for (uint32_t d = 0; d < 32; d++) {
        if ((d & (d - 1)) != 0) {
            uint32_t first_err = 0, errors = 0, no_errors = 0;
            uint32_t first_n = 0x7FFFFFFFul - 1000;
            // Round to N times of d
            first_n = first_n - first_n % d;
            uint32_t q0 = first_n / d;
            for (uint32_t n = first_n; n >= first_n; n += d) {
                uint32_t max_i = ((n + d) >= first_n) ? d : ((0xFFFFFFFFul - n) + 1);
                for (uint32_t i = 0; i < max_i; i++) {
                    uint32_t q = jstd::fast_div_u32(n + i, d);
                    if (first_err == 0) {
                        if (q != q0) {
                            first_err = n;
                            errors++;
                        }
                    } else {
                        if (q != q0)
                            errors++;
                        else
                            no_errors++;
                    }
                }
                q0++;
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

void fast_div_verify_test()
{
    test::StopWatch sw;
    double elapsedTime;

    sw.start();
    fast_div_verify();
    sw.stop();

    elapsedTime = sw.getElapsedMillisec();
    printf("fast_div_verify(): %0.2f ms\n\n", elapsedTime);

    sw.start();
    fast_div_verify_msvc();
    sw.stop();

    elapsedTime = sw.getElapsedMillisec();
    printf("fast_div_verify_msvc(): %0.2f ms\n\n", elapsedTime);
}

void fast_mod_verify()
{
    printf("fast_mod_verify():\n\n");
    //for (uint32_t d = 0; d < (uint32_t)jstd::kMaxDivTable; d++) {
    for (uint32_t d = 0; d < 32; d++) {
        //if ((d & (d - 1)) != 0) {
        if (d != 0) {
            uint32_t first_err = 0, errors = 0, no_errors = 0;
            uint32_t first_n = 0;
            for (uint32_t n = first_n; n < 0x80000u; n += d) {
                for (uint32_t i = 0; i < d; i++) {
                    uint32_t q = jstd::fast_mod_u32(n + i, d);
                    if (q != i) {
                        if (first_err == 0) {
                            first_err = n;
                        }
                        errors++;
                    }
                    else if (first_err != 0) {
                        no_errors++;
                    }
                }
            }
            if (errors != 0) {
                printf("d = %-4u : first_err = 0x%08X, errors = %u, no_errors = %u\n",
                       d, first_err, errors, no_errors);
            }
            else {
                printf("d = %-4u : no errors\n", d);
            }

            first_err = 0; errors = 0; no_errors = 0;
            first_n = 0x7FFFFFFFul - 1000;
            // Round to N times of d
            first_n = first_n - first_n % d;
            for (uint32_t n = first_n; n >= first_n; n += d) {
                uint32_t max_i = ((n + d) >= first_n) ? d : ((0xFFFFFFFFul - n) + 1);
                for (uint32_t i = 0; i < max_i; i++) {
                    uint32_t q = jstd::fast_mod_u32(n + i, d);
                    if (q != i) {
                        if (first_err == 0) {
                            first_err = n;
                        }
                        errors++;
                    }
                    else if (first_err != 0) {
                        no_errors++;
                    }
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
#if 0
    jstd::genDivRatioTbl();
    jstd::genModRatioTbl();
#endif

#if 1
    rotate_test();
    rotate_unit_test();

    //fast_mod_verify();

#ifndef _DEBUG
#if 1
    //fast_div_verify_fast();
#else
#ifdef _MSC_VER
    fast_div_verify_msvc();
#else
    fast_div_verify();
#endif // _MSC_VER
#endif // 1
#endif // _DEBUG

    //fast_mod_verify();

    //simd_rotate_test();
#endif

#if defined(_DEBUG) && defined(_MSC_VER)
    ::system("pause");
#endif
    return 0;
}
