
#ifndef JSTD_BASIC_STDDEF_H
#define JSTD_BASIC_STDDEF_H

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

#if defined(WIN64) || defined(_WIN64) || defined(_M_X64) || defined(_M_AMD64) \
 || defined(_M_IA64) || defined(__amd64__) || defined(__x86_64__)
  #define JSTD_IS_X86       1
  #define JSTD_X86_64       1
  #define JSTD_WORD_SIZE    64
#else
  #if defined(WIN32) || defined(_WIN32) || defined (_M_IX86) || defined(__i386__)
    #define JSTD_IS_X86     1
    #define JSTD_X86_I386   1
  #endif
  #define JSTD_WORD_SIZE    32
#endif

//
// What compiler is it?
//
#if defined(__INTEL_COMPILER) || defined(__ICL) || defined(__ICC) || defined(__ECC)
  #ifndef JSTD_IS_ICC
  #define JSTD_IS_ICC     1
  #endif
#endif

#if defined(_MSC_VER)
  #ifndef JSTD_IS_MSVC
  #define JSTD_IS_MSVC    1
  #endif
#elif defined(__GNUC__) && !defined(__clang__)
  #ifndef JSTD_IS_GCC
  #define JSTD_IS_GCC     1
  #endif
#elif defined(__clang__)
  #ifndef JSTD_IS_CLANG
  #define JSTD_IS_CLANG   1
  #endif
#elif !defined(JSTD_IS_ICC)
  #ifndef JSTD_IS_UNKNOWN_COMPILER
  #define JSTD_IS_UNKNOWN_COMPILER   1
  #endif
#endif // _MSC_VER

#if defined(__GNUC__) || defined(__clang__) || (defined(JSTD_IS_ICC) && defined(__linux__))
#define JSTD_GCC_STYLE_ASM  1
#endif

//
// Intel C++ compiler version
//
#if defined(__INTEL_COMPILER)
  #if (__INTEL_COMPILER == 9999)
    #define __INTEL_CXX_VERSION     1200    // Intel's bug in 12.1.
  #else
    #define __INTEL_CXX_VERSION     __INTEL_COMPILER
  #endif
#elif defined(__ICL)
#  define __INTEL_CXX_VERSION       __ICL
#elif defined(__ICC)
#  define __INTEL_CXX_VERSION       __ICC
#elif defined(__ECC)
#  define __INTEL_CXX_VERSION       __ECC
#endif

////////////////////////////////////////////////////////////////////////////////
//
// Clang Language Extensions
//
// See: http://clang.llvm.org/docs/LanguageExtensions.html#checking_language_features
//
////////////////////////////////////////////////////////////////////////////////

#ifndef __has_builtin                               // Optional of course.
  #define __has_builtin(x)              0           // Compatibility with non-clang compilers.
#endif

#ifndef __has_feature                               // Optional of course.
  #define __has_feature(x)              0           // Compatibility with non-clang compilers.
#endif

#ifndef __has_extension
  #define __has_extension               __has_feature   // Compatibility with pre-3.0 compilers.
#endif

#ifndef __has_cpp_attribute                         // Optional of course.
  #define __has_cpp_attribute(x)        0           // Compatibility with non-clang compilers.
#endif

#ifndef __has_c_attribute                           // Optional of course.
  #define __has_c_attribute(x)          0           // Compatibility with non-clang compilers.
#endif

#ifndef __has_attribute                             // Optional of course.
  #define __has_attribute(x)            0           // Compatibility with non-clang compilers.
#endif

#ifndef __has_declspec_attribute                    // Optional of course.
  #define __has_declspec_attribute(x)   0           // Compatibility with non-clang compilers.
#endif

#ifndef __is_identifier                             // Optional of course.
  // It evaluates to 1 if the argument x is just a regular identifier and not a reserved keyword.
  #define __is_identifier(x)            1           // Compatibility with non-clang compilers.
#endif

#if defined(_MSC_VER)
#ifndef __attribute__
  #define __attribute__(x)
#endif
#endif

////////////////////////////////////////////////////////////////////////////////

#if __has_feature(cxx_rvalue_references)
    // This code will only be compiled with the -std=c++11 and -std=gnu++11
    // options, because rvalue references are only standardized in C++11.
#endif

#if __has_extension(cxx_rvalue_references)
    // This code will be compiled with the -std=c++11, -std=gnu++11, -std=c++98
    // and -std=gnu++98 options, because rvalue references are supported as a
    // language extension in C++98.
#endif

////////////////////////////////////////////////////////////////////////////////

#if defined(_MSC_VER) || __has_declspec_attribute(dllexport)
  #define DLL_EXPORT        __declspec(dllexport)
#else
  #if defined(__GNUC__) || defined(__clang__) || defined(__linux__)
    #define DLL_EXPORT      __attribute__((visibility("default")))
  #else
    #define DLL_EXPORT
  #endif
#endif

#if defined(_MSC_VER) || __has_declspec_attribute(dllimport)
  #define DLL_IMPORT        __declspec(dllimport)
#else
  #define DLL_IMPORT
#endif

#if __is_identifier(__wchar_t)
    // __wchar_t is not a reserved keyword
  #if !defined(_MSC_VER)
    typedef wchar_t __wchar_t;
  #endif // !_MSC_VER
#endif

////////////////////////////////////////////////////////////////////////////////
//
// C++ compiler macro define
// See: http://www.cnblogs.com/zyl910/archive/2012/08/02/printmacro.html
//

//
// Using unlikely/likely macros from boost?
// See: http://www.boost.org/doc/libs/1_60_0/boost/config/compiler/clang.hpp
//
// See:
//      http://www.boost.org/doc/libs/1_60_0/boost/config/compiler/gcc.hpp10
//      http://www.boost.org/doc/libs/1_60_0/boost/config/compiler/clang.hpp4
//      http://www.boost.org/doc/libs/1_60_0/boost/config/compiler/intel.hpp2
// 
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
//
// LLVM Branch Weight Metadata
// See: http://llvm.org/docs/BranchWeightMetadata.html
//
////////////////////////////////////////////////////////////////////////////////

//
// Since gcc 2.96 or Intel C++ compiler 8.0
//
// I'm not sure Intel C++ compiler 8.0 was the first version to support these builtins,
// update the condition if the version is not accurate. (Andrey Semashev)
//
#if (defined(__GNUC__) && ((__GNUC__ == 2 && __GNUC_MINOR__ >= 96) || (__GNUC__ >= 3))) \
    || (defined(__GNUC__) && (__INTEL_CXX_VERSION >= 800))
  #define SUPPORT_LIKELY        1
#elif defined(__clang__)
  //
  // clang: GCC extensions not implemented yet
  // See: http://clang.llvm.org/docs/UsersManual.html#gcc-extensions-not-implemented-yet
  //
  #if defined(__has_builtin)
    #if __has_builtin(__builtin_expect)
      #define SUPPORT_LIKELY    1
    #endif // __has_builtin(__builtin_expect)
  #endif // defined(__has_builtin)
#endif // SUPPORT_LIKELY

//
// Sample: since clang 3.4
//
#if defined(__clang__) && (__clang_major__ > 3 || (__clang_major__ == 3 && __clang_minor__ >= 4))
    // This is a sample macro.
#endif

//
// Branch prediction hints
//
#if defined(SUPPORT_LIKELY) && (SUPPORT_LIKELY != 0)
#ifndef likely
#define likely(x)               __builtin_expect(!!(x), 1)
#endif
#ifndef unlikely
#define unlikely(x)             __builtin_expect(!!(x), 0)
#endif
#ifndef switch_likely
#define switch_likely(x, v)     __builtin_expect(!!(x), (v))
#endif
#else
#ifndef likely
#define likely(x)               (x)
#endif
#ifndef unlikely
#define unlikely(x)             (x)
#endif
#ifndef switch_likely
#define switch_likely(x, v)     (x)
#endif
#endif // likely() & unlikely()

//
// Aligned prefix and suffix declare
//
#if defined(_MSC_VER) || defined(__INTEL_COMPILER) || defined(__ICL) || defined(__ICC) || defined(__ECC)
#ifndef ALIGNED_PREFIX
#define ALIGNED_PREFIX(n)       __declspec(align(n))
#endif
#ifndef ALIGNED_SUFFIX
#define ALIGNED_SUFFIX(n)
#endif
#else
#ifndef ALIGNED_PREFIX
#define ALIGNED_PREFIX(n)
#endif
#ifndef ALIGNED_SUFFIX
#define ALIGNED_SUFFIX(n)       __attribute__((aligned(n)))
#endif
#endif // ALIGNED(n)

#if defined(__GNUC__) && !defined(__GNUC_STDC_INLINE__) && !defined(__GNUC_GNU_INLINE__)
  #define __GNUC_GNU_INLINE__   1
#endif

/**
 * For inline, force-inline and no-inline define.
 */
#if defined(_MSC_VER) || defined(__INTEL_COMPILER) || defined(__ICL) || defined(__ICC) || defined(__ECC)

#define JSTD_HAS_INLINE                     1

#define JSTD_INLINE                         __inline
#define JSTD_FORCE_INLINE                   __forceinline
#define JSTD_NO_INLINE                      __declspec(noinline)

#define JSTD_INLINE_DECLARE(type)           __inline type
#define JSTD_FORCE_INLINE_DECLARE(type)     __forceinline type
#define JSTD_NOINLINE_DECLARE(type)         __declspec(noinline) type

#define JSTD_CRT_INLINE                     extern __inline
#define JSTD_CRT_FORCE_INLINE               extern __forceinline
#define JSTD_CRT_NO_INLINE                  extern __declspec(noinline)

#define JSTD_CRT_INLINE_DECLARE(type)       extern __inline type
#define JSTD_CRT_FORCE_INLINE_DECLARE(type) extern __forceinline type
#define JSTD_CRT_NO_INLINE_DECLARE(type)    extern __declspec(noinline) type

#define JSTD_RESTRICT                       __restrict

#elif defined(__GNUC__) || defined(__clang__) || defined(__MINGW32__) || defined(__CYGWIN__)

#define JSTD_HAS_INLINE                     1

#define JSTD_INLINE                         inline __attribute__((gnu_inline))
#define JSTD_FORCE_INLINE                   inline __attribute__((always_inline))
#define JSTD_NO_INLINE                      __attribute__((noinline))

#define JSTD_INLINE_DECLARE(type)           inline __attribute__((gnu_inline)) type
#define JSTD_FORCE_INLINE_DECLARE(type)     inline __attribute__((always_inline)) type
#define JSTD_NOINLINE_DECLARE(type)         __attribute__((noinline)) type

#define JSTD_CRT_INLINE                     extern inline __attribute__((gnu_inline))
#define JSTD_CRT_FORCE_INLINE               extern inline __attribute__((always_inline))
#define JSTD_CRT_NO_INLINE                  extern __attribute__((noinline))

#define JSTD_CRT_INLINE_DECLARE(type)       extern inline __attribute__((gnu_inline)) type
#define JSTD_CRT_FORCE_INLINE_DECLARE(type) extern inline __attribute__((always_inline)) type
#define JSTD_CRT_NO_INLINE_DECLARE(type)    extern __attribute__((noinline)) type

#define JSTD_RESTRICT                       __restrict__

#else // Unknown compiler

#define JSTD_INLINE                         inline
#define JSTD_FORCE_INLINE                   inline
#define JSTD_NO_INLINE

#define JSTD_INLINE_DECLARE(type)           inline type
#define JSTD_FORCE_INLINE_DECLARE(type)     inline type
#define JSTD_NOINLINE_DECLARE(type)         type

#define JSTD_CRT_INLINE                     extern inline
#define JSTD_CRT_FORCE_INLINE               extern inline
#define JSTD_CRT_NO_INLINE                  extern

#define JSTD_CRT_INLINE_DECLARE(type)       extern inline type
#define JSTD_CRT_FORCE_INLINE_DECLARE(type) extern inline type
#define JSTD_CRT_NO_INLINE_DECLARE(type)    extern type

#define JSTD_RESTRICT

#endif // _MSC_VER

#if defined(_MSC_VER)
#define NAKED_DECL  __declspec(naked)
#elif defined(__attribute__)
#define NAKED_DECL  __attribute__((naked))
#else
#define NAKED_DECL
#endif

#ifndef JSTD_CDECL
#if defined(_MSC_VER) || defined(__INTEL_COMPILER) || defined(__ICL) || defined(__ICC) || defined(__ECC)
#define JSTD_CDECL        __cdecl
#else
#define JSTD_CDECL        __attribute__((__cdecl__))
#endif
#endif // JSTD_CDECL

/**
 * For exported func
 */
#if defined(_MSC_VER) && (_MSC_VER >= 1400)
    #define JSTD_EXPORTED_FUNC      __cdecl
    #define JSTD_EXPORTED_METHOD    __thiscall
#else
    #define JSTD_EXPORTED_FUNC
    #define JSTD_EXPORTED_METHOD
#endif

#ifndef __cplusplus
  #ifndef nullptr
    #define nullptr     ((void *)NULL)
  #endif
#endif // __cplusplus

#ifndef JSTD_UNUSED_VARS
#define JSTD_UNUSED_VARS(x)     (void)(x)
#endif

#ifndef JSTD_TO_STRING
#define JSTD_TO_STRING(Text)    #Text
#endif

#define STD_IOS_RIGHT(width, var) \
    std::right << std::setw(width) << (var)

#define STD_IOS_LEFT(width, var) \
    std::left << std::setw(width) << (var)

#define STD_IOS_DEFAULT() \
    std::left << std::setw(0)

#define UNUSED_VARIANT(var) \
    do { \
        (void)var; \
    } while (0)

#ifndef JSTD_ASSERT
#ifdef _DEBUG
#define JSTD_ASSERT(express)            assert(!!(express))
#else
#define JSTD_ASSERT(express)            (void)0
#endif
#endif // JSTD_ASSERT

#ifndef JSTD_ASSERT_EX
#ifdef _DEBUG
#define JSTD_ASSERT_EX(express, text)   assert(!!(express))
#else
#define JSTD_ASSERT_EX(express, text)   (void)0
#endif
#endif // JSTD_ASSERT

#ifndef JSTD_STATIC_ASSERT
#if (__cplusplus < 201103L) || (defined(_MSC_VER) && (_MSC_VER < 1800))
#define JSTD_STATIC_ASSERT(express, text)       assert(!!(express))
#else
#define JSTD_STATIC_ASSERT(express, text)       static_assert(!!(express), text)
#endif
#endif // JSTD_STATIC_ASSERT

//
// Little-Endian or Big-Endian
//
#define JSTD_LITTLE_ENDIAN  0
#define JSTD_BIG_ENDIAN     1

#ifndef JSTD_ENDIAN
#define JSTD_ENDIAN  JSTD_LITTLE_ENDIAN
#endif

#endif // JSTD_BASIC_STDDEF_H
