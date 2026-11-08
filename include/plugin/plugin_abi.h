#ifndef OI_CORE_PLUGIN_PLUGIN_ABI_H_
#define OI_CORE_PLUGIN_PLUGIN_ABI_H_

/*!
 * \file plugin_abi.h
 * \brief The OpenIndy plugin ABI version - the compatibility contract between
 * the core library and every plugin binary.
 *
 * This number is DECLARED BY HAND. It is deliberately not derived from
 * git, the product version, or anything else automatic.
 *
 * The reason: no automatic source can answer the only question that matters
 * here - "did the contract change?". A commit that fixes a comment and a
 * commit that adds a pure virtual to Sensor are indistinguishable to git,
 * but only one of them invalidates every plugin ever built. Deriving this
 * from a commit count (which is what it used to do) meant the ABI version
 * churned on every core commit whether or not anything broke, and could even
 * run backwards on a revert.
 *
 * It also lives in a header rather than a CMake compile definition on
 * purpose. A header is a tracked source dependency, so changing it forces
 * every plugin that includes it to rebuild. A compile definition is
 * evaluated at configure time, so a plugin build tree configured before the
 * change kept its old value and produced a stale binary that silently failed
 * to load - the exact failure this replaces.
 *
 * ############################
 * WHEN TO BUMP THIS
 * ############################
 *
 * Bump for any change that alters what a plugin binary compiled against the
 * previous version would do:
 *
 *  - adding, removing, or reordering a virtual function in any interface in
 *    include/plugin/ (this moves every subsequent vtable slot)
 *  - changing any of those functions' signatures or return types
 *  - adding, removing, or reordering data members of a type passed across
 *    the boundary by value or by reference (InputElement, NeededElement,
 *    PluginMetaData, MeasurementConfig, Position, Direction, ...)
 *  - changing the underlying value of an existing enumerator, or inserting
 *    one anywhere other than at the end
 *  - changing a struct's alignment, packing or base classes
 *
 * You do NOT need to bump for: implementation-only changes in src/, new
 * non-virtual free functions, appending an enumerator at the end of an enum
 * whose values are never persisted numerically, or comments.
 *
 * If you are unsure, bump it. A spurious bump costs a rebuild. A missed bump
 * lets a mismatched plugin load and corrupt memory at runtime.
 *
 * scripts/abi-check.sh (libabigail's abidiff against abi/baseline.abi, run by
 * CI) guards this: it fails when the public ABI changes incompatibly without
 * this number moving, and when this number moves without the baseline being
 * refreshed (scripts/abi-check.sh --update) in the same commit. See
 * abi/README.md.
 */
#include <string>
#include <string_view>

inline constexpr int kPluginAbiVersion = 19;

namespace oi {

//! The interface identifier ("iid") of a plugin interface: `prefix` followed by
//! the ABI version, so an interface revision never matches an older one.
[[nodiscard]] inline std::string pluginIid(std::string_view prefix) {
  return std::string(prefix) + std::to_string(kPluginAbiVersion);
}

}  // namespace oi

#endif  // OI_CORE_PLUGIN_PLUGIN_ABI_H_
