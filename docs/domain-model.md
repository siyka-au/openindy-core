# OpenIndy domain model — decisions and rationale

This document records **why the domain model is shaped the way it is**, including the
alternatives that were considered and rejected. It is deliberately biased toward the
rejected options: the conclusions are visible in the code, the reasoning is not, and
without it someone re-proposes a rejected design in six months.

It contains only things that should stay true. For stage sequencing, model assignments
and acceptance criteria — which go stale as work completes — see
[`plan-domain-model-rework.md`](plan-domain-model-rework.md), a verbatim copy of the
working plan.

The goal this serves: **make the data model expressive enough that scripting is
optional.** Spatial Analyzer has a sound mathematical core, but its model cannot say
enough, so scripting becomes the escape hatch and users hand-backfill geometry the
software should have derived. "Buy the Ultimate version and script it" is the answer
this project exists to reject.

---

## 1. The load-bearing idea: ownership determines editability and origin

One rule, and most of the model falls out of it.

| Owner | Example | Editable |
|---|---|---|
| **Station** | a point produced by an observation | no — bound to the instrument at its pose |
| **Function** | a fit's generated normal line | no — regenerated on recompute |
| **User / job** | a feature you created | yes |

Consequences worth stating explicitly, because each replaces a worse mechanism:

- Nothing inspects a flag or sniffs at a function's type to decide what something is.
  Where it came from is **structural**.
- A fit does not ask whether an input is a measurement or a derived point. It selects
  `Point`s and intermingles them freely.
- **Ownership is authority, not storage.** Owned geometry is persisted in full with
  stable feature ids — other features reference it as input, and regenerating on load
  would mint new ids and dangle them. Computed values persist too, so opening a project
  does not re-execute the dependency graph.

---

## 2. Point, Observation, and what is actually measured

**Only a `Point` is ever directly measured.** Nothing else is physically measurable;
every other geometry is fitted or constructed from points. So accuracy satisfaction is
checked in one place instead of once per fit-function type.

**A `Point` is one physical target, as captured in one acquisition** — with however many
readings the instrument took of it.

| Instrument | One action yields |
|---|---|
| TS15, `MeasurementConfig` count 10 | one Point, ten readings |
| Laser tracker sampling 100 | one Point, a hundred readings |
| V-STARS snapshot | a thousand Points, one reading each |
| Terrestrial scanner | a `PointCloud` (not Points) |

**The meaning of a `Point` never varies by instrument; only how many an action yields.**
The driver capability declares the count, never the semantics — otherwise swapping
instruments would change the meaning of data already collected. That is not
hypothetical: the driving scenario is a laser tracker failing mid-job and a total
station finishing the work.

**An `Observation` is a `Point`, owned by its `Station`**, and therefore read-only.

### Rejected: fits consuming `Observation`s

Tempting, because `Observation` already carries a position, uncertainty, and a
many-to-many list of what it is for.

It is wrong because **a fit over observations cannot distinguish one nail measured five
times from five distinct locations**, and weights that spot 5×. The sharp case is
cross-station: the same nail seen from two stations is two observations and *one* point,
so an observation-consuming fit double-weights every tie point in the network.

That is not a crash. It is a quietly wrong answer with a plausible-looking residual,
which is the worst failure mode available in metrology.

### Rejected: `ObservationCollection` as the primitive collection

Same reasoning. Observations cannot carry target identity, so no collection of them can.

### Repeated measurement

**One aiming event = one new `Point`, always. There is no re-measure mode.** Combining
repeated shots is an ordinary derived `Point` with a declared function:

> Tracker dies, total station comes out. Four nests × three sets = **12 raw Points**,
> one observation each. Four **derived Points**, each averaging its three. The plane
> still fits from four points.

Project preparation is unchanged; only the function producing those four differs. This
keeps every raw shot individually visible, inspectable and rejectable, and it *removes*
a branch from acquisition rather than adding one.

### Rejected: spatial clustering for target correspondence

Automatic, needs no operator input — and guesses worst exactly where points are densest.
A 100 mm grid on a factory wall would be at the mercy of a tolerance value, and
misgrouping is silent.

---

## 3. Intent is a tag applied at acquisition

The operator always knows what they are shooting, and at that moment the knowledge is
free. This is survey feature-coding — shoot a point with a code, group by it later —
which has survived jobsites for decades.

- A `Point` carries a **list** of `MeasurementIntent{ featureId, roleName }`. A list,
  because a point shot for `wall-01` may also serve as a registration target.
- A fit's point input **resolves to a query**, not a stored membership list. Measuring
  auto-populates it; re-measuring re-evaluates it. This is what removes manual
  backfilling.
- **Hand-allocation and field acquisition are the same operation.** Dissecting a V-STARS
  batch in the office is stamping the same tag retroactively — no separate mechanism.

### Why `featureId + roleName` and not a name

A name is not identity. Rename `wall-01` and every tagged point orphans silently — the
fit does not error, it just quietly uses fewer points. This project has been bitten by
name-as-identity twice already (`MeasurementConfigKey` colliding across drivers; the
git-derived plugin ABI IID).

### Why not the needed-element index

It is **positional** — an index into one function's input list. Swapping a fit function
orphans every tag, and one point cannot serve two purposes. A stable role *name*
survives a function swap as long as the new function declares the same role, which
matters because the tracker-dies scenario is exactly "the function changed, the
measurements did not".

This replaces a half-built mechanism rather than adding one. `InputElementKey` is
`{eNotSet=-1, eDefault=0, /* ... */, eDummyPoint=10000}` with `NeededElement::key`
documented as *"if key not set InputElementKey is NeededElement index"*, and
`BestFitPlane` already declares two same-typed inputs distinguished only by that key.
`isDummyPoint` is a role expressed as a bool.

---

## 4. Nominal and actual

| | produced by | carries |
|---|---|---|
| **nominal** | a *specification* — a drawing, a model, a parameter | **tolerance** — what is allowed |
| **actual** | *realisation* — a fit over measurements | **uncertainty** — how well we know it |

**They are two separate geometries; the actual references its nominal.** The nominal is
user-created and editable; the actual is owned by its fit and read-only.

No second type per geometry is needed — both are a `Plane`. What differs is the
**producer**: `NominalPlane(parameters)` versus `BestFitPlane(points)`.

### Why two, not one geometry holding both

**You re-inspect things.** A weld checked quarterly, a foundation before and after
loading — one nominal, *many* actuals over time. A single geometry with an optional
actual has one slot and cannot express that.

It also dissolves an addressing ambiguity: with one geometry holding both, every input
and every accessor must say which half it means. With two features there is nothing to
qualify.

### Rejected: `isNominal` + `nominals` + `actual` on one `Geometry`

One object wearing two epistemic hats. The visible symptom was a UI that lets you tick
both nominal and actual, which means nothing.

### Rejected: deleting the pairing entirely

Considered, and it over-corrects. The complaint was that a bool plus two lists is
untyped, not that pairing is wrong — "you measure a nominal into an actual" is a real
operation worth keeping in the API.

### On type safety

`std::variant` + `std::visit` gives genuine exhaustiveness — a missed case is a compile
error. `std::optional` in C++23 has monadic operations (`and_then`, `transform`,
`or_else`). Together these get close to the Rust shape.

**`Geometry<G>` is impossible**: `moc` cannot process templated `QObject` subclasses. The
generic parameter has to go and the type is checked at runtime. This is the one place the
Rust model does not carry over, and it is a hard limit, not a preference.

---

## 5. Collections, groups and filters

- **A `Group` is an organizational folder, never a function input.** Tags carry intent;
  `Group` carries organization and solves the thousands-of-rows tree problem.
- **`PointCollection` is retired.** Tagging solves the binding-friction problem it
  existed for, and solves it better: a fit stores *one declaration* and resolves N
  points, so its input list cannot grow by construction rather than by wrapping.
- **`PointCloud` survives as a container**, outside the function-input system. It holds
  bare xyz structs in the millions, not `Point` features — you can tag fifty V-STARS
  targets but not 40,000 scan points.
- A fit's input is **a source plus a pipeline**. `ExplicitList` and `Exclude(reason)` are
  ordinary pipeline stages, which is what makes hand-picking and auto-tagging one
  mechanism instead of two.
- **Rejecting a point is an `Exclude` stage carrying a reason** — not an untag, not a
  boolean. The point keeps its tag and the fit records *why*. Reversible, auditable, and
  survives re-measure, which is what an inspection record requires. Untagging destroys
  the reason; a per-element `isUsed` flag has nowhere stable to live once resolution is a
  live query.

### Why `PointGroup` fought every use case

It was carrying two independent jobs: **provenance** (where points came from) and
**intent** (what they are for). A total-station workflow makes these look identical
because you measure straight into the thing you are fitting. V-STARS separates them
completely — one acquisition, thousands of points, forty geometries.

### The pipeline shape (Stage 7b, shipped)

**`NeededElement` carries an ordered `pipeline` (`QList<PipelineStage>`), literally.** Not a
parallel structure keyed by position elsewhere — the pipeline lives on the same declaration
a `roleName` does, so it travels with the function and persists the same way. A `PipelineStage`
(`include/plugin/function/pipelinestage.h`) is one of:

- **`ExplicitList`** — only these element ids survive. The hand-picking case with no rejection
  reason needed (a curated set, not an exclusion).
- **`Exclude(reason)`** — these ids are dropped, each carrying its own reason. **D16, verbatim:**
  the point keeps its tag; this is the recorded fact of why it did not participate.
- **`ResidualOutlierRejection(sigmaMultiplier)`** — the one computed stage this pass ships. Reads
  whatever residual the fit itself already reported (`Function::addDisplayResidual`, keyed
  `"v"` then `"vr"`) and excludes anything beyond `sigmaMultiplier * stdev`.

**The choke point is `Function::exec(const QPointer<FeatureWrapper>&)`**, not any individual
fit's `exec(Plane&)`/`exec(Point&)`/etc. Every subclass funnels through it (`Feature::recalc()`
calls exactly this overload), so pipeline application is generic across every fit type with
**zero changes to any fit function's own code** — they already read `InputElement::shouldBeUsed`
and call `setIsUsed`/`addDisplayResidual` (7c-ii groundwork). Concretely: apply deterministic
stages (`ExplicitList`/`Exclude`, need no fit result) → run the fit once → if a residual stage is
declared, evaluate it against that pass's `Statistic` → if it excluded anything, run the fit a
second time. Exactly two passes, never iterative reweighting to convergence — see §10.

**Pipelines are opt-in per role.** A `NeededElement` with no declared pipeline is left completely
alone; `Function::exec()` must never overwrite a `shouldBeUsed` nobody asked it to touch. This is
what keeps the pre-existing (if UI-less) `OiJob::setShouldBeUsed`/`Function::setShouldBeUsed`
working unchanged for every function that has not adopted a pipeline.

**Traceability**: `Function::getPipelineExclusions(position)` reports the current exclusion set
with reasons, re-derived on every `exec()` — not persisted separately, the same way `isUsed`
isn't. The declared pipeline itself *is* persisted (`Function::toOpenIndyXML`/`fromOpenIndyXML`),
and needed **no `ProjectExchanger` changes**: unlike `InputElement`, which holds object pointers
`ProjectExchanger` must resolve through its own id maps, a `PipelineStage` holds only ids and
scalars, so it round-trips through `Function`'s own XML methods alone.

**Same mechanism at observation level, verified not assumed.** `BestFitPoint` (the one function
touching raw `Observation`s, D13) needed no code changes either — a pipeline declared on its
`eObservationElement` position works identically, proven end to end against the real class in
`openindy-default-plugin/test/pipelinefilter/tst_pipelinefilter.cpp`.

**Why a value type, not a `QObject`/plugin-discoverable stage like `Function` itself.** The plan's
wording ("plugin-provided and parameterized, same shape as functions") reads as "stages should be
as extensible as functions are." The one stage this pass ships cannot be — it has to read whatever
residual the *owning* fit already reported, which only works evaluated generically at the one
choke point every fit funnels through, not through a per-stage virtual a plugin would implement in
isolation with no access to the fit's own `Statistic`. Building a second plugin-discovery ABI (new
interface header, IID, factory, `PluginRepository` wiring) for exactly one built-in stage is the
speculative filter library the plan explicitly says not to build. `PipelineStageType` can grow a
genuine plugin-discoverable shape later — the persisted XML shape and enum values do not depend on
how a stage's behaviour is dispatched — if a future stage genuinely needs custom, fit-specific math
a plugin author supplies.

### Deliberately out of scope: automated segmentation

Auto-sectioning a cloud or a V-STARS batch and aligning it to what you are measuring is
months-to-years of research. Until then, using V-STARS or a scanner means accepting
responsibility for hand-allocating data — which retroactive tagging makes a first-class
operation rather than a workaround.

### Shipped (Stage 7e): `Group` persistence, and the fit-binding retirement

The fit-binding mechanism this section describes as retired (`InputElement::group`/
`pointGroup`/`asPointCollection()`, the `ePointCollectionElement`-typed `NeededElement`
case in `OiJob`) is now actually gone from the code, not just superseded in practice —
confirmed by grep before deletion that nothing declared it any more. `ePointCollectionElement`
itself (the `ElementTypes` enum value) was deliberately kept: it still has a real,
unrelated use classifying a feature's type for display (feature-tree badges,
`FeatureUpdater::copyGeometry`), so "retired as a fit-binding mechanism" does not mean
"delete the enum value" — only the machinery that consumed it as a `NeededElement` type.

**A `Group` did not survive save/reload at all before this stage — not just its
membership.** `ProjectExchanger` never wrote a `Group`/`PointGroup` feature to the project
XML and never read one back; `Group::pendingMemberIds` (populated by `fromOpenIndyXML`
since Stage 7a-ii) was populated correctly but nothing above `Group` itself ever called
`fromOpenIndyXML` on a saved project, because `Group` was never included in `saveProject`'s
`<geometries>` section in the first place (`Group` is a `Feature`, not a `Geometry`, so
`FeatureContainer::addFeature`'s `!feature->getGeometry().isNull()` guard for
`geometriesList` was never true for one). Fixed with a dedicated `<groups>` XML section,
a `loadGroups` pass, and a `restoreGroupDependencies` pass in `ProjectExchanger` — the same
two-pass load-then-resolve shape §6 already uses for `realises`/`ownerFeature` — plus a new
public `Group::restoreMembers()` that resolves `pendingMemberIds` into real member
references. See §10 trap #14 for why this was missed for as long as it was.

A second, previously-dormant bug surfaced while writing the round-trip test for this:
`getIsGeometry(FeatureTypes)` never excluded `eGroupFeature`/`ePointGroupFeature`, so every
caller that guards a `feature->getGeometry()` access with it (including
`OiJob::checkAndSetUpNewFeature`, the only path that adds a fully-formed feature to a job)
null-derefed the instant a real `Group` reached it. Nothing had ever exercised this before —
no UI or controller code anywhere calls `OiJob::addFeature`/`addFeatures` with a
`Group`/`PointGroup` type — which is also the answer to "does a quick manual pick create a
real, named `Group`": there is currently no quick-pick path at all, and creating one is
Stage 9's job now that doing so will not crash.

---

## 6. Derived features

**Derived features come from declared functions, not from functions spontaneously
generating features.** Having a function create features inverts the established
direction (define a feature, then choose the function that defines it).

**Amended: a function *may* generate sub-geometry when it owns it.** Read-only,
regenerated on recompute, so it cannot drift. Both original objections were about
*unowned* generated features. Spatial Analyzer already does this — tick a box on a
relationship and bound geometry appears in the tree.

So there are two valid mechanisms with different intents:

- **Function-owned sub-geometry** (`Actual.NormalLine`, `Actual.MidPoint`,
  `Actual.Length`) — bound, read-only, maintained by its owner.
- **`LineFromPlaneNormal` / `PointFromPlaneOrigin`** as ordinary construct functions —
  when you want an **independent, editable** feature.

Owned sub-geometry needs **no new addressing syntax**. They are ordinary features in the
tree, so the existing input machinery works unchanged — which deleted a whole
`<feature>.<realisation>.<accessor>` path system from the design.

**Shipped (Stage 7h): ownership is recorded, not inferred from the function's type.**
`Geometry::getOwnerDescription()` checks a new `ownerFeature` field *before* the existing
"is the producer a `FitFunction`" check. This is deliberate: it means the exact same
`ConstructFunction` class (`LineFromPlaneNormal`, `PointFromPlaneOrigin`) is user-owned and
editable when a user attaches it directly (7c-i) and read-only when
`OiJob::addOwnedSubGeometry` attaches it and records an owner. **No new plugin code was
needed for Plane's accessor set** — 7c-i's classes work unchanged as owned-accessor
producers. This generalises past the FitFunction-vs-ConstructFunction split D19 originally
drew for nominal/actual: ownership is now a fact recorded about a *feature*, not something
inferred from *what kind of function* produced it.

Regeneration reuses the ordinary dependency-graph walk (`FeatureUpdater::
recursiveFeatureRecalculation`), not a second recalculation path — `Geometry::recalc()`
compares its own value before/after re-executing its function list and only dirties its
owned children (`setIsUpdated(false)`) when that value actually changed. Combined with
`FeatureUpdater`'s reset loops leaving owned sub-geometry's `isUpdated` alone (instead of
unconditionally clearing it, as they do for every other feature), an owned accessor
regenerates when its owner moves and is left untouched by a recalc pass over an otherwise-
unchanged graph — including the one every project load triggers.

---

## 7. Producer and transform are structurally distinct

A feature's function list conflated two jobs under one convention: whatever sat at
position 0 defined the feature from external inputs (a fit, a construct function, a
nominal producer); everything after refined that same geometry in place. Nothing declared
this — code simply assumed index 0, and the failure mode of getting it wrong was silent
nonsense, not a rejected input.

**`Feature` holds two containers, not one:** `std::optional<QPointer<Function>>
producerFunction` and `QList<QPointer<Function>> transformFunctions`. Two producers, or a
transform sitting where a producer belongs, is not a validated-and-rejected input — it is
a shape that cannot be constructed. Same instinct as D12 (id, not position) and D16 (a
recorded fact, not a boolean): make the invalid state unrepresentable instead of checking
for it.

### Rejected: a declared `FunctionRole` property on one list

The first-drafted version of this kept `functionList` as one container and added a role
enum, validated at attach time. Rejected once the structural-container version was on the
table: an enum can still be set wrong and only fails when something reads it; a container
with room for exactly one producer cannot be set wrong.

### The typing problem, and the wall it hits

The natural expression is generic — `Transform<T>::exec(T) -> T`, `Producer<T>::exec() ->
T` — but **`moc` cannot process a templated `QObject` subclass**, and `Function` must stay
`QObject`-derived for the plugin ABI (`Q_INTERFACES`, plugin discovery). This is the same
wall `Geometry<G>` hit (§4).

**Resolved by composition, not by giving up genericity.** Two plain C++23 templates,
never `QObject`, never seen by `moc`:

```cpp
template<typename T>
class Producer {
public:
    virtual ~Producer() = default;
    virtual std::expected<T, QString> exec() = 0;
};

template<typename T>
class Transform {
public:
    virtual ~Transform() = default;
    virtual std::expected<T, QString> exec(const T &input) = 0;
};
```

A concrete plugin class multiply-inherits both `Function` (the moc-visible,
plugin-discoverable base) and the mixin for its specific geometry type:
`class BestFitPlane : public Function, public Producer<Plane>`. `moc` only ever processes
`Function`/`BestFitPlane` — never the template itself.

### Why a producer takes no argument

`Transform<T>::exec` takes a `T` because there is a genuine prior value — whatever the
producer already built. A producer has nothing prior to consume; its actual inputs (a
`NeededElement`/`InputElement` list resolved by tag per D12, or scalar parameters for a
nominal producer per D18) already live as **member state** on the `Function` object,
populated by the caller before `exec()` runs — exactly how `Function` already works today.
An out-only signature is not a simplification that drops information; it is accurate
about where the information already is.

### Why `std::expected`, not `bool` plus in-place mutation

The pre-existing `exec(Circle&)`/`exec(Plane&)`/... family mutated an already-allocated
geometry in place and returned `bool`. These two mixins are new types with no legacy call
sites, so this is a clean place to actually apply a principle stated from the start of this
project but never yet used anywhere: `std::expected<T, QString>` carries a reason on
failure instead of a bare `false` — the same "make failure traceable, not silent" instinct
as D16's `Exclude(reason)`.

### Dispatch boundary

`Function::execByFeatureType`'s existing runtime switch (unavoidable — a caller only holds
a type-erased `FeatureWrapper`) is where the `dynamic_cast` to `Producer<ConcreteType>*` or
`Transform<ConcreteType>*` happens. A cast that fails **is** the "this function does not
belong in this slot" rejection — no separate validation step required.

### Shipped (Stage 7d): attach-time enforcement, and what stayed the same

**Runtime, not compile-time — D21's left-open call, resolved.** `Feature::addFunction()` is
the attach-time enforcement point (`Function::canActAsProducer`/`canActAsTransform`,
runtime `dynamic_cast` checks, same style as the dispatch boundary above). A compile-time
template API (`attachProducer<T>(Producer<T>*)`) was considered and rejected: the real
attach paths — the UI's function picker, `ProjectExchanger` restoring a saved project —
only ever hold a dynamically-loaded `Function*` with no compile-time geometry type, so a
template-constrained signature could not be used at the one call site that matters most.

**Which slot a function takes is decided by what it implements, not by attach order.**
`Feature::addFunction()` offers the producer slot only if the function implements
`Producer<T>` for this feature's type *and* the slot is empty; otherwise, if it implements
`Transform<T>`, it is appended as a transform *regardless of whether a producer exists at
all* — a station-owned `Point`'s value can come directly from measurement (`Point::recalc()`
computing a centroid from `observations`, D20), with no producer `Function` ever attached,
and a `Transform` (e.g. `Register`) must still be attachable on top of that. A function
implementing neither mixin for the feature's type — or a second `Producer` once the slot is
filled — is rejected outright, with a message naming the reason, and simply is not attached
(`Feature::addFunction()` still returns `void`; a caller cannot currently distinguish "did
not attach" from "attached" without re-checking `getFunctions()`).

**`SpecialFunction` (e.g. `openindy-default-plugin`'s `ConfiguredFunction`) is exempt from
the mixin check entirely.** It manages its own execution by overriding `execChecked()`
directly — running its own sub-function traversal (`CFVisitor`) — rather than ever reaching
`execByFeatureType`'s dispatch, so neither `Producer<T>` nor `Transform<T>` applies to it.
`Function::canActAsProducer`/`canActAsTransform` special-case any `SpecialFunction` to
`true` for exactly this reason.

**`Feature::getFunctions()` survives as a computed view — producer (if set) followed by
transforms, in declared order — for backward compatibility.** Dozens of pre-7d call sites
across all four repos (`FeatureUpdater`, `OiJob`, UI models, `ProjectExchanger`) index into
it positionally, on the standing convention that position 0 is the producer (verified fact
1). Rather than touching every one, `getFunctions()` reconstructs that exact ordering from
the two real containers on every call — position 0 is *guaranteed* to be the producer now
(when one exists), which is strictly more robust than the old convention-only guarantee
regardless of attach order. `addFunction`/`removeFunction` keep their pre-7d signatures too
(`removeFunction(index)` maps `0` to the producer slot, everything else to
`transformFunctions[index-1]`).

**The public `bool Function::exec(feature)` is now a thin wrapper over the new
`std::expected<void, QString> Function::execChecked(feature)`.** `Feature::recalc()` calls
`execChecked()` directly — the actual `std::expected` boundary the plan's acceptance
criteria describe — while `exec()` stays bool-returning and unchanged in signature for the
~150 pre-7d call sites (mostly `tst_function.cpp`) that call a function directly without
going through `Feature::recalc()`. Both are `virtual`, so a `SpecialFunction` overriding
`execChecked()` is picked up correctly through either entry point.

**A `Producer<T>`/`Transform<T>`'s returned `T` is written into the feature's real geometry
object via that type's own `operator=`** (`execAs<T>()` in `function.cpp`) — the same
pattern every migrated concrete class uses: build a fresh local `T`, mutate it with the
class's own pre-existing `setUpResult()`-shaped helper (untouched by this stage — only the
outer `exec()` entry point's signature changed), return it. This is why the migration
preserved every fit/construct function's actual math unchanged; see trap #12 below for the
one place this pattern needed a real fix, not just a wrapper.

---

## 8. Plugin ABI

`OI_PLUGIN_ABI_VERSION` in [`include/plugin/pluginabi.h`](../include/plugin/pluginabi.h)
is a **hand-declared integer**, never derived.

It replaced a version derived from `git describe`'s commit count, which changed every
plugin IID on every core commit whether or not the contract changed.

- **Declared, not derived** — no automatic source can distinguish "fixed a comment" from
  "added a pure virtual to `Sensor`", and a commit count runs *backwards* on a revert.
- **A header, not a compile definition** — a header is a tracked source dependency, so
  changing it forces every plugin that includes it to rebuild. A configure-time define
  let a stale build tree keep the old value and silently ship a mismatched binary.
- **`abiVersion` in `metaInfo.json`** — compatibility is checked from metadata *before*
  the library is instantiated, and a mismatch names the file, both versions and the fix.
  Previously a mismatch was indistinguishable from "not an OpenIndy plugin":
  `qobject_cast` returned null and an entire driver vanished with no message.
- **`tst_pluginabi` guards it** against the failure that matters — a *missed* bump, which
  lets a stale plugin load against a moved vtable. It fingerprints
  `include/plugin/**`, excluding comments, and fails when the surface moves without the
  version.

**Known limit:** the fingerprint does not cover layout changes to domain types outside
`include/plugin/` (`Geometry`, `Position`, `Reading`, `Observation`). Those are equally
ABI-breaking and still need judgement.

### Rejected: a separate `openindy-plugin-api` repo

It would not shrink the ABI surface. The plugin contract genuinely *is* most of the
domain model — `Sensor::measure` returns `QList<QPointer<Reading>>`, functions take
`Geometry&`, `InputElement` carries `Position`/`Direction`. Separating it relabels the
surface rather than reducing it. Revisit only if a genuinely thin interface emerges.

---

## 9. Other standing decisions

- **`SensorType` is one open concept**, resolved from plugin interface IDs. A closed enum
  meant a core release per new instrument type. Proven: a whole instrument family now
  lives outside the core repos with zero core changes.
- **Discrete vs continuous is a driver capability orthogonal to type, and unrelated to
  point count.** V-STARS emits thousands of points from an instant snapshot (discrete); a
  terrestrial scan is start→wait→result (discrete) at millions of points; ATR tracking
  returns one point at a time (continuous). The axis is *live interruptible stream vs
  batch*.
- **Accuracy is attached per measurement by the driver, not looked up from a spec sheet.**
  On a TS15, reflector-based is more accurate than reflectorless, and some capabilities
  do not exist reflectorless at all. Only the driver knows mode, target type, distance,
  environment and instance calibration at measurement time. Vendor specs are *nominal
  display data*.
- **Persistence stays format-agnostic at the domain API** — `toValue()`/`fromValue()` over
  `QVariantMap`, lossless to both JSON and XML.
- **Calibration is an N-point constellation, not a scalar.** The distinguishing fact is
  *certified relative geometry with its own uncertainty*. Scale bar = 2 points; vector bar
  = 3; calibration board = 7; 6DoF probe = 6–7 plus a derived tip. **A DCN differs
  fundamentally** — its points are already absolute, uncertainty is on position, and there
  is no pose to solve; you do not pick a DCN up and move it.
- **Registration spheres are not a `CalibratedConstellation`.** A constellation's relative
  geometry is certified and known going in; registration spheres' relative positions are
  exactly what is being solved for.
- **Protocol openness is rare** — four open standards across ~20 vendor stacks surveyed
  (GeoCom, NMEA/RTCM/NTRIP, I++ DME). Every laser tracker and scanner vendor is closed or
  NDA-gated. GeoCom being open is why the Leica driver was feasible at all.

---

## 10. Traps this codebase has already sprung

Recorded because each was silent, and the same class of bug keeps recurring.

1. **Name as identity.** `MeasurementConfigKey` was (name, configType) and collided
   silently across drivers. The plugin ABI IID was derived from a commit count. Tags
   anchor to ids for this reason.
2. **Display strings doubling as persisted tokens.** The type-name tables in
   `src/util/util.cpp` are written into saved projects as XML `type` attributes and read
   back with an exact, case-sensitive `QMap::key()` lookup. Re-casing `"circle"` to
   `"Circle"` for the UI silently turns loaded geometries into `eUndefined`. Splitting the
   persisted token from the display name is owed work.
3. **Exhaustive switches with `default:` arms.** Adding an enum value does not warn; new
   cases fall through silently and must be found by inspection.
4. **Degeneracy accepted as success.** `bestFitPlane` checked only for an empty point list
   and then returned `true` unconditionally. For collinear points the matrix is
   rank-deficient, two eigenvalues are ~0, and "the eigenvector of the smallest
   eigenvalue" picks arbitrarily — garbage normal, reported as success. A declared minimum
   point count is a **hint**, never a gate; real validation belongs in `exec()`.
5. **Missing metatype registrations.** Every type crossing the `SensorControl` →
   `SensorWorker` thread boundary needs `Q_DECLARE_METATYPE` + `qRegisterMetaType`. A
   missing one fails at *runtime*, silently. Already bitten once, with the wrong type
   registered under the right name.
6. **Uncomputed fields nobody noticed.** `Observation::sigmaXyz` was never populated
   anywhere — `transformAllObsToDestSystem` copied `xyz`/`ijk` from their originals but
   not sigma.
7. **A declared input type that didn't match what the code actually read.**
   `BestFitCylinderFromPoints` declared `eObservationElement` despite its name;
   `setUpResult()` read `element.point`. It was unreachable through ordinary `OiJob`
   binding — no real caller could ever populate it correctly — and "passed" its own unit
   test only because that test bypassed binding and stuffed Point-shaped `InputElement`s
   in directly. A test that skips the real wiring path can make dead code look alive.
8. **A partial reimplementation silently dropped a refinement step.** `SphereFromPoints`
   never ran the iterative least-squares refinement `BestFitSphere` did — approximation
   only, despite looking like a drop-in sibling. Found only by reading both
   implementations side by side during consolidation, not by any test disagreeing.
9. **A residual-based outlier filter is only unmasked by its own outliers once the sample is
   large enough.** `ResidualOutlierRejection` (Stage 7b) computes its threshold from a `Statistic`
   whose `stdev` is itself computed *including* whatever outliers are present — a single gross
   outlier inflates the very stdev meant to catch it. For `k` outliers of roughly equal residual
   magnitude among `N` points, the threshold only exceeds the outlier's own residual once
   `N > 9k + 3` (derivation in `openindy-core/test/pipeline/tst_pipeline.cpp`). Below that, the
   filter silently does nothing — not an error, just a threshold that never trips. This is a
   property of naive population-stdev sigma clipping generally, not a bug in this
   implementation; a future robust-statistics stage (median absolute deviation, iterative
   trimming) would not have this floor, but is out of scope per Stage 7b's "one real stage" limit.
10. **`BestFitPlane`'s normal-*sign* rectification degenerates for three collinear "used"
   points.** It picks the sign (not magnitude — that comes from the PCA fit over every used
   point) from the cross product of `usedPositions[0]`, `[1]`, `[2]` — i.e. whichever three
   elements happen to be first in `inputElements[0]`'s order, which is *acquisition* order, not
   anything geometric. A dataset built by iterating a grid one axis at a time (e.g. all `x=0`
   points before any other `x`) hands it three collinear points as that first triple. Harmless
   for a hand-tagged or randomly-ordered dataset, a real landmine for programmatically generated
   or synthetic test data — found while writing `tst_pipelinefilter.cpp` (Stage 7b), not a
   regression.
11. **`Feature::selfFeature` is not the `FeatureWrapper` that ends up in the job.** Every
   `Feature` constructs its own `FeatureWrapper` in its constructor
   (`Feature::selfFeature`), self-consistently wrapping itself. Separately,
   `OiJob::createFeatureWrapper` (and every test that follows its pattern) does
   `new FeatureWrapper(); wrapper->setPoint(new Point())` — a **second**, different
   `FeatureWrapper` object, which is the one actually passed to `addFeature`, stored in
   `FeatureContainer`, and threaded through the rest of the app. `Feature::addUsedFor`/
   `addPreviouslyNeeded`'s symmetric propagation uses `this->selfFeature` for the reverse
   edge, so `owner->getUsedFor()` ends up holding the *auto-created* wrapper, not whatever
   external wrapper a caller registered the same feature under — `.contains(externalWrapper)`
   on that list silently returns false even though the dependency edge is real and correct.
   `usedForMap`/`previouslyNeededMap` (keyed by feature id) are unaffected and are the
   reliable way to check membership; list membership must be checked by
   `entry->getFeature()->getId()`, not `QPointer<FeatureWrapper>` identity. Found while
   writing `tst_ownedsubgeometry.cpp` (Stage 7h) — not a regression, pre-existing since
   whenever `FeatureWrapper` gained per-feature construction, and every other test/call
   site either doesn't check `getUsedFor()`/`getPreviouslyNeeded()` by object identity or
   happens to use `getFeatureWrapper()` (== `selfFeature`) consistently on both sides.
12. **A hand-written `operator=`/copy constructor that copies only "my own new fields" silently
   drops base-class state once it starts being used for more than in-place mutation.**
   `Plane::operator=` (and every sibling `Geometry` subtype) only ever copied its own added
   members (`xyz`/`ijk`/`d` for `Plane`) — never `Geometry::statistic`, `isCommon`,
   `simulationData`, `activeMeasurementConfig`. Harmless for over a decade because the old
   `exec(Type&)` family mutated the live target object directly and `operator=` was never in
   the loop at all. Stage 7d's `Producer<T>`/`Transform<T>` build a *fresh local* `T` and
   write it back via `*target = result.value()`, which goes through exactly the `operator=`
   that had this gap — `getStatistic().getStdev()` silently read back `0` on every migrated
   fit function until fixed. **`TrafoParam` had the identical bug one level deeper**: its
   *copy constructor* (not just `operator=`) never copied `statistic` either, so the value
   was lost during `return trafoParam;`'s implicit conversion into
   `std::expected<TrafoParam, QString>` — before the dispatch boundary's fix could even see
   it. Fixed two ways, deliberately different: generically at the dispatch boundary
   (`execAs<T>()`, `function.cpp` — a concept-guarded `propagateStatisticIfPresent()` that
   covers any current or future `T` with `getStatistic()`/`setStatistic()`, without editing
   every affected `operator=`) for the `Geometry` case, and directly in `TrafoParam`'s own
   copy constructor and `operator=` for the case a boundary-level fix could not reach. Found
   via `tst_function`'s residual/stdev assertions and `tst_calibratedconstellation` while
   migrating Stage 7d, not pre-existing regressions — the bug was always there, but nothing
   exercised the `operator=`/copy-constructor path before this stage existed.
13. **`Feature::addFunction()` returns `void`, so a rejected attach (Stage 7d's attach-time
   enforcement) is silent to the caller.** A function that implements neither `Producer<T>`
   nor `Transform<T>` for a feature's type — or a second `Producer` once the slot is full —
   emits a message and is simply not attached; nothing communicates "did not attach" back to
   `OiJob`/`ProjectExchanger`/the UI short of re-checking `getFunctions()`'s size before and
   after. Not observed to matter in practice (every real plugin function now implements the
   right mixin; only ad-hoc test doubles ever triggered it, and every one found during this
   stage was fixed to implement a mixin rather than worked around), but a real gap if a
   future plugin ships a function that doesn't declare the mixin its `applicableFor` implies.
14. **A plan document's claim that work "folds into" a later stage is not evidence that stage's
   agent actually delivered it.** The Stage 7f section of `plan-domain-model-rework.md` stated,
   verbatim, "Task #52 (`Group::pendingMemberIds` on load) folds into this." It did not: 7f
   shipped tag resolution for points, and `Group::pendingMemberIds` resolution was never
   written by any stage before 7e actually did it (Stage 7e, above) — `ProjectExchanger` did
   not even write a `Group` to the project file, let alone read one back and resolve its
   members. The gap sat unnoticed through however many stages read that line and took it at
   face value. The only way to verify a "folds into"/"covered by" claim is to check the code
   the claim is about, not the sentence that asserts it — the same discipline this document
   already applies to code claims, extended to claims about *other documents*.
15. **A blanket-exclusion helper silently stops covering a new case the moment that case is
   added, and nothing forces anyone back to update it.** `getIsGeometry(FeatureTypes)` was
   written as "false for these three named types, true for everything else" before `Group`/
   `PointGroup` existed as feature types. When Stage 7a-ii added them, nobody had a reason to
   revisit `getIsGeometry` — it is defined once, far from either call site or enum
   declaration, and an enum growing a new value gives no compiler signal that a exclusion-list
   function elsewhere needs the same value added (the inverse of trap #3's exhaustive-switch
   problem: this shape has no `default:` to silently swallow a case, it has no case-list to be
   exhaustive over at all). Every caller of `getIsGeometry` guards a `feature->getGeometry()`
   access, which is null for a `Group` — so `OiJob::checkAndSetUpNewFeature`,
   `OiJob::connectFeature`, and several `openindy` UI models were all one `addFeature(group)`
   call away from a null-pointer crash for three stages running, undiscovered only because
   nothing had ever added a `Group` to a job through the real API before Stage 7e's round-trip
   test did. An inclusion list (name every type a function's positive case applies to) fails
   safe when a new type is added — the new type simply isn't handled, usually loudly. An
   exclusion list fails unsafe — the new type is silently swept into the "handled" branch
   until something crashes.
