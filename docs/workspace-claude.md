# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Workspace layout

This directory is **not** a single git repo — it's a workspace of six independent sibling repos, each
with its own `origin` remote (`git@github.com:siyka-au/<name>.git`, forked from the upstream `OpenIndy/*`
projects). Build order follows the dependency chain:

- `openindy-math/` — thin C++ wrapper around a linear-algebra backend (Armadillo today; swappable via
  `ChooseLALib`). No internal dependencies. **Qt-free.**
- `openindy-core/` — the kernel: job/feature/element data model, the plugin interfaces (`Sensor`,
  `Function`, `BundleAdjustment`, `SimulationModel`, exchange formats), the protobuf codec, the std-only
  `PluginLoader`, progress (`progress.h`) and logging (`log.h`) facades, sensor I/O threading.
  Depends on `openindy-math`. **Qt-free**: no Qt include, no Qt link, no moc; it configures and builds with
  Qt hidden (`-DCMAKE_DISABLE_FIND_PACKAGE_Qt6=ON`) and `scripts/check-no-qt.sh` (also a ctest, `no_qt`)
  fails on any Qt reference in core/math include, src or test.
- `openindy-default-plugin/` — the standard plugin: fit/construct/transform functions, tachymeter and
  laser-tracker sensor drivers, ASCII import/export. Depends on `openindy-core`. Built and loaded as a
  separate shared library, not linked into the app. May use Qt internally.
- `openindy-leica-plugin/` — the Leica GeoCom tachymeter sensor plugin (same structure, uses Qt serial port).
- `openindy/` — the Qt Widgets desktop application (`OpenIndyApp` library + `openIndy` executable):
  controllers, Qt models/proxy models, dialogs, the Qt adapters of the core's std-only callbacks. Depends on
  `openindy-core` (which pulls in `openindy-math` transitively). This is where Qt lives.

- `openindy-py/` — Python library (nanobind extension + pure-Python API): `openindy_py.fit_plane/...`
  best-fit geometry through the core and the default plugin, plus an optional Session/Job project
  API. Embeds `openindy-core`, `openindy-math` and `openindy-default-plugin` from the sibling
  checkouts. See "openindy-py" below.

Each of `openindy`, `openindy-core`, and `openindy-default-plugin`'s `CMakeLists.txt` resolves its
upstream dependency via `FetchContent`, preferring a **local sibling checkout** over cloning from git:
it checks for `../openindy-core` / `../openindy-math` relative to itself and uses that source tree if
present, only falling back to `GIT_REPOSITORY` otherwise. As long as you keep these repos as
siblings in one parent directory (as they are here), builds use your local checkouts of all of them —
so a change made in `openindy-core` is picked up the next time you rebuild `openindy` without pushing
anything. The app and the plugins find Qt themselves (`find_package(Qt6 ...)`); core does not provide it.

Note: the top-level READMEs in each repo (and Qt Creator `.pro`/`.pri` files still present in
`openindy/test/*`) describe the **old qmake + Qt5 build**. The project has since been migrated to
**CMake + Qt6 + C++23**; the `CMakeLists.txt` in each repo root is the source of truth, not the READMEs.
The `openindy/test/*.pro` subdirectories are leftover qmake test scaffolding.

## Platform and plugin ABI policy

Linux only for now. Windows (MinGW or MSVC, never mixed within one build) only if a user base emerges; nothing
beyond leaving room in the toolchain string is designed for it. Do not over-engineer for a user base that may never form.

The plugin interface passes std types across the library boundary, so the plugin ABI is only defined for the same OS
and toolchain family. Host and plugin must share OS, compiler family and standard library (in practice libstdc++;
GCC and clang with libstdc++ both follow the Itanium ABI and are compatible). libstdc++ is backward compatible: a
plugin built with the oldest supported GCC loads on a host with a newer libstdc++ runtime, never the reverse, so
build distributed plugins with the floor compiler. Flags that change layout (`-D_GLIBCXX_USE_CXX11_ABI`, `-fno-rtti`,
sanitizers, a different `-std` for header types) break compatibility. MSVC, MinGW and Linux toolchains are mutually
incompatible. Full text: `openindy-core/docs/plugin-abi.md`.

Enforcement: besides `kPluginAbiVersion`, every plugin exports `oi_plugin_toolchain()` (e.g.
`itanium-libstdc++-linux-x86_64`: C++ ABI, standard library, OS, architecture, built from predefined macros in
`include/plugin/toolchain.h`; compiler version and name are deliberately not compared). `oi::PluginLoader` checks
the version first, then the toolchain, and refuses a mismatch (`kToolchainMismatch`) or a missing symbol. The
plugin metadata `compiler` field (informational) is `gcc`/`clang`/`mingw`/`msvc`, the same words in
`OI_TOOLCHAIN_COMPILER`, `OiMetaData::getCompiler()` and both plugins' `CMakeLists.txt`.

Compiler baseline: **GCC 14 is the floor** (Debian 13 default; g++-14 installable on Ubuntu 24.04 / Mint 22; has the
C++23 library features used, checked by grep: no `std::print`, `ranges::to`, `views::zip/enumerate`, deducing this,
flat containers etc.; only `std::expected`, `std::format`, `std::jthread`, `chrono::year_month_day`, a few
`std::ranges` algorithms). Core CI builds and tests with GCC 14, 15, 16; GCC 13 runs informationally
(`continue-on-error`). The ABI baseline is generated with GCC 14 (built in a `gcc:14` Docker container, abidw run locally); if the first CI `abi` run disagrees, refresh it from that run's `abi-current` artifact.

## Build

Requires a C++23 compiler (GCC 14 or newer), CMake, Ninja, [uv](https://docs.astral.sh/uv/) (to run Conan), Qt6 (Core, Gui,
Widgets, Svg, SerialPort, Network, WebSockets, Test) for the app and plugins, and Armadillo + BLAS/LAPACK
(for `openindy-math`). The third-party packages of the core (protobuf, spdlog, Catch2) come from Conan
(`openindy-core/conanfile.txt`); Qt and Armadillo come from the system.

Install the Conan packages once (the profile pins the compiler settings; `--build=missing` builds
anything without a binary package; `protoc` comes from Conan too and must match the library exactly):

```sh
cd openindy-core
uvx conan install . -pr:a conan/profiles/linux-gcc --build=missing -of build/conan
```

Then configure with the Conan toolchain. **Use a Release build**: the Conan packages are Release, mixing
in a Debug build of the project breaks the protobuf/abseil ABI.

```sh
# the Qt application and (transitively) core and math from the sibling checkouts
cd openindy
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_TOOLCHAIN_FILE=../openindy-core/build/conan/conan_toolchain.cmake
cmake --build build -j$(nproc)
```

Produces `build/bin/openIndy` plus `libOpenIndyApp.so`, `libOpenIndyCore.so`, `libOpenIndyMath.so`. A
post-build step copies `res-rt/config`, `res-rt/example`, `res-rt/plugins`, and `db/oisystemdb.sqlite`
into `build/bin/`, so the built binary is directly runnable without a separate install step. The plugins
are built alongside (the app's CMake pulls them in) and deployed to `build/bin/plugins`.

Core alone (Qt-free, with the Catch2 tests and the `oi-cli` example):

```sh
cd openindy-core
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_TOOLCHAIN_FILE=build/conan/conan_toolchain.cmake
cmake --build build -j$(nproc) && (cd build && ctest --output-on-failure)
```

Plugins separately (each produces a shared library that is not linked into the app):

```sh
cd openindy-default-plugin   # or openindy-leica-plugin
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_TOOLCHAIN_FILE=../openindy-core/build/conan/conan_toolchain.cmake
cmake --build build -j$(nproc)
```

## Test

Core uses **Catch2** (Qt-free); the app and the plugins use Qt Test. All use CTest, one ctest entry per test
binary. Run all tests for a given repo from its own build directory (the app's need
`QT_QPA_PLATFORM=offscreen` without a display):

```sh
cd openindy-core/build && ctest --output-on-failure
cd openindy-default-plugin/build && ctest --output-on-failure
```

Run a single test binary directly for tighter iteration (e.g. `tst_reading`, `tst_function`,
`tst_loadplugin`, `tst_oiexchangeascii`):

```sh
./build/bin/tst_reading
```

Because `openindy`'s build tree `FetchContent`-includes `openindy-core` as a subdirectory,
`openindy-core`'s tests (e.g. `tst_reading`) also get built and registered under `openindy/build/`, so
`ctest` from there picks them up too. The app also has its own Qt Test binaries (`tst_pluginhost`,
`tst_dataexchanger`, ...). The plugin ABI is guarded by libabigail, not by a ctest entry (see "CI and the plugin ABI check" below).
`openindy-core/build/bin/oi-cli --selftest --plugin <libp_defaultPlugin.so>` is the end-to-end example (load
plugin, fit, save and reload protobuf).

## CI and the plugin ABI check

Every repo has `.github/workflows/ci.yml` (Ubuntu 24.04, GCC 14 as the floor, `permissions: contents: read`, superseded runs
cancelled). Repos that need siblings check them out side by side in the workspace dir, matching the layout the
CMake `FetchContent` logic expects; if a sibling is private, add a `SIBLING_REPO_TOKEN` secret (read access).
Conan runs through `uvx conan` with `-s:a compiler.version=14`; the Conan cache is cached per conanfile+profile.

- `openindy-core`: `build-test` (Release, Conan, ctest; matrix over GCC 14 on the runner, GCC 15 and 16 in the official
  `gcc:15`/`gcc:16` containers, and an informational GCC 13 entry with `continue-on-error`; the other jobs and repos use GCC 14), `no-qt` (`-DCMAKE_DISABLE_FIND_PACKAGE_Qt6=ON`, ctest incl.
  `no_qt`), `format-check`, `clang-tidy` (the checks that `.clang-tidy` enables and that have zero findings, see "clang-tidy" below, fail the job over
  `src/`, `tools/` and `test/`; the rest of the enabled set is informational, log uploaded), `sanitizers` (ASAN with `detect_leaks=1:detect_container_overflow=0`
  over all tests; TSAN for `tst_sensorworker` and `tst_guarded` with `report_thread_leaks=0`), `clang` (clang++ Release),
  `abi` (see below).
- `openindy-math`: build, `format-check`, clang-tidy (fails on any finding).
- `openindy`: Qt 6.8 via `jurplel/install-qt-action`, builds with all sibling repos, offscreen ctest, `format-check`.
- `openindy-default-plugin`, `openindy-leica-plugin`: sibling core/math, build + ctest (the default plugin's
  `oi_cli_default_plugin` runs core's `oi-cli --selftest` against the plugin), `format-check`.

### clang-tidy

Each repo has a `.clang-tidy` (C++ Core Guidelines set plus a few modernize/misc checks, shared base list) and `HeaderFilterRegex`
anchored on the repo's own directories under the checkout directory name (`.*/<repo>/(dirs)/.*`), so sibling-repo and third-party
(Conan) headers are never linted from another repo's run. `FormatStyle: file` makes `clang-tidy --fix` format its fix-its with
the repo's `.clang-format`; it does not change what is reported. Tests have a `test/.clang-tidy` (`InheritParentConfig: true`)
that turns off checks inherently noisy there (avoid-magic-numbers: assertion literals; owning-memory/macro-usage where noted in
that file). Do not pass `--checks` on the command line in CI or locally if you want the test overrides to apply.
Findings are fixed, not suppressed; a per-site `NOLINT(check)` carries a one-line reason (the `OI_DECLARE_PLUGIN*` macros,
Catch2/Qt-Test expression macros). Gates: core and math fail on the zero-finding checks listed in their `ci.yml` (math: any
finding); plugins and the app fail on the checks they got to zero and keep the rest informational (log artifact).
Run locally: `cmake ... -DCMAKE_EXPORT_COMPILE_COMMANDS=ON`, then `run-clang-tidy -p build 'openindy-core/(src|tools|test)/.*\.cpp$'`.

`format-check` is `git ls-files '*.cpp' '*.h' '*.hpp' '*.cc'` (minus vendored `lib/`) through
`clang-format --dry-run -Werror --style=file`, with clang-format pinned to the PyPI version in `CLANG_TOOLS_VERSION`
(keep your local clang-format equal to it; `uvx clang-format==<ver>` works). Format before committing.

### Plugin ABI

`kPluginAbiVersion` (`openindy-core/include/plugin/plugin_abi.h`) stays the runtime check (plugin entry point
handshake, `oi::PluginLoader`). `openindy-core/scripts/abi-check.sh` makes sure it is bumped when needed: `abidw`
extracts the public ABI of `libOpenIndyCore` (declarations from `include/`, minus `abi/suppressions.abignore`) and
`abidiff` compares it with the committed `abi/baseline.abi` (`abi/baseline.version` records the version it belongs to).

**Rule: an ABI break => bump `kPluginAbiVersion` and refresh the baseline in the same commit.** The script fails on an
incompatible change without a bump, on a bump without a refreshed baseline, and when the baseline is missing.

```sh
# core build with debug info (Release + -g keeps the Conan packages compatible; Debug does not)
cd openindy-core
cmake -S . -B build-abi -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS=-g \
      -DCMAKE_TOOLCHAIN_FILE=build/conan/conan_toolchain.cmake
cmake --build build-abi --target OpenIndyCore
scripts/abi-check.sh --build-dir build-abi            # compare
scripts/abi-check.sh --build-dir build-abi --update   # refresh baseline.abi + baseline.version, commit both
```

Needs libabigail (`abidw`, `abidiff`). `-DOI_ABI_CHECK=ON` additionally registers an `abi_check` ctest entry and the
`abi-check`/`abi-update` targets when libabigail is found; the normal build and ctest never depend on it. CI builds a
pinned libabigail from source and uploads the generated `current.abi` as an artifact. The old byte-digest test
(`tst_pluginabi`, `plugin-abi.digest`) no longer exists: there is no digest to refresh and no "format before digest" step.

## File naming and include guards

All first-party headers, sources, tests, test directories and `.ui` files use snake_case stems split into words by
the class they declare (`feature_wrapper.h`, `oi_job.cpp`, `tst_feature_storage.cpp`, `p_best_fit_plane.cpp`); the
extension stays `.cpp`/`.h`. ctest names and test target names (`tst_featurestorage`, ...) were deliberately not
renamed. Every header has a Google-style guard named after its path from the include root (repo root for
non-include dirs) with a per-repo prefix: `OI_CORE_`, `OI_MATH_`, `OI_APP_`, `OI_DEFAULT_PLUGIN_`,
`OI_LEICA_PLUGIN_` (e.g. `include/plugin/function/function.h` -> `OI_CORE_PLUGIN_FUNCTION_FUNCTION_H_`, closed with
`#endif  // OI_CORE_PLUGIN_FUNCTION_FUNCTION_H_`). The generated plugin metadata files are `meta_info.json` and
`meta_info.h`.

## Run

```sh
./build/bin/openIndy [project-file]     # optional path loads that project on startup
./build/bin/openIndy -s                 # skip splash screen
./build/bin/openIndy -v                 # print version and exit
./build/bin/openIndy -i <plugin-dir>    # import a plugin (copies it in and registers it in the
                                         #  system db), then exits — no UI
```

## Architecture

### Data model (`openindy-core`)

`OiJob` is the in-memory representation of an OpenIndy project: it owns the `FeatureContainer` (all
features), stations, coordinate systems, trafo params, and tracks which of each are "active". Everything
placed in a job derives from `Element` (id) → `Feature` (things with a `FeatureWrapper`
that discriminates the concrete geometry/scalar-entity type — see `include/geometry/*` for the ~15
concrete geometry types: point, plane, line, sphere, cylinder, nurbs, etc., plus scalar entities like
angle/distance/temperature). `FeatureWrapper` is the polymorphic handle used throughout the UI/model
layer to treat heterogeneous features uniformly. Projects persist as protobuf (`openindy-core/proto/openindy/project.proto`, codec in
`include/project_codec.h`). The file stores no file name, path or other local identity (the file may be renamed or
moved): `OiJob::getJobName`/`getJobFile` are runtime only, set by the
Controller to the file's name (without directory) on save-as and load, never written by the codec. The human-editable app configs use the protobuf JSON mapping.

### Plugin system

`Plugin` (`openindy-core/include/plugin/plugin.h`) is the std-only abstract factory every plugin
library implements, exposing factories for the plugin types: `Sensor`, `Function` (further specialized
into construct/fit/generate-feature/object-transformation/system-transformation/geodetic in
`include/plugin/function/`), `BundleAdjustment`, `SimulationModel`, and the two exchange-format
interfaces (`ExchangeSimpleAscii`, `ExchangeDefinedFormat`). A plugin exports it through
`extern "C"` entry points declared with `OI_DECLARE_PLUGIN_WITH_METADATA(ClassName, json)`
(`oi_plugin_abi_version`, `oi_plugin_toolchain`, `oi_plugin_create`, `oi_plugin_destroy`, `oi_plugin_metadata_json`).
`oi::PluginLoader` (`include/plugin/plugin_loader.h`, `dlopen`/`LoadLibrary`) checks the ABI number
(`kPluginAbiVersion`, hand-declared in `include/plugin/plugin_abi.h`, guarded by libabigail) and then the
toolchain string (see "Platform and plugin ABI policy")
before creating the factory and returns the reason on failure. UI tools are not part of core: the app
defines its own Qt `ToolPlugin` interface (`controller/tool.h`), still opened with `QPluginLoader`.

`openindy-default-plugin` is the reference implementation: sensors under `sensors/` (`laserTracker`,
`tachymeter`), functions under `functions/{construct,fit,objectTransformation,systemTransformation}`,
ASCII exchange under `exchange/`, plus a "configured function" (`cf/`) subsystem that lets users compose
functions from JSON config rather than writing new C++.

Plugins are not linked at build time. At runtime `openindy/controller/plugin_host.cpp` loads compute plugins
with core's `PluginLoader` from the `plugins/` directory next to the executable (and `~/.openindy/plugins`)
and keeps each library loaded for the process lifetime; `controller/plugin_manager.cpp` (formerly
`PluginLoader`) creates sensors/functions/... from them. The plugin's `metaInfo.json` (generated from
`meta_info.json.in`, compiled into the library) carries name, version, compiler and OS strings (informational; compatibility is decided by the loader's
toolchain check, and the app's `OiMetaData::getCompiler()`/`getOperatingSys()` use the same spelling, covered by
`tst_plugin_host`). `openindy/controller/plugin_copier.cpp` handles importing a new plugin file, and the system sqlite db (`openindy/db/oisystemdb.sqlite`,
managed by `controller/systemdbmanager.cpp`) records them.

### App layer (`openindy/`)

Roughly MVC: `Controller` (`controller/controller.cpp`) is the central hub the UI talks to — it owns the
active `OiJob`, drives measurement/sensor actions (`ControllerSensorActions`), and delegates to
`ModelManager` (`controller/model_manager.cpp`), which owns/wires up all the Qt item models and proxy
models under `models/` (feature table/tree, observation table, sensor config, trafo params, etc.) that
the dialogs and `MainWindow` (`ui/main_window.cpp`) bind to. `controller/config/*` holds the persisted
app-level config managers (measurement configs, sensor configs, project config, table column configs);
`controller/exchange/*` handles project/observation import-export; `controller/network/*` hosts an
optional websocket server (`OI_WEBSOCKETSERVER_ENABLED`) for streaming job state to external clients.

### Unsaved changes (dirty tracking)

There is no content digest. "Has the project changed since the last save or load?" is a flag:
`ProjectDirtyTracker` (`openindy/controller/project_dirty_tracker.{h,cpp}`, Qt-free) is a `JobObserver` that
`Controller::setJob` attaches to every job and that sets the flag on each event that reports **saved** state
(everything in `project.proto`: features and their names/groups/comments, geometry values, functions and inputs,
trafos, systems, bundle, observations, active feature/station/system, the station's sensor). `Controller` exposes
`isProjectDirty()`, `markProjectClean()` (after a successful save, a load, a new project) and `markProjectDirty()`
(for changes outside the job: measurement config edits, bundle runs). Flag semantics: an edit that is undone still
counts. Events that do not set it: streaming readings and connection state (they never reach a `JobObserver`), the
working group, recalculation bookkeeping, `isSolved`/`isUpdated`. **Rule for new code: a mutation of anything the
codec writes must notify a `JobObserver` event (add one to core if there is none; that is a plugin ABI change) or
call `Controller::markProjectDirty()`; core's `tst_savedchanges` and the app's `tst_projectdirtytracker` list the
paths.** Functions report through `MessageSink::onOwnedObjectChanged` (the job is their sink), bundle adjustments
through their system (`CoordinateSystem::notifyBundleChanged`).

### Sensor I/O threading

`SensorControl` (one per `Station`) is a command queue in front of a `std::jthread` running a
`SensorWorker`, which talks to the plugin's `Sensor` implementation and reports readings/status back
through std callbacks that the host hands to its own thread via an `oi::Executor` (the app's `QtExecutor`
queues them onto the UI thread; core tests use a manual executor). UI code never touches a `Sensor`
instance directly across threads — it goes through `SensorControl`.

### Progress and logging

Long-running operations (`Function::execChecked`, `BundleAdjustment::runBundle`, exchange import/export,
simulation analysis) take an `oi::OperationContext` (`include/progress.h`): progress to a host-implemented
`ProgressSink` plus cooperative cancellation. The app's sink is `QtProgressSink`, which re-emits on the UI
thread. Diagnostics go through `oi::log` (`include/log.h`, spdlog behind a std-only facade; the app routes
warnings/errors to its console and log file); user-visible job messages stay on `MessageSink`.

### Versioning

`OPENINDY_VERSION` is derived from `git describe --always --tags` on `openindy` itself (not on
`openindy-core`/`openindy-math`), forced via `CACHE ... FORCE` in `openindy/CMakeLists.txt` so it wins
over the placeholder default that `openindy-core`'s own `CMakeLists.txt` sets when *it's* built
standalone. `openindy/version.h.in` also captures `openindy-core`/`openindy-math`'s own git-describe
strings separately, purely for display in the About dialog — see the comments in `version.h.in` for why
these are isolated to one small generated header instead of being compile definitions on the whole
`OpenIndyApp` target (avoids a full rebuild on every commit just to bump a version string).

## openindy-py

Python bindings (`import openindy_py as oi`). Build and install from the workspace:

```sh
cd openindy-py
uv sync                       # builds the extension (first configure runs `uvx conan install` into build/conan)
uv run pytest                 # tests/ (layer 1 fits, jobs, packaging, pinned known quirks of the core)
uv pip install ./openindy-py  # from the workspace root: install into any venv
scripts/clean-venv-test.sh    # fresh venv outside the repo, no LD_LIBRARY_PATH, runs the example
```

The CMake build (`CMakeLists.txt`, scikit-build-core) adds `openindy-core` and
`openindy-default-plugin` via FetchContent from the sibling dirs with `OI_BUILD_TESTS=OFF` and
`OI_BUILD_CLI=OFF`, and installs `libOpenIndyCore.so`, `libOpenIndyMath.so`,
`libp_defaultPlugin.so` and `_core*.so` into the package dir with RPATH `$ORIGIN`. Needs system
Armadillo/BLAS/LAPACK and Qt6 Core (the default plugin still uses QObject/QPointer). Conan (Release
only) provides protobuf/spdlog; `openindy-py/conanfile.txt` mirrors core's `[requires]` without
catch2. Native sources are in `src/_core/` (process-wide `Engine` with the one lock; fits use a
throwaway `OiJob` per call and read results from the function's own statistic, because
`Statistic`'s copy drops display residuals); the public API is `src/openindy_py/`. After changing
core/plugin C++ run `uv sync` again (the uv cache-keys cover the sibling sources). Not done: a
standalone manylinux wheel (vendor Armadillo/BLAS), CI, and the vetting harness against Spatial
Analyzer (plan phases P2/P3).
