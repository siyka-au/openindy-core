#ifndef OI_CORE_PLUGIN_TOOLCHAIN_H_
#define OI_CORE_PLUGIN_TOOLCHAIN_H_

/*!
 * \file toolchain.h
 * \brief The toolchain string a plugin exports (oi_plugin_toolchain) and the
 * loader compares with its own.
 *
 * The plugin interface passes std types and virtual calls across the library
 * boundary, so host and plugin must agree on the C++ ABI, the standard
 * library, the OS and the architecture. The string is
 *
 *     <c++ abi>-<standard library>-<os>-<architecture>
 *
 * for example "itanium-libstdc++-linux-x86_64". It is built at compile time
 * from predefined macros. Deliberately NOT part of it:
 *
 *  - the compiler VERSION: libstdc++ is backward compatible, so a plugin built
 *    with an older GCC loads on a host with a newer libstdc++ runtime;
 *  - the compiler NAME on the Itanium ABI: GCC and clang with libstdc++ on
 *    Linux interoperate, so both give "itanium-libstdc++-...". The name is
 *    available as OI_TOOLCHAIN_COMPILER for diagnostics only.
 *
 * Flags that change layout (-D_GLIBCXX_USE_CXX11_ABI, -fno-rtti, sanitizers, a
 * different -std for header types) cannot be seen from here; see
 * docs/plugin-abi.md.
 *
 * Define OI_TOOLCHAIN_OVERRIDE to a string literal to replace the whole string
 * (only the tests do that, to build a plugin of a "foreign" toolchain).
 */

#include <version>  // defines the standard library's identifying macros

// Spelled like the "compiler" field of a plugin's metadata (the plugins' CMake
// derives the same words from CMAKE_CXX_COMPILER_ID).
#if defined(__MINGW32__)
#define OI_TOOLCHAIN_COMPILER "mingw"
#elif defined(_MSC_VER) && !defined(__clang__)
#define OI_TOOLCHAIN_COMPILER "msvc"
#elif defined(__clang__)
#define OI_TOOLCHAIN_COMPILER "clang"
#elif defined(__GNUC__)
#define OI_TOOLCHAIN_COMPILER "gcc"
#else
#define OI_TOOLCHAIN_COMPILER "unknown"
#endif

#if defined(_LIBCPP_VERSION)
#define OI_TOOLCHAIN_STDLIB "libc++"
#elif defined(__GLIBCXX__)
#define OI_TOOLCHAIN_STDLIB "libstdc++"
#elif defined(_MSVC_STL_VERSION)
#define OI_TOOLCHAIN_STDLIB "msvc-stl"
#else
#define OI_TOOLCHAIN_STDLIB "unknown-stl"
#endif

#if defined(_WIN32) && defined(_MSC_VER)
#define OI_TOOLCHAIN_CXX_ABI "msvc"
#else
#define OI_TOOLCHAIN_CXX_ABI "itanium"  // GCC, clang, MinGW
#endif

#if defined(__linux__)
#define OI_TOOLCHAIN_OS "linux"
#elif defined(_WIN32)
#define OI_TOOLCHAIN_OS "windows"
#elif defined(__APPLE__)
#define OI_TOOLCHAIN_OS "macos"
#else
#define OI_TOOLCHAIN_OS "unknown-os"
#endif

#if defined(__x86_64__) || defined(_M_X64)
#define OI_TOOLCHAIN_ARCH "x86_64"
#elif defined(__aarch64__) || defined(_M_ARM64)
#define OI_TOOLCHAIN_ARCH "aarch64"
#elif defined(__i386__) || defined(_M_IX86)
#define OI_TOOLCHAIN_ARCH "x86"
#elif defined(__arm__) || defined(_M_ARM)
#define OI_TOOLCHAIN_ARCH "arm"
#else
#define OI_TOOLCHAIN_ARCH "unknown-arch"
#endif

// Sanity: the architecture name must agree with the pointer width, and the
// layout assumptions of the codec/protocol are little endian.
#if defined(__x86_64__) || defined(_M_X64) || defined(__aarch64__) || \
    defined(_M_ARM64)
static_assert(sizeof(void*) == 8, "64-bit architecture with non-64-bit pointers");
#elif defined(__i386__) || defined(_M_IX86) || defined(__arm__) || \
    defined(_M_ARM)
static_assert(sizeof(void*) == 4, "32-bit architecture with non-32-bit pointers");
#endif
#if defined(__BYTE_ORDER__) && defined(__ORDER_LITTLE_ENDIAN__)
static_assert(__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__,
              "big-endian targets are not supported");
#endif

#ifdef OI_TOOLCHAIN_OVERRIDE
#define OI_TOOLCHAIN OI_TOOLCHAIN_OVERRIDE
#else
#define OI_TOOLCHAIN                                             \
  OI_TOOLCHAIN_CXX_ABI "-" OI_TOOLCHAIN_STDLIB "-" OI_TOOLCHAIN_OS \
                       "-" OI_TOOLCHAIN_ARCH
#endif

#endif  // OI_CORE_PLUGIN_TOOLCHAIN_H_
