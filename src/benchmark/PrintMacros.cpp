
#include "jstd/stddef.h"

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifndef JSTD_MAKE_MARCOINFO
#define JSTD_MAKE_MARCOINFO(x)  { #x, JSTD_TO_STRING(x) }
#endif

#ifndef jstd_count_of
#define jstd_count_of(arr)  (sizeof(arr) / sizeof(arr[0]))
#endif

typedef struct {
    const char * name;
    const char * value;
} MarcoInfo;

/* Compilers */
static const MarcoInfo g_compilers[] = {
#ifdef __GNUC__         /* GCC */
    JSTD_MAKE_MARCOINFO(__GNUC__),
#endif

#ifdef __clang__         /* GCC */
    JSTD_MAKE_MARCOINFO(__clang__),
#endif

#ifdef __INTEL_COMPILER /* Interl C++ */
    JSTD_MAKE_MARCOINFO(__INTEL_COMPILER),
#endif

#ifdef __ICL            /* Interl C++ */
    JSTD_MAKE_MARCOINFO(__ICL),
#endif

#ifdef __ICC            /* Interl C++ */
    JSTD_MAKE_MARCOINFO(__ICC),
#endif

#ifdef __ECC            /* Interl C++ */
    JSTD_MAKE_MARCOINFO(__ECC),
#endif

#ifdef __ECL            /* Interl C++ */
    JSTD_MAKE_MARCOINFO(__ECL),
#endif

#ifdef __ICPC            /* Interl C++ */
    JSTD_MAKE_MARCOINFO(__ICPC),
#endif

#ifdef _MSC_VER         /* Visual C++ */
    JSTD_MAKE_MARCOINFO(_MSC_VER),
#endif

#ifdef __DMC__          /* DMC++ */
    JSTD_MAKE_MARCOINFO(__DMC__),
#endif

#ifdef __ARMCC_VERSION  /* ARM C/C++ */
    JSTD_MAKE_MARCOINFO(__ARMCC_VERSION),
#endif

#ifdef JSTD_IS_GCC
    JSTD_MAKE_MARCOINFO(JSTD_IS_GCC),
#endif

#ifdef JSTD_IS_CLANG
    JSTD_MAKE_MARCOINFO(JSTD_IS_CLANG),
#endif

#ifdef JSTD_IS_MSVC
    JSTD_MAKE_MARCOINFO(JSTD_IS_MSVC),
#endif

#ifdef JSTD_IS_ICC
    JSTD_MAKE_MARCOINFO(JSTD_IS_ICC),
#endif

#ifdef __INTEL_CXX_VERSION
    JSTD_MAKE_MARCOINFO(__INTEL_CXX_VERSION),
#endif

#ifdef JSTD_GCC_STYLE_ASM
    JSTD_MAKE_MARCOINFO(JSTD_GCC_STYLE_ASM),
#endif

#ifdef JSTD_IS_PURE_GCC
    JSTD_MAKE_MARCOINFO(JSTD_IS_PURE_GCC),
#endif
};

static const MarcoInfo g_platforms[] = {
#ifdef _WIN32                       /* Windows 32 or Windows 64 */
    JSTD_MAKE_MARCOINFO(_WIN32),
#endif

#ifdef _WIN64                       /* Windows 64 */
    JSTD_MAKE_MARCOINFO(_WIN64),
#endif

#ifdef __MINGW32__                  /* Windows32 by mingw compiler */
    JSTD_MAKE_MARCOINFO(__MINGW32__),
#endif

#ifdef __CYGWIN__                   /* Cygwin */
    JSTD_MAKE_MARCOINFO(__CYGWIN__),
#endif

#ifdef __linux__                    /* linux */
    JSTD_MAKE_MARCOINFO(__linux__),
#endif

#ifdef __FreeBSD__                  /* FreeBSD */
    JSTD_MAKE_MARCOINFO(__FreeBSD__),
#endif

#ifdef __NetBSD__                   /* NetBSD */
    JSTD_MAKE_MARCOINFO(__NetBSD__),
#endif

#ifdef __OpenBSD__                  /* OpenBSD */
    JSTD_MAKE_MARCOINFO(__OpenBSD__),
#endif

#ifdef __sun__                      /* Sun OS */
    JSTD_MAKE_MARCOINFO(__sun__),
#endif

#ifdef __MaxOSX__                   /* MAC OS X */
    JSTD_MAKE_MARCOINFO(__MaxOSX__),
#endif

#ifdef __unix__                     /* unix */
    JSTD_MAKE_MARCOINFO(__unix__),
#endif

#ifdef JSTD_IS_X86
    JSTD_MAKE_MARCOINFO(JSTD_IS_X86),
#endif

#ifdef JSTD_IS_X86_64
    JSTD_MAKE_MARCOINFO(JSTD_IS_X86_64),
#endif

#ifdef JSTD_IS_X86_I386
    JSTD_MAKE_MARCOINFO(JSTD_IS_X86_I386),
#endif

#ifdef JSTD_WORD_SIZE
    JSTD_MAKE_MARCOINFO(JSTD_WORD_SIZE),
#endif
};

static const MarcoInfo g_others[] = {
#ifdef __DATE__ 
    JSTD_MAKE_MARCOINFO(__DATE__),
#endif

#ifdef __TIME__ 
    JSTD_MAKE_MARCOINFO(__TIME__),
#endif

#ifdef _BSD_SOURCE
    JSTD_MAKE_MARCOINFO(_BSD_SOURCE),
#endif

#ifdef _POSIX_SOURCE
    JSTD_MAKE_MARCOINFO(_POSIX_SOURCE),
#endif

#ifdef _XOPEN_SOURCE
    JSTD_MAKE_MARCOINFO(_XOPEN_SOURCE),
#endif

#ifdef _GNU_SOURCE
    JSTD_MAKE_MARCOINFO(_GNU_SOURCE),
#endif

#ifdef __GNUC_MINOR__
    JSTD_MAKE_MARCOINFO(__GNUC_MINOR__),
#endif

#ifdef __VERSION__
    JSTD_MAKE_MARCOINFO(__VERSION__),
#endif

#ifdef __unix
    JSTD_MAKE_MARCOINFO(__unix),
#endif

#ifdef __BIG_ENDIAN__
    JSTD_MAKE_MARCOINFO(__BIG_ENDIAN__),
#endif

#ifdef __LITTLE_ENDIAN__
    JSTD_MAKE_MARCOINFO(__LITTLE_ENDIAN__),
#endif
};

void print_marcos()
{
    size_t i;

    printf("/* Compiler definitions: */\n\n");
    for (i = 0; i < jstd_count_of(g_compilers); i++) {
        printf("#define %s %s\n", g_compilers[i].name, g_compilers[i].value);
    }
    printf("\n");

    printf("/* Platform definitions: */\n\n");
    for (i = 0; i < jstd_count_of(g_platforms); i++) {
        printf("#define %s %s\n", g_platforms[i].name, g_platforms[i].value);
    }
    printf("\n");

    printf("/* Other definitions: */\n\n");
    for (i = 0; i < jstd_count_of(g_others); i++) {
        printf("#define %s %s\n", g_others[i].name, g_others[i].value);
    }
    printf("\n");

    printf("\n");
}
