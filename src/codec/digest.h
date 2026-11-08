#ifndef OI_CORE_SRC_CODEC_DIGEST_H_
#define OI_CORE_SRC_CODEC_DIGEST_H_

#include <cstdint>
#include <string_view>

namespace oi::codec {

/*!
 * 64-bit FNV-1a over a byte string.
 *
 * The project digest only answers "has the project changed since it was last
 * saved?", so it needs to be stable and cheap, not cryptographic. It must stay
 * stable across builds and platforms because the value is stored in the file
 * (which rules out std::hash).
 */
[[nodiscard]] constexpr std::uint64_t fnv1a64(std::string_view bytes) noexcept {
  constexpr std::uint64_t kOffsetBasis =
      14695981039346656037ULL;  // FNV-1a 64-bit
  constexpr std::uint64_t kPrime = 1099511628211ULL;
  std::uint64_t hash = kOffsetBasis;
  for (const char c : bytes) {
    hash ^= static_cast<std::uint8_t>(c);
    hash *= kPrime;
  }
  return hash;
}

}  // namespace oi::codec

#endif  // OI_CORE_SRC_CODEC_DIGEST_H_
