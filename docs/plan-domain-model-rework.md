# OpenIndy: an open-source contender to Spatial Analyzer

## Goal

Build OpenIndy into a serious open-source alternative to Spatial Analyzer.

**The landing zone for this phase of work:** the new domain model and concepts in place, with all *existing* functionality working on top of them, ready to use. Backwards compatibility is explicitly and happily broken — no data migration, no preserving the persisted format, no shims. Larger blast radius is preferred over hedged changes that carry the old model forward.

**Explicitly after that, not now:** incremental refinement of mathematical and measurement robustness end to end (bundle adjustment, GUM-style uncertainty budgets, traceability chains, tolerance/GD&T evaluation, reporting). These are all real and intended — they are deferred deliberately because they should be built on the corrected model, not bolted to the current one. See "Deferred" at the end; nothing there is cancelled.

Near-term proving ground already delivered: total station support and constellation tracking, driving the 7-point calibration-board workflow (position a board in free space with a total station, feed the pose to [mrcal](https://mrcal.secretsauce.net/) instead of photo-only bundle adjustment).

## The anti-scripting thesis — the reason this project is worth doing

Spatial Analyzer has a solid mathematical core and broad (if poorly executed) instrument support. Its structural weakness is that **its data model can't express enough, so scripting is the escape hatch for everything the model can't say**, and users manually backfill geometry that the software should have derived. "Buy the Ultimate version and script it" is not an acceptable answer.

The counter-position, which every stage below is measured against: **make the model expressive enough that scripting is optional.** Concretely, that means automation-first mechanisms rather than automation-by-script:

- **Requirements are declared, not scripted.** The project's geometry and functions *are* the plan — an unsatisfied requirement is a first-class, queryable fact (Stage 8), not something a script discovers.
- **Derivation is a function, not a manual step.** A plane's normal line and origin point are produced by declared functions on new features, never hand-created after the fact (Stage 7c).
- **Filtering is a declared pipeline, not a script.** Outlier rejection and data selection are part of the input declaration and re-evaluate on change (Stage 7b).
- **Collections are bound once, not element-by-element.** Bulk acquisition mutates a collection; the consuming function is untouched (Stage 7a).
- **The dependency graph already recalculates.** `Feature::recalc()` executes a feature's function list in order and `OiJob::recalcFeatureSet` → `FeatureUpdater::recalcFeatureSet` is signal-driven. Reuse it; don't rebuild it.

## Current state (verified 2026-08-10)

Five repos now, all `master`. C++23. FetchContent with local-dir overrides (`OI_CORE_LOCAL_DIR`, `OI_DEFAULT_PLUGIN_LOCAL_DIR`, `OI_LEICA_PLUGIN_LOCAL_DIR`) — the whole stack builds from `openindy/build`. No CI anywhere.

| Repo | Role | Tests in CMake |
|---|---|---|
| `openindy` | app (GUI, controller, models) | 6 |
| `openindy-core` | shared domain library (SHARED lib, plugin ABI) | 3 |
| `openindy-default-plugin` | bundled sensors + functions | 9 |
| `openindy-leica-plugin` | Leica GeoCom total station driver (extracted Stage 5) | 3 |
| `openindy-math` | math primitives | none |

**Working-tree state that must be dealt with first:** `openindy-core` and `openindy` carry uncommitted, now-superseded code from a discarded Stage 6 attempt (a `MeasurementPlan` class wrapping `Geometry`'s staged config, `Geometry::measurementPlan`, a `ProjectExchanger` sensorType-aware lookup fix, `tst_measurementplan`). → **Stage 6a.**

## Verified facts the plan depends on

Established by reading/running the code, not assumed. An agent should re-verify before relying on any of these.

1. **The producer/transform execution model already exists.** `Feature::recalc()` (`feature.cpp:478-498`) executes `functionList` in order, each `function->exec(this->selfFeature)` mutating the same feature, breaking on first failure. `FeatureUpdater` already treats `getFunctions().at(0)` as the producer (`featureupdater.cpp:142-145`). Nothing declares or enforces this; it is convention only. → **Stage 7d formalizes it.**
2. **Nominal/actual pairing already exists — and is being deleted.** `Geometry` has `isNominal`, `nominals`, `getActual()`/`setActual()` (`geometry.h:33,54-55,162-164`), and `Controller::_startMeasurement` swaps a nominal for its actual before measuring (`controller.cpp:1225-1230`). *Was* the machinery for "planned but not yet measured"; **superseded by D17** — one object wearing two epistemic hats, which is why the UI lets you tick both. → **Stage 7g** removes it.
3. **Multi-output generation is unwired.** `GenerateFeatureFunction::addFeature`/`addFeatures` signals exist (`generatefeaturefunction.h:22-23`); the only subclass is `p_pointcloudsegmentation`, which is not in the build; nothing in `openindy/` connects to them. **Deliberately not being wired** — see D8.
4. **Bundle adjustment does not exist.** `BundleAdjustment` is a 275-line interface; `runBundle()` emits `"Bundle ... not implemented"` and returns `false` (`bundleadjustment.cpp:31-34`); `createBundleAdjustment()` returns `NULL` (`p_factory.cpp:259-262`). No subclass in any repo. Deferred, not cancelled.
5. **No GD&T/tolerance concepts exist in core**, and no CAD import. Deferred.
6. **`Reading` already records how it was measured** — `mConfig` (`reading.h:213,307`), and `Observation` references its `Station`. "How was this measured" is answerable by walking down from a result; no geometry needs to store it.
7. **`ePointCloudElement` exists in the `ElementTypes` enum but no function declares it as a `NeededElement`.** Point clouds are currently unusable as fit input.
8. **Function input roles are positional, via a half-built enum.** `InputElementKey` is `{eNotSet=-1, eDefault=0, /* ... */, eDummyPoint=10000}` (`function.h:50-55`) and `NeededElement::key` is documented *"if key not set InputElementKey is NeededElement index"* (`function.h:66`). `BestFitPlane` already declares two `eObservationElement` inputs distinguished only by that key (`p_bestfitplane.cpp`), so roles are real but unnamed. → **Stage 7f replaces this with a stable `roleName`.**

## Cross-cutting principle: modern C++23 over Qt gap-fills

Applies to every stage. Where Qt is only filling a pre-C++11/17 gap, prefer the standard library. Goal is Rust-like robustness — errors and absent values in the type system, not in runtime convention.

- **`std::expected<T, E>` replaces bool-return + out-param error handling.** The sensor layer is full of hand-rolled Results (`bool connectSensor()`, `emit commandFinished(success, …)`).
- **`std::variant` + `std::visit` over enum-tag + `QVariant`** for sum types (D3) — missed cases become compile errors.
- **`std::optional`** for absent values instead of sentinel/invalid-flag conventions.
- **Keep Qt where it is genuinely Qt's domain**: signals/slots, `Q_DECLARE_METATYPE`/queued connections (hard requirement for cross-thread calls), widgets, `QString` at UI/serialization boundaries.
- **Templates are not available for `QObject` subclasses** — moc doesn't support them. Runtime type constraints are the pragmatic substitute.
- Judgement, not a blanket rewrite: weigh churn per case.

## Threading hazards (applies to every stage)

`SensorControl` → `SensorWorker` crosses a thread via `QMetaObject::invokeMethod` with `Q_ARG`.

- **Every new type crossing that boundary needs `Q_DECLARE_METATYPE` + `qRegisterMetaType`.** Missing one fails at *runtime*, silently, with no compile error. This has already bitten once (`qRegisterMetaType<oi::SensorType>("SensorAction")` registered the wrong type under the right name).
- `std::variant` as a metatype must be default-constructible → its **first alternative** must be. Order deliberately.
- **Streaming is the real hazard**: never emit one queued signal per sample. `measurementFinished(geomId, readings)` already passes a *list* — keep that shape, batch per interval.

---

# Completed stages

### Stage 0 — Test harness · **DONE**
Ported the app's four suites from qmake to CMake (`featuresorter`, `measurebehavior`, `watchwindow`, `dialogs`), GUI suites under `QT_QPA_PLATFORM=offscreen`. Also fixed a real bug found on the way: `AvailableFunctionsListProxyModel::filterAcceptsRow` consulted the global `PluginRepository` instead of the row's own function; extracted `PluginRepository::isCreateFunctionFor()` as a shared predicate.

### Stage 1 — `SensorType` overhaul · **DONE**
Closed `SensorTypes` enum (68 refs, 23 files) replaced with an open `SensorType` resolved from plugin interface IDs. Added pure-virtual `getSupportedMeasurementConfigs()`/`getSupportedAcquisitionModes()`; `AcquisitionMode{eDiscrete,eContinuous}` (D2). Deleted `SensorFacade` (~370 lines: wired signals backwards, leaked `inner`, wrapped only one driver). Fixed a metatype registration bug and two drivers under-declaring their own capabilities.

### Stage 2 — Simulation layer + driver-owned accuracy · **DONE**
Extracted `PseudoTracker`'s Ben Hughes polar error model into a reusable `TrackerErrorModel` (`<random>`, not `rand()`); added the scene → station pose → inverse geometry → error model → reading-with-sigma layer that makes everything downstream testable against known truth. Fixed the `zenith = myAzimuth + dze` bug and the stub reading types. **Biggest find: `Observation::sigmaXyz` was never populated anywhere** — `FeatureUpdater::transformAllObsToDestSystem` copied `xyz`/`ijk` from their originals but not sigma.

### Stage 3 — Driver-owned configuration and connection · **DONE**
Drivers publish their own `MeasurementConfig` lists, tagged by `SensorType` at scan time. `ConnectionTypes` enum → open `ConnectionKind`. **Two real bugs caught by new tests**: `MeasurementConfigKey` identity was (name, configType) alone and silently collided across drivers; `MeasurementConfig`'s hand-written copy constructor and `operator=` didn't copy the new field at all.

### Stage 4 — Constellations and pose solving · **core criterion DONE**
`CalibratedConstellation`/`LocalPoint` in core (D6). Pose solving reuses `Helmert6Param` with zero new solver code, and `TrafoParam::getStatistic().getStdev()` — already computed, never previously exercised — supplies propagated uncertainty. Regression test observes a synthetic 7-point board via a simulated total station and recovers both measured points *and* a never-observed derived point, with tolerance derived from the solver's own reported stdev rather than a magic constant.

### Stage 5 — Leica driver extraction · **structural work DONE**
`LeicaTachymeter` moved to its own repo (`openindy-leica-plugin`), code-unchanged except `metaData.pluginName`. Proved the plugin architecture: a second `.so` needed **zero** core changes to be discovered, type-resolved, and deployed. **Owed, hardware-dependent, cannot be done by an agent:** end-to-end verification against a real TS15/TS1103.

### Infrastructure — plugin ABI versioning · **DONE**

Not a numbered stage; an infrastructure defect found while finishing 7c-i, fixed before continuing because it silently invalidates plugin binaries and the whole architecture is third-party plugins.

`PLUGIN_INTERFACE_VERSION` was `git describe`'s **commit count** (`1.20.0-27-g…` → `120027`), so every core commit changed every plugin IID whether or not the contract changed. Two failure modes, both live:

- Plugins built one commit earlier stopped loading. The version arrived as a **CMake compile definition evaluated at configure time**, so a plugin build tree configured before the change kept the old value and produced a stale binary that nothing rebuilt.
- A mismatch was **indistinguishable from "not an OpenIndy plugin"** — `qobject_cast<Plugin*>` returned null and the driver vanished with no message. Diagnosed only after a whole Leica driver disappeared and surfaced as an unrelated measurement-config count mismatch in a test.

Replaced by `OI_PLUGIN_ABI_VERSION`, a hand-declared integer in `openindy-core/include/plugin/pluginabi.h` (commits `229e3c8c` core, `27607c04` app, `99cee47` default plugin, `47b3fe0` leica plugin). Now at **1**; the IID scheme is `de.openIndy.plugin.v1`, invalidating every previously built plugin once, on purpose.

- **Declared, not derived** — no automatic source can tell "fixed a comment" from "added a pure virtual to `Sensor`", and a commit count can run *backwards* on a revert.
- **A header, not a compile definition** — this is what removes the staleness structurally: a header is a tracked source dependency, so changing it forces every plugin that includes it to rebuild. Verified: bumping to 2 and building only the app target rebuilt both plugins automatically; the old bug could not be reproduced.
- **`abiVersion` in `metaInfo.json`** — the app checks compatibility by reading metadata *before* instantiating the library, and reports the file, both versions, and the fix. Checked before the user/system tier merge, so a stale plugin in `~/.openindy/plugins` cannot shadow a working one. Verified end to end against a deliberately stale binary.
- **`tst_pluginabi` guards the number** against the failure that actually matters — a *missed* bump, which lets a stale plugin load against a moved vtable. Fingerprints every header in `include/plugin/`, excluding comments and blank lines, and fails when the surface moves without the version. Verified both ways: comment-only edit passes, added virtual fails.

**Known limit, deliberately accepted:** the fingerprint does not cover layout changes to domain types that cross the boundary but live outside `include/plugin/` (`Geometry`, `Position`, `Reading`, `Observation`). Those are equally ABI-breaking and still need judgement — the bump criteria in `pluginabi.h` say so explicitly. **Stage 7 and Stage 10 both change ABI-relevant types and must bump it.**

**Considered and rejected: extracting a separate `openindy-plugin-api` repo.** It would not shrink the ABI surface — the plugin contract genuinely *is* most of core's domain model (`Sensor::measure` returns `QList<QPointer<Reading>>`, functions take `Geometry&`, `InputElement` carries `Position`/`Direction`). Separating it relabels the surface rather than reducing it, and Stage 7 is actively churning those exact types. Revisit only if the domain model stabilises and a genuinely thin interface emerges.

---

# Remaining stages

Each stage is independently buildable, verifiable, and committable, with its model assignment fixed up front.

---

### Stage 6a — Clear the working tree · **Haiku**

Discard the superseded Stage 6 attempt so later stages start clean.

- `openindy-core`: remove `include/measurementplan.h`, `src/measurementplan.cpp`, `test/measurementplan/`, their `CMakeLists.txt` entries, and revert `Geometry`'s `measurementPlan` member back to a plain field (this whole area is replaced properly in Stages 7–8, so revert rather than half-keep).
- `openindy`: revert `controller/controller.h`/`.cpp` metatype registrations for the discarded type.
- **Keep, do not revert** — these are independently correct and were found during the discarded attempt:
  - `ProjectExchanger`'s `measurementConfigs` map keyed by `MeasurementConfigKey` instead of name (name alone collides across drivers, same class of bug as Stage 3's).
  - `Geometry::toOpenIndyXML` writing the config's `sensorType` attribute, and `ProjectExchanger` reading it back through the full key.

**Acceptance:** all four repos build; `ctest` green in `openindy-core`, `openindy-default-plugin`, `openindy`, `openindy-leica-plugin`; `git status` clean after commit; no reference to `MeasurementPlan` remains.

---

### Stage 7 — The domain model rework

The core of this phase.

**Execution order** (letters are labels, not sequence — this list is the sequence):
`7a` ✓ done, partly superseded → **`7g` nominal/actual rework** → **`7f` intent-tagged acquisition** → `7c-ii` fit consolidation → `7h` function-owned sub-geometry → `7b` filter pipelines → `7d` producer/transform roles → `7e` Group-as-organization.

7g goes first for two reasons: it *deletes a branch from the acquisition path* that 7f would otherwise carry (`Controller::_startMeasurement`, `controller.cpp:1225-1230`, detects a nominal active feature and calls `createActualFromNominal()` before measuring), and it is the **first use of D19 ownership**, which 7f and 7h both then build on.

**The load-bearing idea across all of Stage 7 is D19: ownership determines editability and origin.** A station owns the points its observations produced; a function owns the sub-geometry it generates; the user owns what they create. Everything else — how a fit tells an observation from a derived point, how nominal differs from actual, how generated reference geometry stays in sync — falls out of that one rule instead of needing flags or type sniffing.

7f moved to the front because 7c-ii is blocked on it and because the tagging model it introduces reshapes 7a, 7b and 7e. **7b is closer to the front than its letter suggests**: by D16, rejecting a bad point is an `Exclude` pipeline stage, so nothing can be rejected from a fit until 7b lands. Fitting works without it; field-realistic fitting does not.

#### Stage 7a — `PointCollection` hierarchy and collection binding · **Sonnet** · SHIPPED, then PARTLY SUPERSEDED by 7f

> **Read this first.** 7a shipped and its two bug-fixes were real and stand. But its central mechanism — a fit binding to a stored `PointCollection` — is **superseded by intent tagging (7f)**. Tags solve the same binding-friction problem more directly: the fit stores one declaration and resolves N points, so its input list cannot grow *by construction*. `PointCollection` is retired, `PointCloud` survives as a container outside the function system, and `Group` keeps only its organizational role (D14). The section below is kept for the history and for the two bugs it documents; treat its design as background, not as the target.

Fixes the binding friction that makes bulk acquisition unusable, and gives point clouds a way into fit functions for the first time.

**The hierarchy** (corrected — an earlier draft wrongly claimed cloud and group share nothing):

```
PointCollection        — "a collection of points"; the type a fit function asks for
├── PointCloud         — owns bulk undifferentiated scanner data (Point_PC); exists today
└── PointGroup         — references real, individually-measured Point features
```

- A fit function declares `NeededElement{ typeOfElement = ePointCollectionElement, infinite = false }` — it needs **exactly one** collection. Its `InputElement` list never grows past one entry regardless of how many points are acquired.
- The UI offers any `PointCollection`; the user picks a cloud or a group.
- `PointGroup` holds **references** (`QPointer<FeatureWrapper>`/ids) to `Point` features — never owned copies. `Point` remains the sole first-class measured citizen (D9).
- **Many-to-many membership**: a `Point` may belong to any number of groups simultaneously. This is the direct fix for copy-paste-heavy reuse in SA.
- **Implementation note, `QObject`/moc**: `PointCollection` cannot be a template. Make it an abstract interface providing point access (positions + per-point pedigree where available), implemented by both subclasses. Whether the generic `Group` of Stage 7e *is* `PointGroup` when type-constrained to points, or whether `PointGroup` is a distinct class, is a **design call the implementing agent must make explicitly and record** — both are defensible; do not leave it ambiguous.

**Also decide and record**: deletion semantics (a `Point` deleted while in three groups — do groups hold null `QPointer`s, get compacted, or block deletion?) and whether mutating a group's membership re-triggers dependent fits (it should — the graph is already signal-driven; wire it).

**Split during implementation** — the data model landed cleanly, but registering `Group`/`PointGroup` as `FeatureTypes` cascades into `FeatureWrapper`, `FeatureContainer`, `OiJob`, `ProjectExchanger` and ~30 exhaustive switches (the enum is explicitly order-sensitive). Bundling that with the type definitions would have made one unreviewable commit.

- **7a-i — data model · DONE** (`openindy-core` 38fee516). `PointCollection` pure interface; `Group` (one generic class, optional type constraint, references never copies, many-to-many membership); `PointGroup` (thin `Group` subclass + interface); `PointCloud` now satisfies the interface. 12-case regression suite (`tst_pointcollection`).
  - **Two real bugs found and fixed**: `Point_PC` was entirely non-functional — no `public:` section at all, so its constructor was inaccessible and `xyz` unreadable, and it was held in `QPointer` despite not being a `QObject` (compiled only because the template was never instantiated against it). Nothing could put a point into a `PointCloud` or read one out. Also, `Group`'s duplicate/self-membership detection compared feature *ids*, but `Element` defaults `id` to `-1` until `OiJob` assigns one, so every unmanaged feature would have collapsed into a single member — identity is now the underlying `Feature` pointer.
- **7a-ii — job integration and binding · DONE** (`openindy-core` d8046e6f, `openindy-default-plugin` 1e00445). `Group`/`PointGroup` appended to `FeatureTypes`, wired through `FeatureWrapper` and `OiJob::createFeatureWrapper`; `ePointCollectionElement` added; `InputElement` carries both handles plus `asPointCollection()`; `OiJob::setGroupMembers` relays `groupMembersChanged` into the recalculation graph, walking only `getUsedFor()` rather than recalculating everything. `PlaneFromPointCollection` added to the default plugin as the first collection-binding function.
  - **The blast radius was much smaller than estimated** — 8 case sites across 6 files, not ~30. The switches carry `default:` arms, so new enum values fall through silently rather than warning; they had to be found by inspection, not by the compiler.
  - **Real pre-existing bug found by the new test, not by inspection**: `bestFitPlane` (`fitfunction.h`) checked only `points.size() == 0` and then unconditionally returned `true`. For collinear or coincident points `ata` is rank-deficient, two eigenvalues are ~0, and "the eigenvector of the smallest eigenvalue" picks arbitrarily between them — the normal returned was noise, reported as success. `BestFitPlane` and `PlaneFromPoints` were affected too. Now rejects when the second-largest eigenvalue is negligible relative to the largest (relative threshold, deliberately conservative at 1e-12 so it cannot fail fits that previously worked).
  - **This falsified a design assumption recorded in this plan** — that degeneracy validation "already lives inside each function's `exec()`". It did not. A declared minimum count remains a hint, and the *real* gate now genuinely exists because it was added.
  - `PlaneFromPointCollection` was added **alongside** `PlaneFromPoints` rather than replacing it: swapping now would break plane fitting in the UI, since there is no way to create a group there until Stage 9. Stage 7c consolidates once the UI can keep up.
  - **Still outstanding from 7a-ii's original scope**: `ProjectExchanger` persistence for groups (`Group::pendingMemberIds` exists and is populated by `fromOpenIndyXML`, but nothing resolves it into real references yet — a group will not survive save/load until it does). Fold into Stage 11 or do as a small dedicated task.

#### Stage 7g — Nominals as produced features · **Sonnet** · ← runs first

**The problem, in the user's words:** *"I can actually check both actuals and nominal currently, in the UI which makes zero sense."* `Geometry` carries `isNominal`, a `nominals` list and an `actual` back-reference (verified fact 2), and `Controller::_startMeasurement` swaps a nominal active feature for its actual before measuring (`controller.cpp:1225-1230`). One object wearing two epistemic hats.

**What actually separates them:**

| | produced by | carries |
|---|---|---|
| **nominal** | a *specification* — a drawing, a model, a parameter | **tolerance** — what is allowed |
| **actual** | *observation* — a fit over measurements | **uncertainty** — how well we know it |

**They do not need separate types per geometry.** Both are a `Plane`; what differs is the **producer function** — which Stage 7d formalises anyway:

```
NominalPlane(scalar parameters…)  → Plane  + tolerance
BestFitPlane(points…)             → Plane  + uncertainty
```

**Scope.**
- Delete `Geometry::isNominal`, `nominals`, `actual`/`setActual`, and `Controller::createActualFromNominal`. Clean break — no migration.
- **Two geometries, not one** (D17). Nominal: user-created, editable, producer `NominalPlane(params)`, carries tolerance. Actual: **owned by its fit** (D19), read-only, carries uncertainty, holds `std::optional<QPointer<Geometry>> realises` pointing at its nominal. **Many actuals per nominal** — re-inspection over time is the case that decided this.
- Add nominal producer functions taking **scalar parameters, hand-entered literals** (`NominalPoint` first; the rest are pattern-replication for **Haiku**). They declare no element inputs.
- "Measure this nominal" survives as an operation: select a nominal, measure, and its actual is created or updated — roughly what `createActualFromNominal` does today, but through an explicit reference rather than a flag.
- **Ownership is enforced, not conventional.** A read-only feature rejects user edits at the API, with a message naming its owner. This is the first use of D19 and sets the pattern for function-owned sub-geometry and station-owned observations.
- `std::optional` with C++23 monadic ops (`and_then`/`transform`/`or_else`) rather than sentinel checks. **`Geometry<G>` is impossible** — moc cannot process templated `QObject` subclasses — so the geometry type is checked at runtime; this is the one place the Rust shape does not carry over.
- `ProjectExchanger`, the feature tree, `createfeaturedialog` and `nominalpropertiesdialog` all assume the pairing — expect the blast radius there.

**Deliberately NOT in this stage** — parametric nominals. Literal scalars now; a job-level named parameter table comes later (see Deferred), and is purely additive because a parameter just becomes another thing a scalar input can reference.

**Acceptance:** a nominal plane is created by choosing `NominalPlane` and typing values, with no `isNominal` flag anywhere; measuring no longer has a nominal branch; a nominal and an actual of the same geometry type coexist as two ordinary features; project round-trips; `ctest` green in all four repos.

#### Stage 7f — Intent-tagged acquisition · **Sonnet** · after 7g

**Context.** 7c-ii was blocked because `OiJob::addMeasurementResults` (`oijob.cpp:1631-1637`) binds an observation to a geometry's first function *only if that function declares `eObservationElement`*. Designing the way out surfaced that the original "measure into a bound `PointGroup`" idea had three faults: it put ergonomics inside the domain (core inventing groups as a side effect), it assumed a total station (D2: a V-STARS snapshot yields thousands of points, a scan yields a cloud), and it made membership a stored list that goes stale.

**The model.** Intent is captured at acquisition, when the operator knows it and it is free. This is survey feature-coding — shoot a point with a code, let the office software group by it — which has survived jobsites for decades.

- A `Point` carries a **list** of `MeasurementIntent{ featureId, roleName }` tags. A list, because D10's many-to-many is real: a point shot for `wall-01` may also serve as a registration target.
- **`NeededElement` gains a stable `roleName`.** This replaces a half-built mechanism rather than adding one: `InputElementKey` today is `{eNotSet=-1, eDefault=0, /* ... */, eDummyPoint=10000}` with the comment *"if key not set InputElementKey is NeededElement index"* — roles are **positional by default**, and `BestFitPlane` already has two same-typed inputs distinguished only by that key. Anchoring tags to `featureId + roleName` means swapping a fit function preserves the tags as long as the new function declares the same role — which matters because the tracker-dies-fall-back-to-total-station scenario is exactly "the function changed, the measurements didn't".
- **A fit's point input resolves to a query, not a stored list**: points tagged `(this feature, this role)`. Measuring auto-populates it; re-measuring re-evaluates it. This is what removes the manual backfilling.
- **Hand-allocation and field acquisition are the same operation.** Dissecting a V-STARS batch in the office is stamping the same tag retroactively. No separate mechanism, no special case — the only difference is *when*.

**The one bridge.** Exactly one function type crosses from observations into geometry — the one producing a *measured* `Point`. Everything above that line consumes `Point`s.

```
Reading(s) → Observation(s) → [BestFitPoint]      → Point    ← measured
Point × N                   → [AverageFromPoints] → Point    ← derived
Point × N                   → [BestFitPlane]      → Plane
```

**A `Point` with observations is a measurement; a `Point` without is derived.** The distinction is visible in the data — no flag, no subtype.

**Target correspondence — resolves the design log's oldest open item.** One aiming event = one **new** `Point`, always. There is no re-measure mode. Combining repeated shots of the same physical target is an ordinary derived `Point` with a declared function over the raw ones:

> Tracker dies mid-job, total station comes out. Four nests × three sets = **12 raw `Point`s**, one observation each. Four **derived `Point`s**, each averaging its three. The plane still fits from four points. Project prep is unchanged; only the function producing those four differs, and there are 12 extra objects lying around.

This is D8 applied consistently — combining is a *declared function*, not a hidden acquisition mode — and it is strictly better than the "explicitly re-target the existing `Point`" design this plan carried earlier: the raw shots stay individually visible, inspectable and rejectable instead of being buried inside one point, and acquisition **loses** a branch rather than gaining one. Spatial clustering was rejected outright: it guesses worst exactly where points are densest, which is the 100mm-grid-on-a-factory-wall case.

Requires one new function, `AverageFromPoints` (`ePointElement`, `infinite=true`, applicable for `ePointFeature`) — the same shape as the 7c-i derived functions, so **Haiku can build it** once the tag machinery exists.

**Why fits consume `Point`s and not `Observation`s.** Considered and rejected. A fit over observations cannot distinguish one nail measured five times from five distinct locations, and weights that spot 5×. The sharp case is cross-station: the same nail from two stations is two observations and **one** point, so an observation-consuming fit double-weights every tie point. Not a crash — a quietly wrong answer with a plausible residual.

**Scope.**
- `MeasurementIntent` on `Point`; `roleName` on `NeededElement`; retire `InputElementKey`'s positional fallback (`eDummyPoint` becomes role name `"dummyPoint"`).
- `addMeasurementResults` always creates a new `Point`, stamps its tags, attaches its observations. **The `eObservationElement` special case is deleted, not worked around.**
- **Driver declares its result shape** (one point / many points / a cloud) as a capability, per D2 and the capability-vs-config-field principle. Only the one-point path needs to work now; the others need to be *expressible*.
- **Core never invents a group.** The controller stamps tags from the selected plan item.
- **Bumps `OI_PLUGIN_ABI_VERSION`** — `NeededElement` is passed across the plugin boundary. `tst_pluginabi` will fail until it does; that is the guard working.
- **`PointCollection` is retired.** Tagging solves the binding-friction problem 7a existed to solve, and solves it better: the fit stores *one declaration* and resolves N points, so its input list cannot grow by construction rather than by wrapping. The interface is left with one implementor and no polymorphic use.
  - **`PointCloud` survives as a container**, outside the function-input system. It holds `Point_PC` — bare xyz structs, millions of them — not `Point` features; you can tag fifty V-STARS targets but not 40,000 scan points, and segmenting them is deferred research. Verified fact 7 confirms no function consumes it today anyway.
  - `PlaneFromPointCollection` re-declares its input as tag-resolved points; the fit body is unchanged.
  - `tst_pointcollection` / `tst_collectionbinding` get **reviewed, not deleted** — `functionInputListNeverGrowsWithPointCount` and the many-to-many cases still assert things that must remain true, and are now the regression guard for the tag path.
- **Task #52 (`Group::pendingMemberIds` on load) folds into this** — deferred reference resolution is the same machinery tags need for points measured before their target feature exists.

**RESOLVED — `Observation` and `Point`** (D20, D19, D15a).

`Observation` **is** a `Point`, owned by its `Station`, read-only. A fit selects `Point`s and intermingles observations with derived points freely — the selection logic never asks which kind it holds, which was the stated requirement. A `Point` is *one physical target as captured in one acquisition*, holding however many readings the instrument took (D15a), so a 10-reading config still yields one Point and D15 stands. Per-reading rejection and re-averaging (7b) operate on the readings inside it.

*Background, kept because it explains why this shape:*

`Observation` already is most of what the tag design invents:

```cpp
class Observation : public Element {
    QPointer<Station>              station;
    QList<QPointer<Geometry>>      targetGeometriesList;  // ← many-to-many "what this is for"
    QPointer<Reading>              reading;
    OiVec xyz, sigmaXyz, ijk;                             // ← a position, with uncertainty
    bool isDummyPoint;                                    // ← a role, as a bool
};
```

`targetGeometriesList` **is** the tag list, missing only a role name; `isDummyPoint` is a role badly expressed. As written above, 7f would produce three objects per shot — an `Observation`, a wrapping `Point`, and a `BestFitPoint` that does nothing meaningful for one observation.

**Requirement established during design:** a fit must accept a **mixture of measured and derived points** without the selection logic caring which is which.

The inheritance is legal single inheritance — `Observation : Element` and `Point : Geometry : Feature : Element`. The objection that it would inherit `nominals`/`actual` is **void once 7g lands**, which is why 7g runs first. Remaining inherited oddities (`myObservations`, `functionList`) are handled by D19: a station-owned point has no producer function and no observations of its own, and being read-only makes that unreachable rather than merely unused.

**Still open, deliberately parked until implementation:** whether `targetGeometriesList` is **extended** into the tag list (gaining a role name, `isDummyPoint` becoming role `"dummyPoint"`) or **replaced** by a fresh `MeasurementIntent` list. It already duplicates itself as both a `QList` and a `QMap`, which the implementing agent must resolve either way. Decide with the code in front of you; do not carry both forward.

**Acceptance:** measuring for a plane creates tagged `Point`s and the plane's fit finds them with no binding step; a fit accepts a mixture of measured and derived points transparently; three raw points averaged by `AverageFromPoints` produce one derived point and the plane fits from derived points identically; renaming the feature does not orphan tags; a point carrying two tags is found by both consumers; a measured point has observations and a derived one has none; core rejects a measurement whose target cannot be resolved, with a message; `ctest` green in all four repos.

#### Stage 7b — Filter pipelines on needed elements · **Sonnet** · after 7f · **DONE**

The anti-scripting mechanism for data selection. Now sits *downstream of tags*: tags decide what a needed element's candidate set is, the pipeline refines it. A `NeededElement` carries an ordered, declarative pipeline — Java-Stream-shaped: each stage takes the incoming data and yields incoming, filtered, or computed data.

- Pipelines are **declared and re-evaluated**, never a one-off destructive edit — re-running after new points arrive is the whole point.
- Filter stages are plugin-provided and parameterized, same shape as functions (an outlier-rejection stage is the driving case: sigma threshold, residual-based rejection).
- **`ExplicitList` and `Exclude(reason)` are ordinary stages.** This is what makes hand-picking and auto-tagging one mechanism rather than two, and it makes an exclusion a recorded fact instead of a silent deletion.
- **Traceability requirement**: the pipeline must report what it excluded and why. A silently dropped point is unacceptable in an inspection context.
- The **same pipeline applies at observation level** — `BestFitPoint` already declares `infinite=true` observations, so "average these, reject that bad shot, use only station 2" needs no new type. Design the shape here; ship stages as needed.
- Build the mechanism plus **one** real stage (residual/sigma outlier rejection). Do not speculatively build a filter library.

**Acceptance:** a plane fit over a tagged point set containing deliberate outliers converges correctly with the filter declared and incorrectly without it; the excluded points are individually reportable with a reason; adding a new outlier re-triggers the filter and the fit.

**What shipped (`openindy-core`, `openindy-default-plugin`).**

- **`NeededElement` gained `pipeline` (`QList<PipelineStage>`), literally as the plan describes it** — declared once by whatever attaches the pipeline (an API call today, a UI later) and persisted with the function, not a separate parallel structure keyed by position. `PipelineStage` (new header, `include/plugin/function/pipelinestage.h`) is a plain C++23 value type with a `PipelineStageType` enum (`eExplicitListStage`, `eExcludeStage`, `eResidualOutlierRejectionStage`) and typed fields per stage kind — the same "typed fields on one class" shape `Function` itself uses for `integerParameters`/`doubleParameters`/`stringParameters`, deliberately not `std::variant` (Stage 10 replaces that pattern project-wide; this predates it and follows the existing convention rather than scope-creeping into building Stage 10 early).
- **The choke point is `Function::exec(const QPointer<FeatureWrapper>&)`**, the one method every subclass funnels through (`Feature::recalc()` calls exactly this overload). The pre-existing switch-on-feature-type dispatch moved unchanged into a new private `execByFeatureType()`; `exec()` itself now wraps it: apply deterministic stages (`applyDeterministicPipelineStages()`) → `execByFeatureType()` → if a residual stage fired, apply it against that pass's `Statistic` (`applyResidualPipelineStages()`) → if it changed anything, `execByFeatureType()` again. **This is why zero fit functions needed code changes**: `BestFitPlane`, `BestFitLine`, `BestFitSphere`, `BestFitCircleInPlane`, `BestFitCylinder` and `BestFitPoint` already read `InputElement::shouldBeUsed` and call `setIsUsed`/`addDisplayResidual` (Stage 7c-ii groundwork) — the pipeline mechanism only ever mutates `shouldBeUsed` before a pass they already respect.
- **Exactly two passes, not iterative reweighting to convergence.** `eResidualOutlierRejectionStage` needs a completed fit's own residuals as its input, so it cannot be evaluated before a fit exists; the deliberate simplification is that deterministic stages always resolve before any residual stage regardless of declared order, and a residual stage fires at most once per `exec()` call (a point just inside threshold pre-refit that becomes an outlier post-refit is not caught until the *next* recalc, which reruns the whole two-pass loop fresh — see domain-model.md S9 for why this is accepted, not an oversight).
- **The one real stage reads whatever residual the fit already reported**, via `Function::addDisplayResidual`'s stored `Statistic` (`getObservationDisplayAttributesName(eObservationDisplayV)`, falling back to `eObservationDisplayVR` for functions like `BestFitSphere` that report the scalar-only variant) — not geometry-specific math duplicated in the stage. Not every fit function reports a residual under either key (`BestFitCircle`'s plain variant currently mislabels all three of vx/vy/vz under one `"vx"` key, a pre-existing bug left untouched, out of scope for this stage); the stage degrades to a no-op there rather than guessing, which is a known, documented limitation, not silently wrong behaviour.
- **`OiJob::setPipeline`/`excludeElement`** are the real, callable entry points a UI would call — same shape as the pre-existing (also UI-less) `OiJob::setShouldBeUsed`. No dialog was built, the same latitude Stage 7h used when the mechanism itself is genuinely callable end to end.
- **Persistence needed zero `openindy` (app) changes.** `PipelineStage`/`PipelineExclusion` hold only ids and scalars, no object references, so `Function::toOpenIndyXML`/`fromOpenIndyXML` (both already called by `Feature::toOpenIndyXML` and `ProjectExchanger::restoreFunctionDependencies` respectively) round-trip the declared pipeline entirely on their own — unlike `InputElement`'s object pointers, which is why *that* needs `ProjectExchanger`'s separate ID-resolution pass and pipelines don't. `pipelineExclusions` (the traceability report) is deliberately **not** persisted — it is fully re-derived from the declared pipeline plus current data on every `exec()`, exactly like `isUsed`.
- **Bumped `OI_PLUGIN_ABI_VERSION` 4 → 5.** `Function` gained two data members (`neededElements`' new `pipeline` field, and `pipelineExclusions`) that cross the plugin boundary — every fit function in every plugin subclasses `Function` directly. Digest refreshed.
- **New tests**: `openindy-core/test/pipeline/tst_pipeline.cpp` covers the generic mechanism (opt-in behaviour when no pipeline is declared, ExplicitList/Exclude semantics and reasons, residual-based exclusion and convergence, re-triggering on a newly-added outlier with no redeclaration, XML round-trip) against a fake `FitFunction` — fast, no cross-repo dependency. `openindy-default-plugin/test/pipelinefilter/tst_pipelinefilter.cpp` is the literal acceptance criterion, driven against the real `BestFitPlane` through `OiJob::addMeasurementResults` exactly like `tst_measuredfit.cpp`, plus a `BestFitPoint` case proving (not asserting by symmetry) that the identical mechanism works at observation level, the one function touching raw `Observation`s per D13.

**Deviated from scope, noted explicitly:**

- **Filter stages are not plugin-discoverable**, despite the plan's "plugin-provided and parameterized, same shape as functions" wording. `PipelineStage` is a plain value type dispatched by an enum inside core, not a `QObject` interface a plugin implements and `PluginRepository` discovers the way `Function` subclasses are. Reason: the one stage this pass ships has to read whatever residual the *owning* fit function already reported generically, from `Function::exec()` itself — that only works because it is evaluated at the one choke point every fit funnels through, not through a per-stage virtual a plugin would implement in isolation with no access to the fit's own `Statistic`. Building a second plugin-discovery ABI (new interface header, new IID, factory, `PluginRepository` wiring) for exactly one built-in stage would be the speculative filter library the plan explicitly says not to build. `PipelineStageType` can grow a genuine plugin-discoverable shape later without disturbing what's persisted (the XML shape and enum values are independent of how a stage's behaviour is dispatched) if a second stage genuinely needs custom, fit-specific math a plugin author supplies.
- **Test data surfaced two real facts about `BestFitPlane` worth recording, not fixed here (out of scope):** its normal-sign rectification step degenerates for three collinear "used" points (uses only `usedPositions[0..2]`'s cross product for sign, not the PCA fit itself, so magnitude is unaffected but this is still a landmine for any caller that orders points by acquisition order into a grid); and a naive population-stdev sigma filter is only unmasked by its own outliers once the sample is large enough (`N > 9k+3` for `k` outliers of equal residual magnitude among `N` points) — both noted in `tst_pipelinefilter.cpp` itself and in domain-model.md S9.
- **No UI was built** (no checkbox/dialog to declare a pipeline or pick an explicit-exclude reason). `OiJob::setPipeline`/`excludeElement` are the full, real, callable triggers a UI would call, mirroring Stage 7h's precedent for deferring UI integration when the mechanism is genuinely callable end to end.

#### Stage 7c — Fit-function consolidation and derived-feature functions · **Sonnet first, then Haiku**

**Only a `Point` is ever directly measured** — nothing else can be physically measured. Every fit function therefore consumes `Point`s, **resolved by tag (7f)**; `BestFitPoint` remains the sole function touching raw `Observation`s — the single bridge of D13. *(Written before 7f; the `PointCollection` binding it originally assumed is retired, but the consolidation work below is unchanged — only what feeds the inputs differs.)*

Current duplication, verified:

| Function | Consumes today |
|---|---|
| BestFitPoint | `Observation` (correct — the one true primitive) |
| BestFitPlane / BestFitLine / BestFitSphere / BestFitCircleInPlane | `Observation` |
| PlaneFromPoints / LineFromPoints / SphereFromPoints / BestFitCircleInPlaneFromPoints | `Point` |
| BestFitCircle (plain) | `Observation` — no Points variant exists |
| BestFitCylinder / BestFitCylinderFromPoints | `Direction` + `Observation` — **both**, despite the name |

- Remove every direct-observation fit variant in favour of its collection-consuming equivalent, building the missing ones (plain Circle has none; Cylinder needs a genuine points-based variant).
- **Cylinder is not an exception** — it is `PointCollection` with a minimum of 5 (2 DOF axis direction + 2 DOF point-on-axis + 1 DOF radius), the same shape as Plane's 3.
- **Declared minimums are advisory hints, not enforced gates.** Three points satisfy a plane's minimum and can still be collinear; no generic metadata can enumerate every degenerate arrangement. Real validation stays inside each function's `exec()`, which already fails on singular cases. The hint exists so a field user allocates enough data up front instead of discovering the shortfall after leaving site.
- **Derived-feature functions instead of multi-output generation** (D8): add `LineFromPlaneNormal`, `PointFromPlaneOrigin` and friends as ordinary construct functions. The user defines a new feature and picks the function that defines it — the existing, well-understood direction — rather than having a function spontaneously generate features.

**Split during implementation** — the two halves turned out to carry very different risk.

- **7c-i — derived-feature functions · DONE** (`openindy-default-plugin` 075a0ba). `LineFromPlaneNormal` and `PointFromPlaneOrigin` as ordinary construct functions, per D8. Purely additive, no breakage. `tst_derivedfeatures` moves and tilts the source plane and asserts the derived line and point follow — proving these are live derivations through the existing dependency graph, not one-off copies. Both refuse to derive from an unsolved plane. **Haiku can replicate this pattern** for further derived features (circle centre/axis, cylinder axis, etc.) once wanted.

- **7c-ii — fit consolidation · DONE** (`openindy-core` fitfunction.h digest refresh + plan/model docs, `openindy-default-plugin`). Unblocked by 7f as expected: every fit function but `BestFitPoint` now declares `ePointElement`, `roleName="default"` (+`"dummyPoint"` where applicable), following `PlaneFromPointCollection`'s 7f-established pattern. Each direct-observation fit and its `*FromPoints` twin collapsed into one function, keeping the shorter/canonical name:
  - `BestFitPlane` absorbed `PlaneFromPoints` and 7f's `PlaneFromPointCollection` (three variants → one).
  - `BestFitLine`, `BestFitSphere`, `BestFitCircleInPlane` each absorbed their `*FromPoints` twin.
  - `BestFitCircle` (plain) had no Points variant to absorb — converted in place.
  - `BestFitCylinder` / `BestFitCylinderAppxDirection` / `BestFitCylinderAppxDummyPoint` absorbed their `*FromPoints` siblings. Cylinder's declared minimum is 5, exactly as D9 anticipated — no special-casing needed.
  - **Two real pre-existing bugs found and fixed, not preserved**: `BestFitCylinderFromPoints`'s `init()` still declared `eObservationElement` despite its name and `setUpResult()` reading `element.point` — unreachable through ordinary binding, only "working" because its own unit test bypassed binding entirely. `SphereFromPoints` never ran the iterative least-squares refinement `BestFitSphere` did (approximation only) — the consolidated `BestFitSphere` now always refines.
  - **One core-header fix, not a rename**: `fitfunction.h`'s `BestFitCylinderUtil` "first two dummy points" approximation path read `element.observation` unconditionally in two places, so it silently produced a zero direction (and thus a failed fit) for the now-standard `ePointElement` dummy points. Fixed to match `BestFitUtil::getDummyPoint`'s existing type-dispatch. **Digest refreshed, `OI_PLUGIN_ABI_VERSION` left at 3**: `BestFitCylinderUtil` has no virtuals and no `.cpp` of its own — it is inlined entirely into whichever binary includes it, never compiled into `libOpenIndyCore.so` itself, so an already-built plugin binary's compiled code for this method is untouched by the header changing; it simply doesn't gain the fix until rebuilt. No struct layout, vtable, or enum change — the bump criteria in `pluginabi.h` don't apply.
  - New regression: `tst_measuredfit` (`openindy-default-plugin`) drives `OiJob::addMeasurementResults` → tag resolution → the real `BestFitPlane`/`BestFitCylinder` end to end, with zero manual `addInputElement` calls — the missing link `tst_measurementintent`'s `FakeConsumer` (core, no plugin to link) couldn't prove.

**Acceptance (7c-ii):** no fit function except `BestFitPoint` declares `eObservationElement` — verified. Every existing fit workflow produces the same geometry — verified by the existing `tst_function` regression suite (converted from `addInputObservations` to `addInputPoints`, same expected numbers, same tolerances) plus the new `tst_measuredfit`. `ctest` green in all repos.

#### Stage 7h — Function-owned sub-geometry · **Sonnet first, then Haiku** · after 7c-ii · **DONE (Plane; other geometry types left for Haiku)**

Reference geometry a feature exposes for free: `Actual.NormalLine`, `Nominal.PointOnPlane`, `Actual.MidPoint`, `Actual.Length`. The user's framing: *"in SA you just check a box in a relationship for generating that geometry, and it gets populated in the tree and becomes bound."*

**The cheap version, thanks to D19.** These are **real features in the tree, owned by the producing function and read-only** — not a `<feature>.<realisation>.<accessor>` path syntax. The existing input machinery works unchanged, because a generated sub-feature is just a feature. That deletes an entire addressing system from this plan.

- **They are persisted in full, with stable feature ids.** Regenerating on load would mint new ids and dangle every input referencing them — the same stable-identity trap as `MeasurementConfigKey` colliding by name and the git-derived plugin IID. **Ownership is about authority, not storage**: who may change it and what it dies with, not whether it is written to disk.
- **Computed values persist too**, not just the declaration. Opening a project must not force a full recompute of the dependency graph — trivial for a plane normal, potentially not for a long construction chain. The existing `isUpdated` flag is the hook; loading restores values and marks clean.
- The owner regenerates them on recompute, so they cannot drift.
- **Amends D8** (see the decisions log): D8's objections were about *unowned* generated features. Ownership answers both.
- 7c-i's `LineFromPlaneNormal`/`PointFromPlaneOrigin` keep the narrower case: an **independent, editable** feature rather than a bound one. Both remain valid; they are different intents.
- Sonnet does the ownership plumbing and the first geometry's accessor set; **Haiku replicates** per geometry type.
- Scalars (`Actual.Length`) need a scalar feature type — check what `ScalarEntity*` already provides before adding one.

**Acceptance:** ticking a plane's "normal line" makes a bound, read-only line appear in the tree; moving the plane moves it; attempting to edit it is rejected naming its owner; deleting the plane deletes it; **a second feature referencing the generated line still resolves after save/reload — its id is unchanged**; reloading does not re-execute the construction chain.

**What shipped (`openindy-core`, `openindy-default-plugin`, `openindy`).**

- **The mechanism, generic across geometry type, not Plane-specific.** `Geometry` gains `ownerFeature` (`std::optional<QPointer<Geometry>>`) and `ownedSubGeometries` (`QList<QPointer<Geometry>>`) — the reverse pair to `realises` (D17), same shape, same reasoning. `getOwnerDescription()` checks `ownerFeature` *before* the existing `FitFunction` check, so **the exact same `ConstructFunction` class is user-owned when attached directly (7c-i's path) and read-only when owned** — ownership is not a property of the function's type, it is recorded once by whoever generated the accessor. This is the key simplification that made Plane's accessors need zero new plugin code (see below).
- **`OiJob::addOwnedSubGeometry(owner, subGeometry, producerFunction, neededElementsIndex)`** (`openindy-core`) is the one new generic primitive: attaches the function (`Feature::addFunction`), binds the owner as its input (`OiJob::addInputFeature`, which wires the ordinary `previouslyNeeded`/`usedFor` dependency edges), and records ownership (`Geometry::setOwnerFeature`, which registers on both sides). No parallel wiring mechanism — this is the same machinery 7c-i's independent feature uses, called through one entry point instead of by hand.
- **Regeneration reuses the existing DAG walk, not a second recalculation path.** `Geometry::recalc()` snapshots its own position/direction/radius/`isSolved` before re-running its function list and, only if the value actually changed, sets `isUpdated=false` on its owned children. `FeatureUpdater`'s three blanket "mark everything not-updated" loops (`recalcFeatureSet`, `recalcFeature`, `recalcFeatureSetWithoutTransformation`) now skip function-owned sub-geometry instead of unconditionally dirtying it. The two together are what make an owned accessor: (a) regenerate automatically when its owner's value moves, through the same walk as everything else, and (b) **not** get re-executed by a recalc pass over an otherwise-unchanged graph — including the recalc `Controller::loadProject` triggers on every load.
- **Persistence**: `Geometry::toOpenIndyXML` (`openindy-core`) writes `<ownerFeature ref="ID"/>` when set, same shape as `<realises>`. `ProjectExchanger::restoreGeometryDependencies` (`openindy`) resolves it in the same second pass as `realises`, and marks the geometry clean (`setIsUpdated(true)`) once resolved and solved — its position/direction/radius were already restored by the geometry's own ordinary `fromOpenIndyXML` (Line/Point/Plane already round-tripped these before this stage; nothing new needed there).
- **Deletion cascade**: `OiJob::removeFeature` (both overloads) now calls a new private `removeOwnedSubGeometries` before removing the target, which recurses through the ordinary `removeFeature(int)` path for each owned child (not a raw `delete`), so cleanup, signals and container bookkeeping all happen normally.
- **Plane's accessor set needed no new plugin code at all.** `LineFromPlaneNormal`/`PointFromPlaneOrigin` (7c-i, `openindy-default-plugin`) are used unchanged as the owned accessors' producer functions — `OiJob::addOwnedSubGeometry` is what turns the same class into a bound, read-only feature instead of an independent one. `openindy-default-plugin/test/ownedplaneaccessors/tst_ownedplaneaccessors.cpp` drives this end to end with the real classes: generated line/point compute the right normal/origin, name their owner, move with the plane on recalc (and are left alone when the plane recalculates to the same value), reject a direct edit, and get deleted when the plane does.
- **Plugin ABI bumped 3 → 4.** `Geometry` gained two data members. It lives outside `include/plugin/`, so `tst_pluginabi`'s fingerprint does not cover it (documented limit, domain-model.md §7) and the test could not have caught a missed bump here — this is exactly the judgement call that note exists for. Digest refreshed (hash unchanged, since `include/plugin/**` itself wasn't touched — only the recorded version number moved).
- **New tests**: `openindy-core/test/ownership/tst_ownership.cpp` gains six cases for the generic ownership plumbing (owner naming, edit rejection, both-sides registration, self-ownership rejected, XML write side, destructor cleanup unregisters from the owner). `openindy-core/test/ownedsubgeometry/tst_ownedsubgeometry.cpp` is new and covers the `OiJob`-level mechanism with a plugin-agnostic fake `ConstructFunction` (attach/bind/ownership wiring, change-gated regeneration, a second feature consuming the owned child as an ordinary input, cascade-delete). `openindy-default-plugin`'s `tst_ownedplaneaccessors.cpp` (above) is the Plane-specific, real-function integration test.

**Deviated from scope, noted explicitly rather than silently:**

- **The full "second feature references the owned line, survives save/reload with the same id" acceptance test was not written as an app-level `ProjectExchanger` round trip.** The write side (`<ownerFeature ref=.../>`) and the resolve side (`restoreGeometryDependencies`, marking clean) are both implemented and follow the `realises` precedent exactly (which *does* have this coverage, in `tst_ownership::realisesRoundTripsThroughXml`, on the write side only — resolution has never had a dedicated app-level test either, for the same reason: `ProjectExchanger` reconstructs a project's functions via `PluginRepository`/`PluginLoader`, i.e. by dynamically loading a real plugin `.so` from `qApp->applicationDirPath() + "/plugins"`, which is exactly what the running app does but is nontrivial to stand up cleanly inside a `QTest` fixture in the time available this pass). Confirmed instead by: the id-stability mechanism itself (`Element::fromOpenIndyXML` preserving the persisted `id` rather than minting a new one) is exercised by every existing project load, and `tst_ownedplaneaccessors`/`tst_ownedsubgeometry` both prove a second feature resolves an owned child as an ordinary input pre-save. **Left for a follow-up pass**: an app-level test target (mirroring `tst_dialogs`'s `QT_QPA_PLATFORM=offscreen` setup) that builds a job with a plane, an owned line, and a consumer, round-trips it through `ProjectExchanger::saveProject`/`loadProject` with the real default plugin discovered from the build's own deployed `bin/plugins/`, and asserts the consumer's input still resolves to the same id.
- **No UI trigger was built** (no checkbox/button in a properties dialog). `OiJob::addOwnedSubGeometry` is the full, real, callable trigger the UI would call; wiring an actual widget was cut for time given the plumbing was the larger risk and the explicit latitude in the task to defer UI integration when the mechanism is genuinely callable end to end.
- **Scalars (`Actual.Length`) are untouched**, as the plan anticipated might happen — `ScalarEntityDistance` (`openindy-core/include/geometry/scalarentitydistance.h`) is an ordinary `Geometry` subclass with one scalar unknown instead of a vector, so it needs no new type, only a scalar-producing accessor function (e.g. a plane-to-plane or point-to-point distance) and goes through the exact same `addOwnedSubGeometry` mechanism. Left for Haiku, per the plan's own scoping call.

**For Haiku's replication pass**: the generic parts are `Geometry::ownerFeature`/`ownedSubGeometries`/`getOwnerDescription`, `OiJob::addOwnedSubGeometry`/`removeOwnedSubGeometries`, the `Geometry::recalc()` change-detection, `FeatureUpdater`'s three reset-loop skips, and `ProjectExchanger`'s `<ownerFeature>` resolution — none of that is Plane-specific and nothing there needs touching again. What's Plane-specific (and what changes per geometry type) is only the producer function(s) passed to `addOwnedSubGeometry` and the accessor's own math — for Sphere/Cylinder/Circle that means new or existing `ConstructFunction`s (`SphereCentre`, `CylinderAxis`, `CircleCentre`, ...), most of which do not exist yet and are exactly 7c-i-shaped work.

#### Stage 7d — Formalize producer/transform in the function list · **Sonnet**

The execution model already works this way (verified fact 1) but nothing declares it, so nothing can validate it or present it correctly.

- A `Function` declares its role: **producer** (consumes input elements, defines the feature — must be position 0, exactly one) or **transform** (consumes and returns the same geometry type — positions 1..n, any number).
- Validate on attach: a second producer, or a transform at position 0, is rejected with a clear error rather than silently producing nonsense.
- The existing `at(0)`-is-the-producer assumptions (`featureupdater.cpp:142-145`, `oijob.cpp`'s fit-function check) read the declared role instead of assuming by position.

**Acceptance:** attaching two producers to one feature is rejected with a specific message; every existing function declares a role; all existing recalculation behaviour is unchanged (regression suite green).

#### Stage 7e — `Group` as organization only · **Sonnet** · scope much reduced by 7f

**Shrunk deliberately.** Tags (7f) handle *consumption*, so `Group` is never a function input. It keeps one job: **organization** — the collapsible tree node of Stage 9, a folder for a named set, and the answer to the thousands-of-rows scale problem. Registration spheres are found by tag like everything else, not gathered into a `Group`.

The original scope below is retained for the reasoning it records (particularly the constellation distinction, which still matters), but the "selectable as function inputs" half of it is gone.

- **One `Group` class**, not `PlaneGroup`/`SphereGroup` per type: name + `QList<QPointer<FeatureWrapper>>` + an *optional* type constraint enforced at add-time and/or execution-time.
- **Reconciled with 7f: `Group` is an organizational folder, not a fit-binding mechanism.** Fits find their inputs by tag (7f) refined by pipeline (7b); `Group` exists for the tree, for reuse across features, and for the scale problem in Stage 9. The two roles `PointGroup` was carrying — provenance and intent — are now separate, which is why it stopped making sense as one type. `PointGroup` as a *stored membership list feeding a fit* is retired; `PointCollection` (the interface) and `PointCloud` are unaffected.
- **Registration spheres are a `Group`, emphatically not a `CalibratedConstellation`** — a constellation's relative geometry is *certified and known going in* (solve for pose given trusted geometry); registration spheres' relative positions are exactly what is *being solved for*. Do not conflate.
- A quick manual pick still creates a **real, explicitly named `Group`** — no anonymous domain object. The UI may auto-generate a default name and create it transparently so it never feels like ceremony, but the model never carries a "doesn't really count" special case.

**Acceptance:** a `Group` of planes and a `Group` of spheres both persist, reload, and are selectable as function inputs; a type-constrained group rejects a wrong-typed member with a clear error.

---

### Stage 8 — `InspectionPlan` · **Sonnet**

*(Renamed from "MeasurementPlan" — that term is Spatial Analyzer's. `InspectionPlan` matches oil & gas ITP language and describes the object better: what must be verified, to what criteria.)*

**A derived query over the job, holding no state of its own.** Walk every `Function`'s `NeededElement`s across every feature and report what is acquired versus outstanding. Recomputed on demand — it cannot drift out of sync because there is nothing to sync.

The workflow it serves, in the user's own framing: build geometry and functions in the office (**that authoring *is* the plan** — an unresolved requirement is outstanding work by construction); take it to the field and iterate the outstanding entries, tweaking geometry as needed; close out; report.

`Geometry` carries **zero** configuration state — verified fact 6 means "how was this measured" is already answerable by walking down from the result. A staged config is a parameter at the moment of action, not a domain fact to persist.

**The distinction that needs resolving here** (raised by the user, not yet settled): an outstanding entry has two genuinely different shapes.

1. **Enumerable** — a `PointGroup` referencing 10 specific planned-but-unmeasured points. The plan can list them: "measure P1…P10." Existing nominal/actual machinery (verified fact 2) is the natural expression: a planned point is a nominal with no actual.
2. **Open-ended** — an empty `PointGroup` that needs populating with an unspecified number of points. Nothing to enumerate; the entry is "acquire points into this group," possibly with a declared target count.

**RESOLVED by D17.** Neither case needs a "planned" flag, and the nominal/actual pairing it would have used is gone. A planned feature is simply one whose producer is a **nominal** function and which has no actual to compare against yet — case 1 enumerates the nominal points that exist, case 2 is a requirement with no nominals declared. Both fall out of walking producers; nothing new to store.

**Also unresolved, blocks the accuracy half of this stage:** target accuracy is declared at feature/project level (confirmed), but the user's own example — a length needing ±0.1mm allocating down to ≤0.05mm per contributing point — implies targets can be declared on a **derived** feature and propagate *down* the function graph. Whether that's the intent, or targets are always declared directly per-point, is not settled. **Build the structural half of `InspectionPlan` first; the accuracy half waits on this answer and on bundle adjustment (verified fact 4).**

**Acceptance (structural half):** a job with several partially-satisfied fit functions produces a correct, complete outstanding/acquired list; adding points to a bound group moves entries from outstanding to acquired without any explicit plan update; regression test in `ctest`.

---

### Stage 9 — `Group`-aware feature tree · **Sonnet**

The scale worry ("thousands of rows") is presentation, not domain — deliberately deferred as an optimisation question, but the *tree* is a real usability requirement now.

- `Group` becomes a collapsible node; a feature belonging to at least one group nests under it rather than also cluttering the flat top level.
- "Add new point" (creates a feature + adds a reference; also the programmatic bulk-acquisition entry point) and "Add existing point" (reference only, no new measurement).
- Same underlying reference model — rendering only.

**Acceptance:** a job with 1000 points in one group renders as one collapsible node; both add operations work; no domain-model change in the diff.

---

### Stage 10 — Typed parameters across every plugin interface · **Sonnet first, then Haiku**

*(Was "Stage 3b" — renumbered; it was never actually sequenced where its old number implied.)*

D3 turned out bigger than scoped: the `integerParameter`/`doubleParameter`/`stringParameter` three-map pattern is duplicated independently across `Sensor`, `Function`, `SimulationModel`, and `BundleAdjustment` — 40 files. Migrating only the sensor-facing maps would leave two parallel parameter systems side by side.

- One shared `ParameterValue` (`std::variant<BoolValue, RangedInt, RangedDouble, EnumString, FreeString, IPAddress>`), used by all four interfaces.
- `SensorConfiguration`'s four maps migrate first — the existing generic parameter UI (`sensorparametersmodel.cpp`, `sensorparametersdelegate.cpp`) dispatches via `std::visit` instead of "which map did this row come from," proving the shape.
- **Sonnet** does `SensorConfiguration` + the UI; **Haiku** replicates across `Function`, `SimulationModel`, `BundleAdjustment`.
- Metatype hazard: the variant's **first alternative must be default-constructible**.

**Acceptance:** all 40 files build against the new type; `PseudoTracker`'s 16 error terms and `LeicaTachymeter`'s reflector/ATR options round-trip unchanged; `ctest` green in all repos.

---

### Stage 11 — Format-agnostic persistence · **Sonnet**

D5, decided long ago and never scheduled. `toOpenIndyXML`/`fromOpenIndyXML` leaks storage format into every domain class, and Stages 7–9 add `Group`/`PointCollection` persistence that would otherwise deepen the debt.

- Touched classes gain `toValue()`/`fromValue()` over `QVariantMap` — lossless to both JSON and XML.
- Do **not** attempt a big-bang migration of every class; convert what Stages 7–9 touch plus `Geometry`, and leave the rest behind the same interface.
- Fixes a known asymmetry: `Geometry::toOpenIndyXML` writes the staged config but `Geometry::fromOpenIndyXML` never reads it back — `ProjectExchanger` does it externally as a workaround.

**Acceptance:** a project round-trips through both JSON and XML with identical resulting object graphs; no domain class touched by Stages 7–9 mentions `QDom`.

---

# Deferred — real, intended, explicitly not now

Deferred so they are built on the corrected model rather than bolted to the current one. **Nothing here is cancelled.**

- **Bundle adjustment.** The largest capability gap in the project (verified fact 4) and SA's USMN equivalent. The accuracy half of Stage 8, the user's instrument-fallback scenario, and the multi-station pipe workflow all depend on it. Deferred by explicit decision: nail the domain model first.
- **GUM-style uncertainty budgets and traceability chains** — coverage factors (k=2), instrument calibration certificates, defensible stated uncertainty. `CalibratedConstellation.certificateId` is the right instinct already; instruments need the same.
- **Tolerance / GD&T evaluation** — flatness, cylindricity, position tolerance, datum reference frames. None exist in core today. This is the actual "inspection" half of an inspection plan.
- **Reporting** — the workflow's real deliverable ("go to the office and start reporting"). Mostly a read-out-and-format problem once the model is right.
- **Parametric nominals — job-level named parameter table.** A table of named scalars that nominal producers reference instead of holding literals; change `bend_radius` and every nominal built from it recalculates through the dependency graph that already exists. Driving case: a pipe-bend schedule where each bend has its own radius, angle and tangent lengths. This is the Inventor/SolidWorks model applied to metrology nominals, and it is a genuine differentiator — Spatial Analyzer's answer is "buy Ultimate and script it". **Purely additive after D18** (nominal producers already take typed scalars), so deferring costs nothing. Wants Stage 10's `std::variant` parameters underneath. Full CAD-style expressions *between* parameters (`tangent = radius * 1.5`) — parser, evaluation order, cycle detection — remain out of scope.
- **CAD interop** (STEP/IGES nominals from Inventor/SolidWorks).
- **Scale/performance** — `QObject`-per-`Point` at 10k+ features is unmeasured. Get the model right, optimise later.
- **mrcal export path** for board pose → camera calibration (needs mrcal's real extrinsics contract checked, not guessed).
- **`Accessory` catalog** — target-offset hardware (SMR/prism/stylus), reference artifacts, jigs.
- **Richer pipe/duct feature type** — a real pipe deforms asymmetrically and one rigid mathematical cylinder can't represent it; measuring both ends via the DCN is the current workaround.
- **Additional `SensorType`s** — structured-light scanner (a stated direction for the user), CMM, probe arm, GNSS, photogrammetry.
- **Per-generation Leica driver split** — not started deliberately; one driver and no evidence yet of where it needs to fork.
- **CI** — none anywhere; the per-task loop below is the only gate.
- **Automated segmentation / feature recognition** — auto-sectioning a point cloud or a V-STARS batch and aligning it to what you're trying to measure. Explicitly out of scope by the user's own call: months-to-years of research. Until then, using V-STARS or a scanner means accepting responsibility for hand-allocating measured data — which Stage 7f makes a first-class operation (retroactive tagging) rather than a workaround.
- **UI text pass** — no translation layer exists anywhere (no `.ts`/`.qm`, no `QTranslator`, 6 `tr()` calls total). Capitalisation and wording live in 23 `.ui` files (~515 strings, 133 in `mainwindow.ui`) and in plugin `metaData.name`/`description`. **User is doing this manually, in parallel.** The type-name tables in `openindy-core/src/util/util.cpp` are *excluded* from that pass — they are also written into saved projects as XML `type` attributes and read back with an exact, case-sensitive `QMap::key()` lookup (`util.cpp:758`), so re-casing them silently turns loaded geometries into `eUndefined`. Splitting the persisted token from the display name belongs to **Stage 11**.

---

# Deliverable — check this design into the repo

Two files in **`openindy-core/docs/`**, because almost everything decided here is core's (`Point`, `Observation`, `Geometry`, nominal/actual, D19 ownership, the plugin ABI), so the rationale should sit beside the code it governs.

1. **`domain-model.md` — the distillation.** The durable design: the decisions log, the verified facts, the domain model rationale, and critically the **considered-and-rejected** reasoning — observations as fit input, one-geometry nominal/actual, regenerable owned sub-geometry, `ObservationCollection`, a separate `openindy-plugin-api` repo, spatial clustering for correspondence. In six months that reasoning is worth more than the conclusions, because without it someone re-proposes the rejected option. Ages well: it contains only things that stay true.
2. **`plan-domain-model-rework.md` — a verbatim copy of this plan.** Stage sequencing, model assignments, acceptance criteria. Goes stale as stages complete, which is fine — it is the record of what was intended and in what order.

`domain-model.md` is written **first, from this plan**, and cross-references the verbatim copy. Keep both updated at stage boundaries alongside the code commits.

# Agent execution protocol

**Model assignment is fixed per stage above.** The rule behind it: **Haiku** for mechanical, pattern-established, compiler-or-test-verifiable work (mass call-site updates, replication of an established pattern, dead-code removal). **Sonnet** for design-bearing, cross-cutting, threading-related, or unfamiliar-code work. Where a Haiku task's pattern isn't established yet, **Sonnet does the first instance and Haiku replicates** — Stages 7c and 10 are explicitly structured that way.

**Per-task loop, mandatory:**
1. Read the stage's acceptance criteria first, and re-verify any "verified fact" the stage depends on.
2. Build all repos from `openindy/build` (`cmake --build . -j$(nproc)`) — zero new warnings. Allow >2 min; a full build exceeds short timeouts.
3. `ctest` in `openindy-core/build`, `openindy-default-plugin/build`, `openindy/build`, `openindy-leica-plugin/build`.
4. Headless smoke: `QT_QPA_PLATFORM=offscreen QT_FORCE_STDERR_LOGGING=1 timeout 5 ./bin/openIndy -s`; check `oiLogFile.log`.
5. Commit at stage boundaries only, never mid-stage. Split commits per repo. **No `Co-Authored-By` trailers** (standing rule).

**Escalate, don't improvise.** Stop and ask when:
- A stage's acceptance criteria can't be met as written.
- A stage carries an explicitly unresolved question (Stages 7a, 8 both do) — **do not silently pick a side**.
- A new type crosses the `SensorWorker` thread boundary without a metatype registration.
- A change would make a currently-passing test fail.
- **The design itself looks wrong mid-implementation.** Two Stage 6 attempts were built and discarded precisely because this happened; discarding was correct both times. A wrong design discovered early is a finding, not a failure.

**Report honestly.** Failing tests get reported with output. Partial completion gets named as partial. Never report green on a build that didn't run. Distinguish three things explicitly: a real regression, a test-setup gap (e.g. missing `ChooseLALib::setLinearAlgebra` in `initTestCase`), and a pre-existing bug newly exposed.

---

# Design decisions log

Full market research: [artifact](https://claude.ai/code/artifact/9cb43de6-2b95-4b8a-9586-4addf053f242).

- **D1 — `SensorType` is one open concept.** `*_iidd` constants are shared ABI version tags identical across every plugin of a type, carrying no per-instance identity. A closed enum means a core release per new instrument type. Type drives identity, UI presentation, and shared behaviour — splitting "category" from "type" was over-abstraction. *Proven by Stage 5: a whole instrument family now lives outside the core repos with zero core changes.*
- **D2 — Discrete vs. continuous is a driver capability orthogonal to type, unrelated to point count.** V-STARS emits thousands of points from an instant snapshot (discrete); a terrestrial scan is start→wait→result (discrete) at millions of points; ATR tracking returns one point at a time (continuous). The axis is *live interruptible stream vs. batch*.
- **D3 — Parameters use `std::variant`.** Constraints validated in each alternative's constructor, dispatch via `std::visit` — a missed case is a compile error, not a runtime check.
- **D4 — Accuracy is attached per measurement by the driver, not looked up from a model spec.** On a TS15, reflector-based is more accurate than reflectorless and some capabilities (ATR, target lock) don't exist reflectorless at all. Vendor spec sheets are *nominal display data* only. The driver alone knows mode, target type, distance, environment, and instance calibration at measurement time.
- **D5 — Persistence stays format-agnostic at the domain API.** `toValue()`/`fromValue()` over `QVariantMap`. → Stage 11.
- **D6 — Calibration is an N-point constellation, not a scalar.** The distinguishing fact is *certified relative geometry with its own uncertainty*. Scale bar = 2 points; vector bar = 3 (one derived); calibration board = 7; 6DoF probe = 6–7 + derived tip. **A DCN differs fundamentally**: its points are already absolute, uncertainty is on position, no pose-solve — you don't pick a DCN up and move it. A calibrated jig is the middle case: once pose-solved for a session, its points function as a session-local DCN.
- **D7 — Protocol openness is rare.** Four open standards across ~20 vendor stacks surveyed (GeoCom, NMEA/RTCM/NTRIP, I++ DME); every laser tracker and scanner vendor is closed or NDA-gated. GeoCom being open is why the Leica driver was feasible at all.
- **D8 — Derived features come from declared functions, not from functions generating features.** The `GenerateFeatureFunction` multi-output hook stays unwired *deliberately*. Having a function spontaneously create features inverts the established direction (define a feature, then choose the function that defines it) and is harder to reason about, persist, and recalculate. `LineFromPlaneNormal`/`PointFromPlaneOrigin` as ordinary construct functions achieve the same outcome within the existing mechanism.
  - **Amended by D19.** A function *may* generate sub-geometry when it **owns** it: read-only and regenerated on recompute, so it cannot drift. Both original objections were about unowned generated features. **Ownership is authority, not storage** — owned geometry is persisted in full with stable feature ids, because other features reference it as input and regenerating would dangle them; computed values persist too, so loading a project does not force a full recompute. Spatial Analyzer does this already — tick a box on a relationship and the geometry appears in the tree, bound. The 7c-i functions keep a narrower purpose: when you want an **independent, editable** feature rather than a bound one.
- **D9 — Only a `Point` is ever directly measured.** Nothing else is physically measurable. Every other geometry is fitted or constructed from points, so accuracy satisfaction is checked in one place rather than duplicated per fit-function type. *Sharpened by D13 (fits never consume observations) and by the correspondence resolution below (one aiming event = one `Point`).*
- **D10 — A `Group` is organizational and holds references, never copies.** A `Point` is the sole first-class citizen and may belong to many groups at once. Directly targets the copy-paste-heavy manual reuse that Spatial Analyzer forces.
- **D11 — The plugin ABI version is declared by hand, never derived.** It answers "did the contract change?", which no automatic source can. Derived-from-git was tried and failed in both directions: it churned when nothing broke, and would run backwards on a revert. It lives in a header rather than a build-system define so that changing it forces dependent rebuilds, and is guarded by a fingerprint test because the dangerous failure is a *missed* bump, not a spurious one. **Any stage that changes a type crossing the plugin boundary must bump it.**
- **D12 — Measurement intent is captured at acquisition as a tag, not as a binding.** A `Point` carries a list of `MeasurementIntent{featureId, roleName}`; a fit's point input is a *query* over those tags, not a stored membership list. This is survey feature-coding, and it is what removes manual backfilling: re-measure and the fit re-finds its points. Anchored to `featureId + roleName` (not a name string, not a positional index) so renaming a feature and swapping its fit function both leave tags intact. Hand-allocating a V-STARS batch is the same operation done later.
- **D13 — Exactly one function bridges observations into geometry: the one producing a measured `Point`.** Everything above that line consumes `Point`s. A `Point` *with* observations is a measurement; a `Point` *without* is derived — the distinction is visible in the data, needing no flag and no subtype. Fits above the bridge must not consume observations: such a fit cannot tell one target measured five times from five distinct locations, and double-weights every cross-station tie point — a quietly wrong answer with a plausible residual. *An earlier wording of this decision said "fits consume Points, never Observations", which contradicted D9's "`BestFitPoint` is the sole function touching raw Observations". The bridge is the correct, narrower statement.*
- **D14 — A `Group` is an organizational folder, never a function input.** Superseded the fit-binding half of D10. `PointGroup` was carrying two independent jobs — *provenance* (where points came from) and *intent* (what they are for) — which is why it fought every use case; V-STARS separates them completely. Tags carry intent; `Group` carries organization and the tree-scale problem. `PointCollection` is retired with it; `PointCloud` survives as a container outside the function-input system. D10's many-to-many and references-not-copies both stand.
- **D15a — A `Point` is one physical target, as captured in one acquisition** — with however many readings the instrument took of it. TS15 at count=10 → one Point, ten readings; a tracker sampling 100 → one Point; V-STARS → a thousand Points, one reading each. **The meaning of a `Point` never varies by instrument; only how many an action yields.** So D2's driver capability declares *how many Points an action produces*, never *what a Point is* — otherwise swapping instruments would change the meaning of data already collected, which is the tracker-dies scenario.
- **D15 — Repeated measurement of one target produces separate `Point`s combined by a declared function.** There is no re-measure mode: one aiming event = one new `Point`, always. Twelve raw points from four nests × three sets become four derived points via `AverageFromPoints`, and the plane still fits from four. D8 applied consistently — combining is a declared function, not hidden acquisition behaviour — and it *removes* a branch from acquisition rather than adding one, while keeping every raw shot individually visible and rejectable.
- **D16 — Rejecting a point is an `Exclude` pipeline stage carrying a reason, not an untag and not a boolean.** The point keeps its tag; the fit records *why* it was excluded. Reversible, auditable, and survives re-measure — which is what an ITP requires. Untagging destroys the record; a per-element `isUsed` flag has nowhere stable to live once resolution is a live query. **Consequence: `7b` is a prerequisite for rejection working at all.**
- **D19 — Ownership determines editability and origin.** One rule, three cases: a **station** owns the points its observations produced (read-only, bound to the instrument that made them); a **function** owns any sub-geometry it generates (read-only, regenerated on recompute); the **user/job** owns features you create (editable). This replaces sniffing at flags or at a function's type to work out what something is — where it came from is structural.
- **D17 — Nominal and actual are two separate geometries; the actual references its nominal.** A nominal is *specified*, user-created, editable, produced by `NominalPlane(parameters)`, and carries **tolerance**. An actual is *realised*, owned by its fit (D19), read-only, and carries **uncertainty**. Both are the same geometric type, so no second type per geometry is needed. `isNominal`/`nominals`/`actual` are deleted.
  - **Decisive argument: you re-inspect things.** A weld checked quarterly, a foundation before and after loading — one nominal, *many* actuals over time. A single geometry with `Option<Actual>` has one slot and cannot express that.
  - It also dissolves the addressing ambiguity ("if I have a nominal plane with measurements, what do I refer to?") — two features, two names, nothing to qualify.
  - *Resolves the "planned vs outstanding" open question in Stage 8. Note the user's caveat: listing every geometry with a nominal producer is a reasonable way to build the plan, **not** an assumption that every nominal must be measured.*
  - *An earlier draft of this decision said "delete the pairing entirely, two independent features plus a comparison object". That over-read the intent: the complaint was that a bool plus two lists is untyped and lets the UI tick both, not that pairing is wrong.*
- **D20 — An `Observation` is a `Point`, owned by a `Station`.** Station-owned by D19, so read-only and bound to the instrument at its pose. This means a fit selects `Point`s and **intermingles observations with derived points freely** — the selection logic never asks which kind it has. There is no third "observed" epistemic kind: *observed* is simply a realisation whose producer is an instrument rather than a fit, and Stage 7d's producer role carries that, so nothing has to inspect a fit function's type to decide whether it can read observations.
- **D18 — Nominals take scalar parameters, so parametric nominals are additive later.** Nominal producers declare no element inputs, only typed scalars (Stage 10's `std::variant`). Literals first; a job-level named parameter table comes later and requires no model change, because a parameter simply becomes another thing a scalar input can reference — and the dependency graph already recalculates. Target case: a pipe-bend table of radius/angle/tangent length driving its nominal geometry. Full CAD-style expressions between parameters are explicitly out of scope.
- **Resolved — target correspondence** *(open since Stage 5)*. One aiming event = one new `Point`; the config's N readings are that point's N observations. Correspondence between repeated shots is expressed by D15's derived point, not by re-targeting. Spatial clustering rejected: it guesses worst where points are densest.
