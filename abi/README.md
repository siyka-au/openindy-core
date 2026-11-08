# Plugin ABI baseline

`libOpenIndyCore` is the binary contract between the host and every plugin. The
runtime check is the integer `OI_PLUGIN_ABI_VERSION`
([`include/plugin/plugin_abi.h`](../include/plugin/plugin_abi.h)), compared by the
plugin entry point handshake and `oi::PluginLoader` before a factory is created.
This directory holds what makes sure that integer is bumped when it must be.

| File | Purpose |
| --- | --- |
| `baseline.abi` | libabigail abixml of the public ABI (declarations from `include/`) at `baseline.version` |
| `baseline.version` | the `OI_PLUGIN_ABI_VERSION` the baseline was generated for |
| `suppressions.abignore` | host-only types hidden from baseline and comparison |
| `suppressions-diff.abignore` | standard-library differences ignored by abidiff only (compiler noise) |

## Rule

An ABI break detected by `abidiff` must come with an `OI_PLUGIN_ABI_VERSION`
bump, and the baseline is refreshed **in the same commit**:

1. Change the headers; run the check (below). If it reports an incompatible
   change, bump `OI_PLUGIN_ABI_VERSION`.
2. Run `scripts/abi-check.sh --update` and commit `abi/baseline.abi` and
   `abi/baseline.version` together with the bump.

`scripts/abi-check.sh` fails when (a) there is an incompatible change and the
version equals `baseline.version`, (b) the version differs from
`baseline.version` (baseline not refreshed, even if abidiff sees no change), or
(c) the baseline files are missing. Compatible changes (e.g. an added
non-virtual function) only print a note.

## Running it

Needs libabigail (`abidw`, `abidiff`; Debian/Ubuntu `abigail-tools`, Arch
`libabigail`) and a core build **with debug info**. `-g` on top of Release keeps
the Conan packages ABI-compatible (a Debug build does not):

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS=-g \
      -DCMAKE_TOOLCHAIN_FILE=build/conan/conan_toolchain.cmake
cmake --build build --target OpenIndyCore
scripts/abi-check.sh               # compare against the baseline
scripts/abi-check.sh --update      # rewrite baseline.abi / baseline.version
```

Optionally configure with `-DOI_ABI_CHECK=ON` for an `abi_check` ctest entry and
the `abi-check` / `abi-update` targets (only registered when `abidw` and
`abidiff` are found; never part of the normal build or ctest).

## Compiler caveat

The abixml depends slightly on the compiler's DWARF. The baseline is produced
with the compiler CI uses (GCC 14, see `.github/workflows/ci.yml`); the `abi` CI
job uploads the freshly generated `current.abi` as an artifact, so the baseline
can be refreshed from CI output if a local compiler differs.
