#ifndef OBSERVATION_H
#define OBSERVATION_H

#include <math.h>

#include <QList>
#include <QObject>
#include <QPointer>
#include <optional>

#include "geometry/point.h"
#include "oivec.h"
#include "statistic.h"

class TrafoController;

namespace oi {

using namespace math;

class Reading;
class Station;
class CoordinateSystem;
class Geometry;

/*!
 * \brief The Observation class
 *
 * D20 (domain-model.md S2): an Observation *is* a Point - single
 * inheritance, legal because Point's own chain (Geometry : Feature :
 * Element) never crossed Observation's old one (Element directly). This is
 * what lets a fit select Points and freely intermingle raw shots with
 * derived ones without ever asking which kind it holds (the stated
 * requirement behind D12): an Observation satisfies every Point-typed
 * input by ordinary polymorphism, no wrapping/adapter type needed.
 *
 * D19: an Observation is owned by its Station and always read-only - see
 * getOwnerDescription(). It has no producer function and (deliberately)
 * never populates its own inherited Geometry::observations list - "no
 * observations of its own" is what the design doc calls this, and it is
 * unreachable rather than merely unused, because nothing in this codebase
 * ever calls recalc()/exec() on a raw, station-owned Observation - only
 * the aggregate Point that OiJob::addMeasurementResults builds from one or
 * more Observations is job-managed and fed to functions.
 *
 * Observation keeps its own xyz/ijk (with sigma, and an "original" vs
 * "current display system" split) rather than reusing the inherited
 * Position/Direction Geometry carries, because Position has no sigma slot
 * at all (see position.h) - this is a real, not cosmetic, duplication.
 * setXYZ()/setIsSolved() keep the inherited Point-facing position and
 * Feature-facing isSolved in sync defensively, but nothing in the normal
 * data flow actually reads Point::getPosition()/Feature::getIsSolved() on
 * a raw Observation today.
 */
class OI_CORE_EXPORT Observation : public Point {
  friend class Reading;
  friend class CoordinateSystem;
  friend class ::TrafoController;
  Q_OBJECT

 public:
  explicit Observation(QObject* parent = nullptr);

  Observation(const Observation& copy, QObject* parent = nullptr);
  Observation(const OiVec& xyz, bool isValid, QObject* parent = nullptr);
  Observation(const OiVec& xyz, int id, bool isValid,
              QObject* parent = nullptr);

  Observation& operator=(const Observation& copy);

  ~Observation() override;

  // ##################################
  // get or set observations attributes
  // ##################################

  const OiVec& getXYZ() const;
  void setXYZ(const OiVec& xyz);

  const OiVec& getOriginalXYZ() const;

  const OiVec& getSigmaXYZ() const;
  void setSigmaXyz(const OiVec& sigmaXyz);

  const OiVec& getOriginalSigmaXyz() const;

  const OiVec& getIJK() const;
  void setIJK(const OiVec& ijk);

  const OiVec& getOriginalIJK() const;

  const OiVec& getSigmaIJK() const;
  void setSigmaIjk(const OiVec& sigmaIjk);

  const OiVec& getOriginalSigmaIjk() const;

  // ##################################################
  // get and set station, reading and target geometries
  // ##################################################

  const QPointer<Station>& getStation() const;
  void setStation(const QPointer<Station>& station);

  // D12: backed by the inherited Point::intents (MeasurementIntent) list
  // rather than a second, Observation-only membership list - see
  // point.h/measurementintent.h. Kept as QPointer<Geometry> convenience
  // accessors (resolved via job) so existing call sites did not all need
  // to move to raw ids; roleName is not exposed here, only feature
  // identity, since none of the pre-existing callers distinguished roles.
  QList<QPointer<Geometry> > getTargetGeometries() const;
  void addTargetGeometry(const QPointer<Geometry>& targetGeometry);
  void removeTargetGeometry(const QPointer<Geometry>& targetGeometry);
  void removeTargetGeometry(const int& geomId);
  const QPointer<Geometry>& getMeasuredTargetGeometry() const;
  void setMeasuredTargetGeometry(const QPointer<Geometry>& measuredTargetGeom);

  const QPointer<Reading>& getReading() const;

  // D19/D20: always station-owned, always read-only - see observation.cpp
  std::optional<QString> getOwnerDescription() const override;

  // keeps the inherited Point-facing position correct regardless of
  // which of Observation's own xyz write paths ran last - see
  // observation.cpp
  const Position& getPosition() const override;

  // ##########################
  // get and set state booleans
  // ##########################

  const bool& getIsValid() const;

  void setIsSolved(const bool& isSolved);
  const bool& getIsSolved() const;

  const bool& getHasDirection() const;

  const bool& getIsDummyPoint() const;

  // ###############
  // display methods
  // ###############

  QString getDisplayId() const;
  QString getDisplayStation() const;
  QString getDisplayTargetGeometries() const;

  // matches Geometry::getDisplayX/Y/Z's signature exactly (including the
  // showDiff param, unused here) so this is an ordinary override, not an
  // overload that hides the base one under -Woverloaded-virtual - and so
  // a 2-arg call (every existing call site) stays unambiguous rather than
  // resolving against two equally-viable candidates.
  QString getDisplayX(const UnitType& type, const int& digits,
                      const bool& showDiff = false) const override;
  QString getDisplayY(const UnitType& type, const int& digits,
                      const bool& showDiff = false) const override;
  QString getDisplayZ(const UnitType& type, const int& digits,
                      const bool& showDiff = false) const override;
  QString getDisplaySigmaX(const UnitType& type, const int& digits) const;
  QString getDisplaySigmaY(const UnitType& type, const int& digits) const;
  QString getDisplaySigmaZ(const UnitType& type, const int& digits) const;
  QString getDisplayI(const int& digits) const;
  QString getDisplayJ(const int& digits) const;
  QString getDisplayK(const int& digits) const;
  QString getDisplaySigmaI(const int& digits) const;
  QString getDisplaySigmaJ(const int& digits) const;
  QString getDisplaySigmaK(const int& digits) const;
  QString getDisplayIsValid() const;
  QString getDisplayIsSolved() const override;

  // #################
  // save and load XML
  // #################

  QDomElement toOpenIndyXML(QDomDocument& xmlDoc) const override;
  bool fromOpenIndyXML(QDomElement& xmlElem) override;

 private:
  // ####################################################
  // references to station, reading and target geometries
  // ####################################################

  QPointer<Station> station;

  QPointer<Geometry> measuredTargetGeometry;

  // backing store for the getPosition() override, computed from xyz on
  // each call - see observation.cpp
  mutable Position positionCache;

  QPointer<Reading> reading;

  // ###################################################
  // booleans that indicate the state of the observation
  // ###################################################

  bool isValid;  // defines if the reading can be transformed, without any
                 // function, to a xyz/ijk observation
  bool isSolved;  // defines wether this observation is valid in current
                  // coordinate system
  bool hasDirection;  // defines if the observation has a valid ijk direction

  // #############################
  // xyz coordinates and statistic
  // #############################

  OiVec xyz;  // the homogeneous coordinate vector in the current display
              // coordinate system
  OiVec originalXyz;  // the homogeneous coordinate vector in the station system
                      // that produced this observation

  OiVec sigmaXyz;  // the homogeneous stdev vector in the current display
                   // coordinate system
  OiVec originalSigmaXyz;  // the homogeneous stdev vector in the station system
                           // that produced this observation

  // ###############################
  // ijk - unit vector and statistic
  // ###############################

  OiVec ijk;          // unit vector
  OiVec originalIjk;  // orignal unit vector

  OiVec sigmaIjk;          // stdev unit vector
  OiVec originalSigmaIjk;  // orignal sigma unit vector

  bool isDummyPoint;  // is dummy observation
};

}  // namespace oi

Q_DECLARE_METATYPE(oi::Observation*)

#endif  // OBSERVATION_H
