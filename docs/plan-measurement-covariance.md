# Plan: measurement context, offsets, directions, covariance and weighted fits

## Context
The user is vetting `openindy-core` (via the standalone Python library `openindy-py`) against Spatial Analyzer (SA) on real
survey data (Leica TS15 + 1.5" SMR). First results: plane and 3D circle agree with SA to a few micrometres (inside the 0.01 mm
rounding of the pasted data) once SA's probe offset is 0. The gaps found are structural, not numerical:
- fits have no offset parameter, and the sign of a fitted normal depends on point order, which breaks automated measuring
  (probe/target definitions themselves are an *instrument* concern, not a core one: see the decisions below);
- no per-measurement covariance (computed, then thrown away), fits are unweighted, no parameter covariance (`Statistic::Qxx`
  is set by 2 fits, never persisted), nothing propagates through derived features (e.g. a line from two planes);
- measurement configuration (fast/standard/precise...) is defined by the core and does not fit all instruments (arms);
- the core cannot recompute a dependency chain headlessly (the Qt app's `FeatureUpdater` did it), so "live" geometry only
  works in the app.
Goal: results equal to or better than SA, with explainable uncertainties, usable from Python and the app.

## Decisions made by the user (design rules)
- **Points are always target centres.** No point/observation carries an offset (a lone point has no direction).
- **Offsets are parameters of fits that supply a direction.** Plane: planar. Sphere, cylinder, circle, cone, slot: radial.
  Line: none. Per-fit signed value: outside = +19.05, bore (flip) = -19.05, ignore = 0. To the core, an offset is just a plain
  signed number on the fit.
- **Probe/target definitions are an instrument concern, not a core one.** A laser tracker or total station has SMRs, nests and
  prisms (SMR 1.5": planar 19.05, radial 19.05, centring ~0.005 mm; a nest is another entry with planar 25, radial kept; survey
  prisms: planar only; reflectorless: none); an arm has ball probes; a lidar scanner, structured-light scanner or photogrammetry
  system has no probe at all. Each instrument manages its own probe definitions in its own data (its blob below), which gives
  maximal flexibility. **The core has no `Target` type.** The instrument plugin/UI turns a probe definition into the signed number
  passed to a fit, and the plugin folds the target's centring uncertainty into the measurement covariance it reports.
- **Planar offset on other fits (edge found with a nest) needs an explicit direction** (SA "projection"): any vector.
- **A direction is just a vector from any source**, bindable live to other features: literal vector, any geometry's direction
  (line, plane normal, cylinder/cone axis, circle normal), vector between two points, an instrument frame axis (gravity is just
  the instrument z axis). No hard-coded source list, no assumed gravity, and no validation beyond non-zero.
- **Result-vector orientation rule** (plane normal, line direction, cylinder axis, circle normal): two independent settings.
  `orientation` = "first three points" (today's order-dependent rule, kept as the default) | "reference direction";
  `sense` = SAME | OPPOSITE, applied to **either** source (so the user can flip a first-three-points result too, and a
  reference-oriented result). The reference direction makes automated measuring independent of point order.
- **Measurement configuration lives with the instrument**, defined by its plugin. The core defines only an opaque, named
  blob: `{name, size in bytes, byte data}` (the name identifies the instrument's own schema/version; the instrument may store
  anything, including very complex configuration and context such as prism constant, temperature, humidity, serial number,
  time of day). The core never interprets it, plus the cross-instrument parts it does need: covariance and target. No
  interoperability between instruments is promised. Blobs are held once and referenced (shared in memory, de-duplicated in the
  project file), not copied per reading.
- **Instrument as a shared object (yes):** a first-class `Instrument` data object (name, type, serial, its config/systematic
  blobs) referenced by every `Station` (setup) that uses it, so data is reused and not copy-pasted. It is data, not the driver:
  the `Sensor` driver object stays per station/connection.
- **Covariance**: per measurement (3x3, from instrument error model + target centring), per station pose (6x6 on the
  station->working-frame `TrafoParam`), per fitted feature over a minimal parameterisation (point 3, plane 3, sphere 4, line 4,
  cylinder 5, circle 6, trafo 6/7, scalar 1), propagated first-order (J S J^T) through derived features; correlation between
  features (shared station pose) assembled on demand as J_i S_pose J_j^T, independence assumed until then.
- Offset sign convention to confirm in review: s = distance from the surface to the target centre along the outward normal
  (plane: reference-oriented normal; radial: away from the axis); surface = centre-geometry - s * outward, so a circle's surface
  radius is fitted radius - s (outside +, bore -, ignore 0). A constant offset along a fit's own normal/radial is identical to a
  post-fit shift (same residuals), so it can reuse the existing transforms `Shift`/`ChangeRadius`.

## Further decisions from the review (user-confirmed)
- **Warnings are first-class and retrievable.** A fit that falls back or degrades (near-perpendicular reference direction ->
  deterministic first-three-points fallback; ill-conditioned or near-singular geometry; too few degrees of freedom; non-PSD input
  covariance) records a structured warning on the feature (severity, code, message, source feature) and the job aggregates them
  (not saved in the project; recomputed on recalculation). Python: `job.warnings()` returns them (a script lists them before it
  ends); the UI shows a hazard sign next to a fit with warnings, and the CLI can print them. Distinct from errors (the fit failed).
  Near-perpendicular reference: threshold on |cos| (e.g. configurable, default documented), deterministic fallback, warning.
- **Units and definitions agreed:** SI internally (metres, radians; covariance in m^2), conversion at the edges; report RMS
  (sqrt(sum d^2 / n)) and sigma (sqrt(sum d^2 / (n - p))) separately; form errors (flatness, circularity, cylindricity) as max - min of
  residuals to match SA; rename the ambiguous `stdev`.
- **Per-point offsets (yes):** `offset` is a scalar or a per-point list of plain numbers, entering the residual per point (mixed
  probes/nests in one point set). The core still has no target type.
- **Directions are first-class features (yes):** a derived Vector/direction feature (literal, a geometry's direction, two points,
  cross product, instrument frame axis, mean of directions) with its own 2-dof covariance; fits take one `referenceDirection`.
- **Geometries expose derived outputs ("ports"), e.g. cardinal points** (cylinder: start, end, mid, axis; plane: origin, normal;
  line: start, end, mid, direction; cone: apex, axis; circle: centre, normal; sphere: centre). Each is an owned derived feature
  (point or vector) that stays live. The core already has the mechanism: function-owned sub-geometry (`OiJob::addOwnedSubGeometry`,
  `Geometry::getOwnerFeature`, and the existing constructs `PointFromPlaneOrigin` and `LineFromPlaneNormal`), to be generalised with
  named ports. Caveat to design: a line's or cylinder's start/end are not defined by the infinite geometry; they come from an extent
  rule (extreme projections of the fit's input points onto the axis) and are non-smooth functions of the data, so their
  uncertainty needs care (see the study guide).
- **Missing plugins:** a decoder policy (`allow_missing_plugins`) opens the project with placeholders, keeps blobs and stored
  points/fits readable, and refuses to run the missing plugin's functions or sensors.
- **Blobs are protobuf messages written by the plugin.** A readable text summary is **not required** (SA's text slab is only its
  own convention). The blob name carries the protobuf message type and version (for example `leica.ts15.MeasurementContext/v1`);
  a plugin may optionally implement `describe(blob) -> text` for diagnostics when loaded, and protobuf wire data can always be
  inspected generically (`protoc --decode_raw`). The core never stores a text copy.
- **Point sets (scale):** a `PointCloud` object exists in the core already (`PointCollection`, `PointGroup`, `PointCloud` with `Point_PC`
  floats; `PointGroup` is documented as the bulk container for many reflectorless shots). Covariance for point sets is designed
  behind a small "uncertainty model" interface giving the covariance of point i: none, shared, per point, or grouped (a cloud split
  into segments that each share one covariance, which is simply several clouds); the choice is deferred. SA, for reference: points
  are identifiers within a Point Group (not objects); a Point Cloud is a scan result; a "point list" appears to be a reference-list
  data type (a list of point names), not a geometry object; point sets generally spaced by time or distance are treated as one body.
- **Instrument error models:** the core provides only generic building blocks: `propagate(f, x, S_x)` with a Jacobian callback or
  numerical central-difference Jacobian, and the polar -> cartesian covariance helper (the most common case, shared by trackers and
  total stations). Arm kinematics (joint encoders, forward-kinematics Jacobian), scanner and photogrammetry models live in plugins,
  or in optional shared model libraries that plugins can use; the core does not model them. Arms are dominated by *systematic*,
  correlated calibration errors, so an arm model needs instrument-level leaf variables, not just per-point random terms.

## Findings that shape the work (verified in code)
- Inputs: `NeededElement`/`InputElement` (`core/include/plugin/function/function.h:71-155`); `ElementTypes::eDirectionElement`
  already exists (`core/include/util/types.h:72`, used by `BestFitCylinderAppxDirection`, `TranslateByValue`); a geometry-sourced
  direction already gets a live dependency edge (`OiJob::setUpDependencies`, `oi_job.cpp:3399-3580`, via
  `Feature::addPreviouslyNeeded/addUsedFor`). Literal vectors can use `ScalarInputParams` double/string params (persisted via
  `FunctionRecord`, `project.proto:675-694`). `Geometry::getDirection()` exists for line, plane, cylinder, cone, circle, ...
  (`geometry.h:124,133`); station axes via `CoordinateSystem::getZAxis` (`coordinate_system.h:62-64`) but need the app-level
  transform to the active system.
- Current sign rules: plane `p_best_fit_plane.cpp:117-134` (dummy point or cross of first three points, then
  `rectifyNormalToDirection`, `fit_function.h:184-190`, unclamped `acos` can return NaN); line `p_best_fit_line.cpp:126-131`;
  cylinder `fit_function.h:605-668`; circle-in-plane `fit_function.h:301-319` (dummy formula looks wrong, `:145-158`).
  There is no cone or slot fit at all (geometries exist: `geometry/cone.h:35`, `slotted_hole.h:40`).
- Recalc: `Feature::recalc` runs one feature only (`feature.cpp:627-673`); the cascade (`recursiveFeatureRecalculation`, ~45 lines,
  `app/controller/feature_updater.cpp:416-461`) plus observation transform (`app/controller/trafo_controller.cpp`, 341 lines;
  applies the transform *including translation to sigma vectors*, `:105-112`: a bug) are app-only and Qt-bound. A Qt-free core
  `Recalculator` is ~300-500 lines incl. tests (DFS over previouslyNeeded/usedFor, cycle guard, trafo-chain BFS).
- Weights: `Statistic::setP` never called; `Function::supportsWeights` (`function.h:434`) always false. Hooks: point fit
  `A^T A -> A^T P A`; sphere/circle/cylinder Gauss-Helmert `B B^T -> B Qll B^T` (`p_best_fit_circle.cpp:199`, `fit_function.h:1123-1180`);
  plane/line/circle-in-plane are closed-form SVD and need an iterative effective-variance (York) scheme for per-point covariance.
- Persistence: `GeometryStatistic` stores 5 scalars (`project.proto:359`), `Observation`/`Reading` only diagonal sigmas, `TrafoParam`
  no statistic; `kFormatVersion = 1` strict (no compatibility requirement, so changing fields is fine).
- `MeasurementConfig` blast radius: ~100 files / ~1,300 lines (core 51, app 50 incl. a 169-hit dialog, plugins 8, python 1) plus
  proto, JSON config files and `network.proto`. The target selection today is only the Leica string parameter `"reflector"`.
- Known defects to fix on the way (each with a test; see "Done so far" for what is fixed): `BestFitSphere` stdev dof (fixed);
  `BestFitCircle` never set stdev and wrote `"vx"` three times (fixed); `BestFitCircleInPlane` was Kasa (fixed: geometric default,
  Kasa kept as `BestFitCircleInPlaneAlgebraic`); `BestFitPoint` n=1 (fixed); `Reading::toPolar` inverted guard (fixed); mixed mm/m
  unit labels (open); `BestFitLine` stdev biased by a factor sqrt(2) (open, proposal below); `BestFitCylinder` with the default
  "first two points" approximation silently converges to a wrong cylinder in ~1 of 100 random 50-point sets (open).
  - Proposal for the 3D line divisor: `BestFitLine` divides the sum of the two small eigenvalues by `n - 2`, but a 3D line has two
    effective errors per point and four parameters, so the redundancy is `2n - 4`. The Monte Carlo check
    (`openindy-py/tests/vetting/test_reported_stdev_unbiased.py`) measures mean(stdev^2)/sigma^2 = 2.007 (strict xfail). Fix: divide
    by `2n - 4` (stdev falls by sqrt(2)); not applied yet because `openindy-py/examples/07` compares OpenIndy's line stdev with a 2D
    s0 whose convention must be revisited together with the change.

## Done so far (fixes made in the first two vetting batches, all tested)
- Direction flip helper (`fit::opposesDirection`, replaces the `acos`-based test; exactly antiparallel directions are flipped); line direction follows
  the point order.
- Display residual keys: `BestFitCircle` writes `vx`/`vy`/`vz`/`v` per point (they used to overwrite each other); in-plane circle
  writes `vr`, the signed radial distance.
- `BestFitPoint` with one point: stdev 0 and a warning instead of NaN.
- `BestFitSphere` stdev divides by `n - 4` (was `3n - 12`).
- In-plane circle dummy-point rule: the normal points from the centroid to the dummy point (`dummy - centroid`, translation invariant).
- `Reading::toPolar` guard fixed.
- Third batch: `BestFitCircleInPlane` is now the geometric (orthogonal-distance, Gauss-Newton) fit started from Kasa; the algebraic fit
  is `BestFitCircleInPlaneAlgebraic`; `fit_circle(..., method=)` in openindy-py. `BestFitCircle` (3D) sets stdev and s0 a posteriori
  from `sqrt(vv / (2n - 6))`; the in-plane circles use the same redundancy (radial plus out-of-plane corrections). A Monte Carlo
  unbiasedness check (400 trials per fit) validates plane `n-3`, sphere `n-4`, cylinder `n-5`, circles `2n-6`, and flags the line.

## Execution notes for a fresh session (this file is the saved plan; nothing below P0 has been started)
Status checklist: [ ] P1 directions + signed offsets  [ ] P1b direction features, ports, warnings  [ ] P2 recalculation  [ ] P3 covariance data + Instrument/blobs
[ ] P4 fit covariance + optional weighting  [ ] P5 propagation  [ ] P6 MeasurementConfig removal. User confirmed P1 first
and the offset sign convention (above).
Workspace: `/home/malleblas/workspaces/openindy` is not a git repo; it holds sibling repos `openindy-core` (Qt-free, std only),
`openindy-math`, `openindy` (Qt app), `openindy-default-plugin`, `openindy-leica-plugin`, `openindy-py` (nanobind Python library,
examples in `openindy-py/examples/`, 06 = SA comparison, 07 = flatland worked example). Read `/home/malleblas/workspaces/openindy/CLAUDE.md`
(build, CI, ABI policy, clang-tidy, dirty tracking, openindy-py) and the memory notes in
`/home/malleblas/.claude/projects/-home-malleblas-workspaces-openindy/memory/` first (SA vetting conventions, offsets, ABI policy).
Repo state after the third vetting batch (all local, **nothing pushed beyond what the user pushed**; do not push unless asked):
core `17bde732` (plus the commit that records these hashes), math `20e8e79`, app `beee96f6`, default plugin `38a1738`, leica plugin `2a52fbb`,
openindy-py `94abed3`; plugin ABI 20 (unchanged by this batch). Originally written at core `4d7ef0b5`, plugin `d85e670`, openindy-py `d3af676`.
Standing rules: commit often with tests green; **no attribution lines** in commit messages or PR descriptions; no new dependencies
without asking; use cheaper subagents for mechanical/large work and verify their output yourself; never run destructive git commands
on a dirty tree; changes to plugin-facing headers need `scripts/abi-check.sh` (libabigail built at `/home/malleblas/.cache/abg/inst`:
put `bin` on PATH and `lib` on LD_LIBRARY_PATH) and one ABI bump + `--update` per phase; quote shell globs (zsh).
Builds: Conan packages via `uvx conan install` (Release only), Ninja or the repo's configured generator, fresh build dirs under a
scratch directory (the old scratchpad under `/tmp` may be gone); `/tmp` is small (4.9 GB), `/home` has ~11 GB free.
The user's real build dir is `openindy/build` (Makefiles, Release, Conan toolchain at `build/conan`).

## Phases (each independently useful, tested, committed; ABI bumped at most once per phase when headers change)

### P1 - Directions and signed offsets in fits (highest value first; no covariance needed)
- A shared helper in `fit_function.h` resolves a **reference direction** from the function's inputs: a geometry element bound with a
  new role `"referenceDirection"` (live via existing edges), or literal `refDirX/Y/Z` params, or two point elements (roles
  `"directionFrom"/"directionTo"`), or a station/coordinate-system axis element plus an `axis` param; plus `sense` = SAME/OPPOSITE and
  `orientation` = "first points" (default, current behaviour) | "reference". Wire plane, line, cylinder, circle (3D and in-plane) to it;
  clamp the `acos` NaN; keep `dummyPoint` until the new inputs are proven, then remove it. `sense` (same/opposite) applies after
  either orientation source, including "first points".
- Signed `offset` (double param, default 0) on plane (planar, along the oriented normal), sphere, cylinder, circle (radial), applied as
  the post-fit shift reusing the geometry setters (and `Shift`/`ChangeRadius` logic); no cone/slot fits yet.
- openindy-py: `fit_plane(points, reference=vector|Plane|Line|Cylinder..., sense="same", offset=0.0)` etc.; Layer 1 takes a vector or
  a result object's direction as a literal; Job API takes feature handles (live once P2 lands).
- Tests: core Catch2 (reverse point order no longer flips with a reference; offsets per fit and sign convention; a geometry-bound
  reference updates after the source changes); Python tests; add the SA probe-offset-19.05 cases to the SA comparison example.
- Files: `core/include/plugin/function/fit_function.h`, `function.h` (roles/params), `default-plugin/functions/fit/p_best_fit_*.cpp`,
  `core/include/util/types.h`, `openindy-py/src/_core/fit.cpp`, `src/openindy_py/fits.py`.

### P1b - Direction features, geometry outputs ("ports") and warnings
- Warnings: `Feature` keeps a transient list of structured warnings (severity, code, message), `OiJob::warnings()` aggregates; a fit
  records the near-perpendicular-reference fallback, ill-conditioning, low dof; observers get an event so UIs can show a hazard sign;
  Python `job.warnings()`. Not persisted; recomputed on recalculation.
- Derived `Vector`/direction feature (construct functions: literal, geometry direction, two points, cross product, instrument frame
  axis, mean of directions) and a generalised "ports" mechanism on geometries (cylinder/line start, end, mid; plane origin; cone apex;
  circle centre ...) built on the existing function-owned sub-geometry (`OiJob::addOwnedSubGeometry`, `PointFromPlaneOrigin`,
  `LineFromPlaneNormal`). Defines the extent rule for finite ends.
- Per-point offset lists on fits (scalar or list).

### P2 - Headless recalculation in core
How it worked before: the app's `recalcFeatureSet` iterated *all* features (clear `isUpdated` on each, DFS each) and
`recalcFeature(f)` recomputed one feature's subtree, both triggered by observer events. The new design keeps that capability but
makes it a plain, pure-function API that hosts call when they choose, with an optional automatic mode:
- `Recalculator` (std-only): `recalculateAll(job)` (the on-demand project recalculation call, e.g. `job.recalculate()` in Python)
  and `recalculateAffected(feature)`; change events only **mark dependents stale** (cheap, no computation, reusing `isUpdated`),
  and a recalculation computes just the stale features. Hosts choose: the UI notices a change (dirty flag / stale marking) and calls
  recalculate (debounced for batch edits); Python calls it explicitly or sets `auto_recalc`; an optional auto mode is an observer
  that recalculates on change (coalesced). The dirty-for-saving flag stays driven by user-mutation events only (it must not be
  set by recomputed derived values such as `isSolved`; recalculation itself is not a user edit), but both consume the same
  change notifications.
- Algorithm: DFS previouslyNeeded -> `Feature::recalc` -> usedFor with a cycle guard and error capture; observation transformation
  into the active system (port of `TrafoController::getTransformationMatrix` as a BFS over `TrafoParam` edges, rotation-only for
  directions and covariances, *not* translating sigmas); a public/friend setter on `Observation` for xyz/ijk/sigma.
- The Python Job API gets `recalculate()` and live dependents; the app can delegate the generic cascade later (trafo-param and
  bundle code stays in the app for now). Replace the hand-called `recalc()` in `tst_measurement_intent`, `tst_pipeline`,
  `tst_measured_fit`.
- Files: new `core/include/recalculator.h`, `core/src/recalculator.cpp`, `core/include/observation.h`, `core/src/oi_job.cpp` (default
  observer), tests under `core/test/recalculator/`.

### P3 - Covariance data and target/instrument data (no algorithm changes yet)
- `Cov3` value type (symmetric 3x3, 6 doubles) on `Reading` and `Observation` (original and transformed); keep the full
  `Qxx = F S F^T` from `Reading::errorPropagationPolarToCartesian` (`reading.cpp:1012`) instead of discarding it; optional covariance on
  `Point` (Python can supply it); persist (`project.proto` Reading/Observation/Point, `codec/entities.cpp`).
- Station pose: add a `Statistic` (Qxx 6x6) to the persisted `TrafoParam` (station -> working frame); document the parameter order.
- No core `Target` type: probe/target definitions (SMR, nests, prisms, ball probes) live in the instrument's own blob. The Leica
  and pseudo-tracker plugins define their own (1.5" SMR 19.05/19.05, nests, prisms, reflectorless) and expose "offsets for this
  probe" through their own UI/plugin API; fits only ever receive the resulting signed `offset` number.
- `InstrumentBlob {name, bytes}` (core stores and persists it; plugins interpret it) held by a first-class `Instrument` object
  (type, serial, config and systematic-term blobs) that `Station`s reference, and optionally per reading for measurement context
  (shared pointer in memory; the project file stores each distinct blob once in a table and readings/instruments reference its id).
  A plugin-published schema/description for UIs is optional and lives in the plugin, not the core.
- Blob contract (so the core can host any current or future instrument's data, including lidar, structured light and
  photogrammetry): the core stores and round-trips blobs **verbatim even when the plugin is not installed**; `name` is the
  instrument's schema id and version, and the plugin owns migration; the core never interprets bytes; anything the core itself needs
  (the covariance, a fit's offset number) is **not** hidden in a blob: the plugin extracts it into core-known fields when it
  measures. The `Instrument` object stays minimal: id, plugin id, display name, blobs; it is not a driver.
- Leica/TS15 error model in the Leica plugin: angle 1" and distance 0.5 mm (configurable), target centring added; sensor plugins
  fill the covariance (including the target's centring uncertainty) and their instrument blob in `Sensor::measure`. Imported SA data supplies them from the file.
- `MeasurementConfig` migration is staged: new instrument-owned configuration is added *alongside* the old type first.

### P4 - Fit parameter covariance (always) and weighted fits (option, off by default)
Covariance and weighting are independent:
- **Parameter covariance is computed for every fit, weighted or not** (P4a), over the fit's documented minimal parameter vector:
  (a) with no input covariance, the classical residual-based Qxx x s0^2 (assumes equal independent errors; reflects the scatter of
  the data, not the instrument uncertainty); (b) when points/observations carry covariance, the covariance of the *unweighted
  estimator* by propagating the input covariances through the fit (sandwich J S J^T, as the flatland example does).
- **Weighting is an explicit option, default off** (P4b): `weighting="off"|"on"`. With it on the estimate itself changes: point
  `A^T P A`; sphere/circle/cylinder `B Qll B^T`; plane/line/circle-in-plane iterative effective-variance (York) from the SVD start
  (weights depend on the normal: `n^T S_i n`); the covariance is then the optimal `(J^T W J)^-1`. Set `supportsWeights`.
- Every fit sets `Statistic` consistently: Qxx, s0 a priori/posteriori, chi^2, correct degrees of freedom (fix sphere, circle);
  persist Qxx and P in `GeometryStatistic`; expose `covariance` and `weighting` in openindy-py.
- Vetting: SA case with deliberately unequal point uncertainties, weighting on/off, to learn whether SA's normal fit weights;
  turn the flatland example (`examples/07_flatland_intersection.py`) into a regression test (line parameter covariance).

### P5 - Propagation through derived features
- Shared helper `propagate(J, S_inputs)`; constructs (`IntersectPlanePlane`, `IntersectLinePlane`, line-line, point-from-points...)
  compute Qxx from their inputs' Qxx. Independence assumed (stated), then shared-station-pose correlation via per-observation +
  per-station covariance assembled on demand.
- Verify against Monte Carlo (as in the flatland example) and SA's uncertainty outputs.

### P6 - `MeasurementConfig` removal from the core (largest refactor, last)
- Move fast/standard/precise etc. into the instrument plugins' own config data; remove the core type, proto messages, JSON config
  files and the app's measurement-config dialog/model/manager (~100 files). Keep only the instrument-owned blobs.
- The first-class `Instrument` object is introduced in P3 (shared by stations, data reused rather than copied); P6 only moves the
  old fast/standard/precise configuration into the instruments' own blobs.

## Analysis of the decisions: issues and improvements (review before P3/P4; items marked [decide] change the design)

### Mathematical rigour
1. **[decide] Covariance of orientation-bearing features needs a defined manifold/tangent parameterisation.** A plane normal, line
   direction, cylinder axis and circle normal are unit vectors (2 degrees of freedom). "Minimal parameters" must be an explicit
   *local* parameterisation (e.g. a small rotation about the fitted direction, in a declared tangent basis, plus offsets), with the
   covariance defined there, and a known linear map when the sign is flipped (SAME/OPPOSITE) or the frame changes. Without it,
   covariances of two features, or the same feature before/after a flip, cannot be compared or propagated consistently. Document the
   basis and order per geometry type, and test invariance under flip and rigid transform.
2. **[decide] Independence is wrong in common cases, and per-feature covariance cannot fix it.** Features that share points
   (a plane and a circle from one point group), a station pose, or an instrument's systematic terms (an EDM scale error shared by every
   setup of that instrument) are correlated. The rigorous way is to track each estimated feature's Jacobian with respect to the
   *leaf* uncertain variables (observation errors, station poses, instrument terms): Cov(f1, f2) = J1 S_leaf J2^T. That gives correct
   correlations "for free" through any chain of derived features, at the cost of storing sparse Jacobians. Alternative: per-feature
   covariance now, leaf Jacobians in P5. At least add the instrument as a leaf variable in the design, not only the station pose.
3. **First-order propagation (J S J^T) is invalid near singularities** (nearly parallel planes intersecting, short-arc circle or
   cylinder fits, collinear points for a plane): the covariance blows up and the linearisation fails. Add validity checks (condition
   number, uncertainty vs scale, linearisation residual) and an option `propagation="linear"|"monte_carlo"`; SA itself uses Monte
   Carlo in USMN. Use Monte Carlo as the standing test oracle for every covariance output (generalise `examples/07` into a test utility).
4. **Residual-based covariance with few degrees of freedom is unreliable.** Three points on a line give 1 dof, and in the worked
   example s0 ranged 0.03 to 0.98 mm. Report both the a priori (from the instrument model, when available) and the a posteriori
   scaling, the chi^2 against its dof, and use the a priori value when dof is small. Offer an expanded uncertainty with a coverage factor
   from the effective degrees of freedom (GUM / Welch-Satterthwaite) instead of presenting 1-sigma only.
5. **One estimator family: Gauss-Helmert everywhere.** Sphere, circle and cylinder already are. Implement plane and line the same
   way (condition equations f(l, x) = 0 with a general Qll, unit-vector constraint) instead of closed-form SVD plus a special
   effective-variance (York) path. That gives the weighted option, the parameter covariance, `B Qll B^T`, and the sandwich propagation
   from one code path, and removes per-fit special cases. Keep the SVD only as the starting value.
6. **Numerical conditioning.** The plane/line fits take the SVD of A^T A (squaring the condition number: exact points reported stdev
   ~1e-10, not ~1e-16). Use the SVD or QR of the centred data matrix itself, always centre coordinates before forming normal equations,
   and fix the NaN-prone `acos` in `rectifyNormalToDirection` (use atan2 on the cross and dot).
7. **Offsets and uncertainty.** An offset has its own uncertainty (probe diameter, nest tolerance). A reference direction that comes
   from another feature has its own covariance, so an offset along it (the nest/edge projection case) adds s^2 S_dir and correlates
   with that feature: the "post-fit shift" is equivalent for the *residuals* and the estimate when the offset is constant along the fit's
   own normal/radial direction, but not for the covariance of an externally directed offset. Model the offset as a value with optional
   sigma, and propagate the direction's covariance (the same live dependency edge).
8. **The generic definition of an offset is the parallel (Minkowski) surface:** the surface offset by s along its own normal. It
   reduces to a shifted plane, a smaller/larger sphere, cylinder and circle radius, but for a cone it is **not** a radius change (the
   apex moves along the axis by s / sin(half-angle)). State the definition so cone, torus and slot are done correctly later.
9. **Near-degenerate references (decided: threshold, deterministic fallback, warning).** SAME/OPPOSITE against a reference direction that is almost perpendicular to the fitted vector is
   ill-defined, and with live recalculation tiny changes could flip the result chaotically. Define a threshold on |cos|, a
   deterministic fallback (first three points), and a warning. A reference may also carry uncertainty (a total station's compensator
   gives gravity to about 0.5 to 1 arcsecond), which matters when it defines the sign only marginally.
10. **Units and definitions.** Fix internal units (SI: metres and radians; covariance in m^2) and convert at the edges; the audit found
    mixed mm/m labels. Define precisely, and report separately, RMS = sqrt(sum d^2 / n), sigma = sqrt(sum d^2 / (n - p)), form errors
    (flatness, circularity, cylindricity as max - min of residuals) so they match SA's definitions; rename the ambiguous `stdev`.
11. **Exact and degenerate covariances.** Nominal/exact points have zero covariance (infinite weight); singular or non-PSD matrices from
    imports must be detected (PSD check, symmetric storage) and handled by a defined rule (floor at the instrument resolution or treat as
    fixed). Store covariances as symmetric packed triangles so they cannot go asymmetric.

### Functionality and flexibility
12. **Per-point offsets (decided: yes).** The post-fit shift assumes one offset for the whole fit. Mixed probes in one point set (some points
    with the SMR in a 25 mm nest, others with 19.05) are real. Allow `offset` to be a scalar or a per-point list (a plain number list; the
    instrument/UI still supplies the numbers; the core still has no target type), entering the residual per point.
13. **Directions as first-class features (decided: yes).** Instead of enumerating input roles for every direction source, add a derived
    `Vector`/direction feature (from literal, a geometry's direction, two points, cross product of two directions, an instrument frame
    axis, a mean of directions) computed by ordinary construct functions with its own 2-dof covariance. Any fit then takes one
    "referenceDirection" input of that type. Matches "a direction is just a vector" and removes the combinatorial input design; P1 can
    start with literal + geometry direction and add the feature later without changing the fit parameters.
14. **Missing plugins versus verbatim blobs (decided: allow_missing_plugins policy).** The project loader currently fails when a plugin is missing, while the blob
    contract says data survives verbatim without its plugin. Reconcile with a decoder policy (for example `allow_missing_plugins`: open
    with placeholders, keep blobs and stored points/fits readable, refuse to run the missing plugin's functions or sensors) so a
    colleague without the instrument plugin can still inspect and re-fit the data.
15. **Blob practicalities (readable text summary is optional, see decisions).** Treat blob contents as untrusted input to plugins; set a size limit; use the name to carry schema id and
    version, with plugin-owned migration; allow an optional human-readable text summary beside the opaque bytes (SA's slab is
    readable text) for diagnostics; de-duplicate on save by exact byte comparison (not a hash); mutations of an instrument or blob raise
    change events so the dirty flag and stale marking work.
16. **Scale (decided: PointCloud with an uncertainty-model interface, details later).** Lidar, structured-light and photogrammetry data are millions of points. A `Feature` per point (wrapper, intents,
    observation) will not scale (job insertion is already O(n) per point; 40k points take 2.5 s). Fits and covariance should accept
    contiguous arrays or a bulk point-cloud geometry with optional per-point or shared covariance. Not for now, but do not design APIs that
    force per-point objects.
17. **Instrument error-model helpers (decided: generic propagate + polar helper in core; others in plugins/libraries).** A std-only helper that turns polar readings and angle/distance sigmas into a 3x3
    covariance (the Jacobian code behind `Reading::errorPropagationPolarToCartesian`, made public and tested) lets Python users who
    supply raw angles and plugins share one verified implementation; the target's centring variance is added by the plugin.
18. **Separate the options clearly:** `weighting` (changes the estimate), `covariance` source (residual-based, propagated from inputs,
    or Monte Carlo), `orientation`/`sense`, `offset`, and `reference`; keep each option orthogonal and documented. Each phase keeps
    defaults identical to today so existing results do not change.

### Verification strategy additions (rigour)
- Property tests for every fit and propagation: invariance under rigid transform (covariance rotates), point permutation, SAME/OPPOSITE
  flip (known map), scaling (units), and added duplicate points; symmetric and PSD covariance checks; finite-difference checks of any
  analytic Jacobian; Monte Carlo agreement within sampling error; SA agreement where SA reports uncertainty
  ("Fit Geometries Using Uncertainty Fields") and the unequal-uncertainty weighting test.

## Out of scope / later
Cone and slot fit functions; bundle adjustment weighting (default plugin has none); manylinux wheels; CI for openindy-py; the
`Qt`-free replacement of app code beyond the Recalculator.

## Verification (every phase)
- Core/plugin/app `ctest`, `scripts/abi-check.sh` (bump `kPluginAbiVersion` + `--update` if headers change), clang-tidy and format gates,
  ASAN leak gate, Qt-hidden core build.
- openindy-py: `uv run pytest`, the examples (`examples/06`, `07`) still pass; new vetting cases added per phase (SA offset on/off,
  unequal-uncertainty case, flatland regression).
- P1 acceptance: a plane/line/cylinder fit with a reference direction gives the same oriented result for points in any order;
  offsets match the hand calculation (plane +/-19.05 along n, circle radius 310.447 -/+ 19.05) and SA with its default offset.
- P2 acceptance: edit a source line and the dependent plane re-orients without any manual recalc call.
- P4 acceptance: weighted vs unweighted lines of the flatland example reproduce the documented numbers; parameter covariance agrees
  with a Monte Carlo run.

---

# Appendix A: Mathematics study guide (for self-study)

Written so the ideas behind the "Mathematical rigour" items can be learned in depth. **The book and paper references are from my
memory: verify titles, authors and years before relying on them.** Each section says what the idea is in plain words, why this project
needs it, what to search for, and where it appears in the plan. Formulas are plain text.

## A0. Suggested learning order
1. Variance, covariance, covariance matrices, error ellipses/ellipsoids (A1).
2. Least squares, weights, degrees of freedom, residuals and the covariance of the estimate (A2).
3. Jacobians and first-order (linear) propagation of uncertainty (A3).
4. Errors-in-variables fits and the Gauss-Helmert model (A4).
5. Parameterising geometry: unit vectors, tangent spaces (A5).
6. Correlation, shared errors and "leaf variables" (A6).
7. When linearisation fails, and Monte Carlo (A7).
8. Small-sample statistics: dof, chi-square, coverage factors (A8).
9. Numerical linear algebra: conditioning, SVD vs normal equations (A9).
10. Offset (parallel) surfaces (A10).
11. Instrument error models (A11).

## A1. Variance, covariance and covariance matrices
- A **random variable** here is a measurement error. **Variance** s^2 measures how spread out it is; the **standard deviation** s
  is its square root, in the units of the quantity.
- **Covariance** Cov(x, y) = E[(x - mx)(y - my)] says whether two errors move together; the **correlation** r = Cov / (sx sy) is in
  [-1, 1].
- The **covariance matrix** of a vector (x, y, z) is the symmetric table of all variances (diagonal) and covariances (off
  diagonal). It must be **positive semi-definite** (every variance along any direction is >= 0).
- Its **eigenvectors** are the principal axes of the **error ellipsoid** and the eigenvalues are the variances along them. The
  uncertainty of a point *in a given direction n* is n^T S n (used for weights and for "across the line" residuals).
- Search: "covariance matrix", "error ellipse", "eigen decomposition of covariance", "Mahalanobis distance".
- Read: Ghilani, *Adjustment Computations: Spatial Data Analysis* (early chapters, surveying flavour); any statistics text.
- In the plan: every covariance item; the SA record's covariance matrix; the flatland example (`openindy-py/examples/07`).

## A2. Least squares, weights and the covariance of the estimate
- **Least squares** picks parameters x to minimise the sum of squared **residuals** v. **Weighted** least squares minimises
  v^T W v; the best (minimum-variance) choice is W = S_l^-1, the inverse of the observation covariance. A point with a larger
  uncertainty in a direction gets a smaller weight in that direction.
- For the linear model l = A x + v: x_hat = (A^T W A)^-1 A^T W l. When W = S_l^-1 the covariance of the estimate is
  S_x = (A^T W A)^-1 (the **cofactor matrix** Qxx, when scaled by s0^2).
- **Unweighted fit, honest uncertainty:** if you use W = I but the observations really have covariance S_l, the covariance is the
  **sandwich** S_x = (A^T A)^-1 A^T S_l A (A^T A)^-1. This is what the worked flatland example computed by propagating through the
  fit. It is larger than the optimal (weighted) covariance, which is the point of weighting.
- **s0** (the **variance factor**): s0^2 = v^T W v / (n - u) with n observations and u unknowns; n - u is the **degrees of
  freedom**. A priori s0 is what you expect from the instrument model, a posteriori s0 is what the residuals show.
- Search: "Gauss-Markov theorem", "BLUE", "generalized least squares", "Aitken estimator", "sandwich estimator", "cofactor matrix".
- Read: Mikhail & Ackermann, *Observations and Least Squares*; Koch, *Parameter Estimation and Hypothesis Testing in Linear Models*;
  Hartley & Zisserman, *Multiple View Geometry*, appendices on estimation.
- In the plan: P4 (always compute parameter covariance, weighting optional), item 4 (few dof).

## A3. Jacobians and first-order propagation (the law of propagation of uncertainty)
- If y = f(x) and x has covariance S_x, then to first order S_y = J S_x J^T where J is the **Jacobian** (matrix of partial
  derivatives dy_i / dx_j). It is the multi-variable version of "sigma_y = |f'(x)| sigma_x".
- Why it is right for small errors: it is a **first-order Taylor expansion** of f around the measured values.
- Examples here: polar -> cartesian (J of (d cos t, d sin t) with respect to (d, t)); the line fit as a function of its points; the
  intersection of two lines as a function of their parameters; a plane-plane intersection line.
- Practical: Jacobians can be analytic (exact, fast) or numerical (central differences, easy, checks the analytic ones).
- Search: "law of propagation of uncertainty", "error propagation Jacobian", "delta method".
- Read: **JCGM 100:2008 (GUM)** and **JCGM 102:2011** (multivariate case); Förstner & Wrobel, *Photogrammetric Computer Vision*.
- In the plan: P3 (reading covariance), P4 (sandwich), P5 (derived features), the `propagate` helper.

## A4. Errors-in-variables fits and the Gauss-Helmert model
- Ordinary least squares assumes only the "y" values are noisy. For geometry fits *all* coordinates are noisy and the residual
  that matters is the **orthogonal (perpendicular) distance** to the shape: this is **total least squares**, or **orthogonal distance
  regression**. For a line (n . p = rho) the residual is r = (n . p - rho) and its variance is n^T S_i n, so the weighted objective is
  chi^2 = sum r_i^2 / (n^T S_i n): the weights depend on the unknown n, so the fit is iterative (the **effective-variance** method).
  **York's** method is the exact solution for straight lines with correlated x and y errors.
- The general framework is the **Gauss-Helmert model**: the observations l and unknowns x satisfy condition equations
  f(l + v, x) = 0; minimise v^T S_l^-1 v. With A = df/dx and B = df/dl: Pww = (B S_l B^T)^-1, N = A^T Pww A, S_x = N^-1.
  Sphere, circle and cylinder fits in this code are already Gauss-Helmert; planes and lines are solved in closed form (SVD), which
  is why weights and covariance need extra work there. Doing all of them in Gauss-Helmert gives one code path.
- Circle fitting has well-known pitfalls: the fast **algebraic** fits (Kasa, Pratt, Taubin) are biased on short arcs; the
  **geometric** fit (minimising orthogonal distances) is unbiased; this is the Kasa-versus-geometric difference seen in the SA comparison.
- Search: "total least squares", "orthogonal distance regression", "Gauss-Helmert model", "effective variance method", "York fit",
  "circle fitting Kasa Pratt Taubin geometric".
- Read: Van Huffel & Vandewalle, *The Total Least Squares Problem*; York et al. (2004), Am. J. Phys. 72, 367 ("Unified equations
  for the slope, intercept, and standard errors of the best straight line"); Orear (1982); Chernov, *Circular and Linear
  Regression*; Förstner & Wrobel (Gauss-Helmert); Koch.
- In the plan: items 5 and P4 (weighted fits).

## A5. Parameterising geometry: unit vectors, tangent spaces and "minimal parameters"
- A plane normal is a unit vector: three numbers with one constraint, so only **2 degrees of freedom**. A covariance of its three
  components is singular (rank 2): the variance along n itself is zero by definition. A **minimal parameterisation** uses exactly the
  independent degrees of freedom: plane 3, sphere 4, line 4, cylinder 5, circle 6, rigid transform 6 (+1 scale).
- The standard rigorous trick (used in robotics and bundle adjustment) is to describe uncertainty in a **tangent space**: represent a
  small error as a rotation delta (perpendicular to n) applied to the fitted n. The covariance lives on those 2 delta numbers in a
  declared basis. A **manifold** is the curved space of all unit vectors; the tangent space is its flat local approximation.
- Why it matters: (a) covariances of two features can only be compared/propagated if they use the same declared parameters and
  basis; (b) when the normal is flipped (n -> -n) the basis and the offset change sign by a known linear map, so the covariance must
  be transformed, not reused; (c) homogeneous or Plucker coordinates are alternatives with their own constraints.
- Also called the **gauge** problem: parameters that cannot be determined (a gauge freedom) must be fixed explicitly.
- Search: "minimal parameterisation", "tangent space error state", "Lie group SO(3) uncertainty", "uncertain geometric entities",
  "homogeneous coordinates covariance", "Plucker coordinates line".
- Read: Sola, Deray & Atchuthan (2018) "A micro Lie theory for state estimation in robotics"; Triggs et al. (2000) "Bundle
  Adjustment: a Modern Synthesis"; Foerstner & Wrobel (uncertain projective geometry); Heuel, *Uncertain Projective Geometry*.
- In the plan: item 1, P3/P4 (every fit's Qxx order and basis must be written down), the SAME/OPPOSITE flip.

## A6. Correlation, shared errors and "leaf variables"
- Two derived features are correlated when they depend on the same uncertain inputs: two fits using one point group, or two points
  measured from the same station (shared pose error), or from the same instrument (shared systematic error such as an EDM scale error).
  Treating them as independent gives wrong uncertainties for anything computed from both (e.g. the line where two planes meet).
- Rigorous approach: keep each feature's Jacobian with respect to the **leaf** random variables (individual measurement errors, station
  poses, instrument terms). Then Cov(f1, f2) = J1 S_leaf J2^T for any pair, however long the chain of derived features. This is the
  idea behind network adjustments (SA's USMN) and Monte Carlo methods that carry the same random draws through every calculation.
- Cheaper alternative: per-feature covariances assuming independence, with the assumption stated. Intermediate: per-observation
  covariance plus one pose covariance per station, joint covariance assembled on demand.
- Search: "correlated inputs GUM", "covariance between derived quantities", "network adjustment joint covariance",
  "unified spatial metrology network", "uncertainty fields".
- Read: GUM (correlated inputs); Calkins (2002), PhD thesis on coordinate uncertainty fields in coupled spatial measurement systems
  (the basis of USMN as I recall); the Muelaner et al. paper found in the web search ("Estimation of Uncertainty in Three Dimensional
  Coordinate Measurement by Comparison with Calibrated Points"); SA's USMN page.
- In the plan: item 2, P3/P5, the Instrument object.

## A7. When linearisation fails, and Monte Carlo
- First-order propagation assumes f is nearly linear over the size of the uncertainty. It breaks down near singularities: two
  planes almost parallel (their intersection line swings wildly), a circle fit from a short arc, a cylinder from a small patch, a
  plane from nearly collinear points. The covariance then explodes or is simply wrong.
- Checks: the **condition number** of the system (large = fragile), the ratio of uncertainty to the geometric scale, comparing the
  linear result with a Monte Carlo run.
- **Monte Carlo** propagation (JCGM 101): draw many random sets of inputs from their distributions, push each through the full
  nonlinear calculation, and take the statistics of the outputs. It is slow but needs no Jacobians and handles any nonlinearity; use it
  as the test oracle for every covariance output. The **unscented transform** is a cheaper deterministic variant.
- Search: "GUM supplement 1 Monte Carlo", "unscented transform", "condition number", "ill-conditioned geometry".
- Read: JCGM 101:2008; Julier & Uhlmann (1997) on the unscented transform; Golub & Van Loan (condition numbers).
- In the plan: item 3, the validity checks and `propagation="monte_carlo"` option, all test strategy.

## A8. Few degrees of freedom: dof, chi-square, coverage factors
- With few redundant observations (a line from three points has 1 dof) the residual-based s0 is a very noisy estimate: in the worked
  example it ranged from 0.03 to 0.98 mm over repeated surveys. Prefer the a priori sigma from the instrument model when known.
- **Chi-square test**: under correct a priori sigmas, chi^2 = v^T S^-1 v follows a chi-square distribution with dof degrees of
  freedom; a value far outside its range says the model or sigmas are wrong, or there is an outlier.
- **Expanded uncertainty**: report U = k u with a **coverage factor** k (about 2 for ~95%); with small dof use the Student t value.
  The **effective degrees of freedom** of a combined uncertainty come from the **Welch-Satterthwaite** formula
  nu_eff = u_c^4 / sum(u_i^4 / nu_i).
- Search: "chi-square test adjustment", "global test least squares", "Student t coverage factor", "Welch-Satterthwaite".
- Read: GUM, especially Annex G; Koch (hypothesis testing).
- In the plan: item 4, `Statistic` reporting a priori/a posteriori/chi-square/dof.

## A9. Numerical linear algebra: conditioning and SVD
- The **condition number** k(A) is how much errors are magnified solving with A. The normal equations A^T A square it:
  k(A^T A) = k(A)^2, which is why the plane fit's roundoff-sized eigenvalue gave an exact-point stdev of ~1e-10 and not ~1e-16.
- Prefer the **SVD or QR of the data matrix itself** and always **centre** coordinates (subtract the centroid) before forming
  equations: coordinates of thousands of mm otherwise lose precision.
- Replace `acos(dot)` (undefined when rounding pushes |dot| past 1, returning NaN) with atan2(|cross|, dot).
- Search: "condition number least squares", "SVD versus normal equations", "QR factorisation least squares", "numerical stability".
- Read: Golub & Van Loan, *Matrix Computations*; Trefethen & Bau, *Numerical Linear Algebra*.
- In the plan: item 6, P4.

## A10. Offsets as parallel (offset) surfaces
- A probe of radius s touching a surface puts its centre on the **parallel surface** at distance s along the surface normal
  (the **Minkowski sum** of the surface with a ball). That single definition explains the "planar" and "radial" offsets: a plane
  moves along its normal; a sphere's, cylinder's and circle's radius grows or shrinks by s; but a **cone** offset is not a radius
  change (the apex moves along the axis by s / sin(half-angle)), and a torus changes its tube radius.
- The user's cylindrical-puck model (planar offset along the puck axis, radial perpendicular to it) decomposes the contact normal
  into axial and radial parts for flat and round contacts.
- Uncertainty: an offset has its own sigma; an offset along an *externally supplied* direction adds s^2 times the direction's
  covariance and correlates the result with that direction's source (the nest-and-reference-plane projection case).
- Search: "offset surface", "parallel surface", "Minkowski sum", "probe radius compensation CMM".
- Read: do Carmo, *Differential Geometry of Curves and Surfaces* (parallel surfaces); CMM probe compensation literature.
- In the plan: items 7, 8 and the offset definition.

## A11. Instrument error models
- A **polar** instrument (total station, laser tracker) measures horizontal angle, vertical angle and distance, each with its own
  sigma. The cartesian covariance is S = J diag(sd^2, sa^2, sz^2) J^T (A3): long along the line of sight (distance error), narrow
  across it (angle error times range). The worked flatland example shows the resulting needle-shaped error ellipse.
- Real instruments add **systematic**, correlated terms (axis misalignments, scale errors, compensator error, refraction,
  temperature), which is why trackers have multi-term models (the repository's `tracker_error_model` follows Hughes et al. 2010).
  A **measurement arm** is a chain of rotary joints: its covariance comes from the **forward-kinematics Jacobian** with the encoder
  and calibration errors, mostly systematic. Scanners and photogrammetry have their own models.
- Why the core only offers generic helpers (polar covariance, numerical Jacobian propagation): each instrument family is a model
  of its own and belongs to its plugin or a shared model library.
- Search: "total station error model", "laser tracker error model Hughes", "articulated arm CMM kinematic model uncertainty",
  "leica TS15 specifications".
- Read: Ghilani; Hughes et al. on laser tracker error determination; Estler et al., "Large-scale metrology: an update" (CIRP Annals,
  2002); the instrument manufacturers' specification documents.
- In the plan: P3, item 17.

## A12. Glossary
- **Residual** v: observed minus fitted. **Redundancy / dof**: n - u. **Weight** W: importance, usually S_l^-1.
- **Covariance** S, **cofactor** Qxx = S / s0^2. **Jacobian** J: matrix of derivatives. **Linearisation**: replacing f by its tangent.
- **Gauge freedom**: undetermined parameters that must be fixed. **Manifold / tangent space**: curved parameter space / its local
  flat approximation. **PSD**: positive semi-definite.
- **A priori / a posteriori**: before (from the model) / after (from the residuals) the fit.
