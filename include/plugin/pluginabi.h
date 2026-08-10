#ifndef PLUGINABI_H
#define PLUGINABI_H

/*!
 * \file pluginabi.h
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
 * tst_pluginabi guards this: it fails when the interface headers change
 * without this number moving. Read that test's output before overriding it.
 */
#define OI_PLUGIN_ABI_VERSION 2

#endif // PLUGINABI_H
