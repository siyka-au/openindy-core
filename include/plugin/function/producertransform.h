#ifndef PRODUCERTRANSFORM_H
#define PRODUCERTRANSFORM_H

#include <expected>
#include <string>

namespace oi {

/*!
 * Stage 7d / D21 / domain-model.md S7 - "Producer and transform are
 * structurally distinct".
 *
 * A feature's function list used to conflate two jobs under one
 * convention: whatever sat at position 0 defined the feature from
 * external inputs (a fit, a construct function, a nominal producer);
 * everything after refined that same geometry in place. Nothing declared
 * this - code simply assumed index 0.
 *
 * These two mixins give each job its own, differently-typed, compiler-
 * checked signature. They are plain C++23 templates, not part of the Function
 * class hierarchy: Function is a non-template plugin interface, and the
 * generic parameter cannot go on it (the same reason Geometry<G> was not
 * introduced, domain-model.md S4).
 *
 * Resolved by composition, not by giving up genericity: a concrete plugin
 * class multiply-inherits Function (the moc-visible, plugin-discoverable
 * base) and the mixin for its specific geometry type, e.g.
 * `class BestFitPlane : public FitFunction, public Producer<Plane>`. moc
 * only ever processes Function/BestFitPlane - never Producer<T>/
 * Transform<T> themselves.
 */

/*!
 * \brief A feature's producer: defines its value from external inputs (a
 * fit's points, a construct function's other-feature inputs, a nominal
 * producer's scalar parameters).
 *
 * exec() takes no argument on purpose - a producer has no prior T to
 * receive. Its real inputs (tag-resolved NeededElements/InputElements, or
 * scalar parameters for a nominal producer) already live as ordinary
 * Function member state, populated by the caller (OiJob/ProjectExchanger)
 * before exec() runs, exactly how Function has always worked. This is not
 * a loss of information relative to Transform<T>::exec(const T&); it is
 * accurate about where the information already is.
 */
template <typename T>
class Producer {
 public:
  virtual ~Producer() = default;
  virtual std::expected<T, std::string> exec() = 0;
};

/*!
 * \brief A feature's transform: refines an already-produced value in
 * place, in declared order. Unlike Producer<T>, there is a genuine prior
 * value to consume - whatever the producer already built (or, for a
 * station-owned Point, whatever measurement already wrote).
 */
template <typename T>
class Transform {
 public:
  virtual ~Transform() = default;
  virtual std::expected<T, std::string> exec(const T& input) = 0;
};

}  // namespace oi

#endif  // PRODUCERTRANSFORM_H
