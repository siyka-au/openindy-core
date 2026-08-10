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

### Deliberately out of scope: automated segmentation

Auto-sectioning a cloud or a V-STARS batch and aligning it to what you are measuring is
months-to-years of research. Until then, using V-STARS or a scanner means accepting
responsibility for hand-allocating data — which retroactive tagging makes a first-class
operation rather than a workaround.

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

---

## 7. Plugin ABI

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

## 8. Other standing decisions

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

## 9. Traps this codebase has already sprung

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
