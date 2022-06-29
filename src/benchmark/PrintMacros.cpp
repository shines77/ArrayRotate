
#include "jstd/stddef.h"

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifndef __count_of
#define __count_of(arr)     (sizeof(arr) / sizeof(arr[0]))
#endif

#define COMPILER_MARCO_VALUE(value)     #value
#define COMPILER_MARCO_ENTRY(name)      { #name, COMPILER_MARCO_VALUE(name) }

struct CompilerMarco {
    CompilerMarco(const char * name = nullptr, const char * value = nullptr)
        : name(name), value(value) {
    }

    const char * name;
    const char * value;
};

/* Compilers */
static const CompilerMarco compiler_version[] = {
#ifdef __cplusplus
    COMPILER_MARCO_ENTRY(__cplusplus),
#endif

/* gcc or g++ */

#ifdef __GNUC__
    COMPILER_MARCO_ENTRY(__GNUC__),
#endif

#ifdef __GNUC_MAJOR__
    COMPILER_MARCO_ENTRY(__GNUC_MAJOR__),
#endif

#ifdef __GNUC_MINOR__
    COMPILER_MARCO_ENTRY(__GNUC_MINOR__),
#endif

#ifdef __GNUC_PATCHLEVEL__
    COMPILER_MARCO_ENTRY(__GNUC_PATCHLEVEL__),
#endif

#ifdef __GNUG__
    COMPILER_MARCO_ENTRY(__GNUG__),
#endif

/* clang */

#ifdef __clang__
    COMPILER_MARCO_ENTRY(__clang__),
#endif

#ifdef __clang_major__
    COMPILER_MARCO_ENTRY(__clang_major__),
#endif

#ifdef __clang_minor__
    COMPILER_MARCO_ENTRY(__clang_minor__),
#endif

#ifdef __clang_patchlevel__
    COMPILER_MARCO_ENTRY(__clang_patchlevel__),
#endif

/* Interl C++ */

#ifdef __ICL
    COMPILER_MARCO_ENTRY(__ICL),
#endif

#ifdef __ICC
    COMPILER_MARCO_ENTRY(__ICC),
#endif

#ifdef __ECC
    COMPILER_MARCO_ENTRY(__ECC),
#endif

#ifdef __ECL
    COMPILER_MARCO_ENTRY(__ECL),
#endif

#ifdef __ICPC
    COMPILER_MARCO_ENTRY(__ICPC),
#endif

#ifdef __INTEL_COMPILER
    COMPILER_MARCO_ENTRY(__INTEL_COMPILER),
#endif

#ifdef __INTEL_CXX_VERSION
    COMPILER_MARCO_ENTRY(__INTEL_CXX_VERSION),
#endif

/* Visual C++ */

#ifdef _MSC_VER
    COMPILER_MARCO_ENTRY(_MSC_VER),
#endif

#ifdef _MSC_FULL_VER
    COMPILER_MARCO_ENTRY(_MSC_FULL_VER),
#endif

#ifdef _MSVC_LANG
    COMPILER_MARCO_ENTRY(_MSVC_LANG),
#endif

/* DMC++ */

#ifdef __DMC__
    COMPILER_MARCO_ENTRY(__DMC__),
#endif

/* ARM C/C++ */

#ifdef __ARMCC_VERSION
    COMPILER_MARCO_ENTRY(__ARMCC_VERSION),
#endif

/* jstd */

#ifdef JSTD_IS_GCC
    COMPILER_MARCO_ENTRY(JSTD_IS_GCC),
#endif

#ifdef JSTD_IS_CLANG
    COMPILER_MARCO_ENTRY(JSTD_IS_CLANG),
#endif

#ifdef JSTD_IS_MSVC
    COMPILER_MARCO_ENTRY(JSTD_IS_MSVC),
#endif

#ifdef JSTD_IS_ICC
    COMPILER_MARCO_ENTRY(JSTD_IS_ICC),
#endif

#ifdef JSTD_GCC_STYLE_ASM
    COMPILER_MARCO_ENTRY(JSTD_GCC_STYLE_ASM),
#endif

#ifdef JSTD_IS_PURE_GCC
    COMPILER_MARCO_ENTRY(JSTD_IS_PURE_GCC),
#endif
};

static const CompilerMarco compiler_platforms[] = {
#ifdef _WIN32
    COMPILER_MARCO_ENTRY(_WIN32),
#endif

#ifdef _WIN64
    COMPILER_MARCO_ENTRY(_WIN64),
#endif

/* Windows32 by mingw compiler */
#ifdef __MINGW32__
    COMPILER_MARCO_ENTRY(__MINGW32__),
#endif

/* Cygwin */
#ifdef __CYGWIN__
    COMPILER_MARCO_ENTRY(__CYGWIN__),
#endif

/* linux */
#ifdef __linux__
    COMPILER_MARCO_ENTRY(__linux__),
#endif

/* unix */
#ifdef __unix__
    COMPILER_MARCO_ENTRY(__unix__),
#endif

#ifdef __unix
    COMPILER_MARCO_ENTRY(__unix),
#endif

/* FreeBSD */
#ifdef __FreeBSD__
    COMPILER_MARCO_ENTRY(__FreeBSD__),
#endif

/* NetBSD */
#ifdef __NetBSD__
    COMPILER_MARCO_ENTRY(__NetBSD__),
#endif

/* OpenBSD */
#ifdef __OpenBSD__
    COMPILER_MARCO_ENTRY(__OpenBSD__),
#endif

/* Sun OS */
#ifdef __sun__
    COMPILER_MARCO_ENTRY(__sun__),
#endif

/* Apple */
#ifdef __APPLE__
    COMPILER_MARCO_ENTRY(__APPLE__),
#endif

/* Apple */
#ifdef __apple__
    COMPILER_MARCO_ENTRY(__apple__),
#endif

/* MAC OS X */
#ifdef __MaxOSX__
    COMPILER_MARCO_ENTRY(__MaxOSX__),
#endif

/* Android */
#ifdef __ANDROID__
    COMPILER_MARCO_ENTRY(__ANDROID__),
#endif
};

static const CompilerMarco compiler_arch[] = {
#ifdef JSTD_IS_X86
    COMPILER_MARCO_ENTRY(JSTD_IS_X86),
#endif

#ifdef JSTD_IS_X86_64
    COMPILER_MARCO_ENTRY(JSTD_IS_X86_64),
#endif

#ifdef JSTD_IS_X86_I386
    COMPILER_MARCO_ENTRY(JSTD_IS_X86_I386),
#endif

#ifdef JSTD_WORD_LEN
    COMPILER_MARCO_ENTRY(JSTD_WORD_LEN),
#endif

#ifdef __BIG_ENDIAN__
    COMPILER_MARCO_ENTRY(__BIG_ENDIAN__),
#endif

#ifdef __LITTLE_ENDIAN__
    COMPILER_MARCO_ENTRY(__LITTLE_ENDIAN__),
#endif
};

static const CompilerMarco compiler_others[] = {
#ifdef __DATE__
    COMPILER_MARCO_ENTRY(__DATE__),
#endif

#ifdef __TIME__
    COMPILER_MARCO_ENTRY(__TIME__),
#endif

#ifdef __FILE__
    COMPILER_MARCO_ENTRY(__FILE__),
#endif

#ifdef _BSD_SOURCE
    COMPILER_MARCO_ENTRY(_BSD_SOURCE),
#endif

#ifdef _POSIX_SOURCE
    COMPILER_MARCO_ENTRY(_POSIX_SOURCE),
#endif

#ifdef _XOPEN_SOURCE
    COMPILER_MARCO_ENTRY(_XOPEN_SOURCE),
#endif

#ifdef _GNU_SOURCE
    COMPILER_MARCO_ENTRY(_GNU_SOURCE),
#endif

#ifdef __VERSION__
    COMPILER_MARCO_ENTRY(__VERSION__),
#endif

#ifdef __cpp_constexpr
    COMPILER_MARCO_ENTRY(__cpp_constexpr),
#endif

#ifdef __cpp_variable_templates
    COMPILER_MARCO_ENTRY(__cpp_variable_templates),
#endif

#ifdef __cpp_lib_integer_sequence
    COMPILER_MARCO_ENTRY(__cpp_lib_integer_sequence),
#endif

#ifdef __cpp_exceptions
    COMPILER_MARCO_ENTRY(__cpp_exceptions),
#endif
};

void print_marcos()
{
    size_t i;

    printf("/* Compiler definitions: */\n\n");
    for (i = 0; i < __count_of(compiler_version); i++) {
        printf("#define %s %s\n", compiler_version[i].name, compiler_version[i].value);
    }
    printf("\n");

    printf("/* Platform definitions: */\n\n");
    for (i = 0; i < __count_of(compiler_platforms); i++) {
        printf("#define %s %s\n", compiler_platforms[i].name, compiler_platforms[i].value);
    }
    printf("\n");

    printf("/* Architecture definitions: */\n\n");
    for (i = 0; i < __count_of(compiler_arch); i++) {
        printf("#define %s %s\n", compiler_arch[i].name, compiler_arch[i].value);
    }
    printf("\n");

    printf("/* Other definitions: */\n\n");
    for (i = 0; i < __count_of(compiler_others); i++) {
        printf("#define %s %s\n", compiler_others[i].name, compiler_others[i].value);
    }
    printf("\n");

    printf("\n");
}
