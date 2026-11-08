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
 *    available as oi::kToolchainCompiler for diagnostics only.
 *
 * Flags that change layout (-D_GLIBCXX_USE_CXX11_ABI, -fno-rtti, sanitizers, a
 * different -std for header types) cannot be seen from here; see
 * docs/plugin-abi.md.
 *
 * Define OI_TOOLCHAIN_OVERRIDE (a macro the build passes with -D) to a string
 * literal to replace the whole string (only the tests do that, to build a
 * plugin of a "foreign" toolchain).
 */

#include <array>
#include <climits>
#include <cstddef>
#include <string_view>
#include <version>  // defines the standard library's identifying macros

namespace oi {

// Spelled like the "compiler" field of a plugin's metadata (the plugins' CMake
// derives the same words from CMAKE_CXX_COMPILER_ID).
#if defined(__MINGW32__)
inline constexpr std::string_view kToolchainCompiler = "mingw";
#elif defined(_MSC_VER) && !defined(__clang__)
inline constexpr std::string_view kToolchainCompiler = "msvc";
#elif defined(__clang__)
inline constexpr std::string_view kToolchainCompiler = "clang";
#elif defined(__GNUC__)
inline constexpr std::string_view kToolchainCompiler = "gcc";
#else
inline constexpr std::string_view kToolchainCompiler = "unknown";
#endif

#if defined(_LIBCPP_VERSION)
inline constexpr std::string_view kToolchainStdlib = "libc++";
#elif defined(__GLIBCXX__)
inline constexpr std::string_view kToolchainStdlib = "libstdc++";
#elif defined(_MSVC_STL_VERSION)
inline constexpr std::string_view kToolchainStdlib = "msvc-stl";
#else
inline constexpr std::string_view kToolchainStdlib = "unknown-stl";
#endif

#if defined(_WIN32) && defined(_MSC_VER)
inline constexpr std::string_view kToolchainCxxAbi = "msvc";
#else
inline constexpr std::string_view kToolchainCxxAbi =
    "itanium";  // GCC, clang, MinGW
#endif

#if defined(__linux__)
inline constexpr std::string_view kToolchainOs = "linux";
#elif defined(_WIN32)
inline constexpr std::string_view kToolchainOs = "windows";
#elif defined(__APPLE__)
inline constexpr std::string_view kToolchainOs = "macos";
#else
inline constexpr std::string_view kToolchainOs = "unknown-os";
#endif

#if defined(__x86_64__) || defined(_M_X64)
inline constexpr std::string_view kToolchainArch = "x86_64";
#elif defined(__aarch64__) || defined(_M_ARM64)
inline constexpr std::string_view kToolchainArch = "aarch64";
#elif defined(__i386__) || defined(_M_IX86)
inline constexpr std::string_view kToolchainArch = "x86";
#elif defined(__arm__) || defined(_M_ARM)
inline constexpr std::string_view kToolchainArch = "arm";
#else
inline constexpr std::string_view kToolchainArch = "unknown-arch";
#endif

inline constexpr std::size_t kBits64 = 64;
inline constexpr std::size_t kBits32 = 32;

// Sanity: the architecture name must agree with the pointer width, and the
// layout assumptions of the codec/protocol are little endian.
#if defined(__x86_64__) || defined(_M_X64) || defined(__aarch64__) || \
    defined(_M_ARM64)
static_assert(sizeof(void*) * CHAR_BIT == kBits64,
              "64-bit architecture with non-64-bit pointers");
#elif defined(__i386__) || defined(_M_IX86) || defined(__arm__) || \
    defined(_M_ARM)
static_assert(sizeof(void*) * CHAR_BIT == kBits32,
              "32-bit architecture with non-32-bit pointers");
#endif
#if defined(__BYTE_ORDER__) && defined(__ORDER_LITTLE_ENDIAN__)
static_assert(__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__,
              "big-endian targets are not supported");
#endif

namespace detail {

//! Capacity of the buffer that holds the toolchain string (with its NUL).
inline constexpr std::size_t kToolchainCapacity = 64;

//! "<c++ abi>-<standard library>-<os>-<architecture>" as a NUL-terminated
//! array, built at compile time.
[[nodiscard]] constexpr std::array<char, kToolchainCapacity> joinToolchain() {
  std::array<char, kToolchainCapacity> out{};
  std::size_t n = 0;
  const auto append = [&](std::string_view part) {
    for (const char c : part) {
      out.at(n++) = c;
    }
  };
  append(kToolchainCxxAbi);
  append("-");
  append(kToolchainStdlib);
  append("-");
  append(kToolchainOs);
  append("-");
  append(kToolchainArch);
  return out;
}

inline constexpr std::array<char, kToolchainCapacity> kToolchainStorage =
    joinToolchain();

}  // namespace detail

#ifdef OI_TOOLCHAIN_OVERRIDE
inline constexpr const char* kToolchain = OI_TOOLCHAIN_OVERRIDE;
#else
inline constexpr const char* kToolchain = detail::kToolchainStorage.data();
#endif

}  // namespace oi

#endif  // OI_CORE_PLUGIN_TOOLCHAIN_H_
