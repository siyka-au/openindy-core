#ifndef POINT_H
#define POINT_H

#include <QObject>
#include <QtXml>

#include "geometry.h"
#include "measurementintent.h"
#include "position.h"

namespace oi {

/*!
 * \brief The Point class
 */
class OI_CORE_EXPORT Point : public Geometry {
  friend class Station;
  Q_OBJECT

 public:
  explicit Point(QObject* parent = nullptr);
  explicit Point(const Position& xyz, QObject* parent = nullptr);
  Point(int id, const Position& xyz, QObject* parent = nullptr);

  Point(const Point& copy, QObject* parent = nullptr);

  Point& operator=(const Point& copy);

  ~Point() override;

  // ########################################
  // order of unknown parameters (Qxx-matrix)
  // ########################################

  enum PointUnknowns { unknownCenterX = 0, unknownCenterY, unknownCenterZ };

  // ###################################################################
  // check wether this geometry has a direction, a position and a radius
  // ###################################################################

  [[nodiscard]] bool hasPosition() const override;

  // ###########################
  // get or set point parameters
  // ###########################

  [[nodiscard]] const Position& getPosition() const override;

  void setPoint(const Position& xyz);

  [[nodiscard]] const double& getDistance() const;
  void setDistance(const double distance);

  // #############################
  // get or set unknown parameters
  // #############################

  [[nodiscard]] QMap<GeometryParameters, QString> getUnknownParameters(
      const QMap<DimensionType, UnitType>& displayUnits,
      const QMap<DimensionType, int>& displayDigits) const override;
  void applyUnknownParameters(
      const QMap<GeometryParameters, double>& parameters) override;

  // ###########################
  // reexecute the function list
  // ###########################

  void recalc() override;

  // ###############################################################
  // D19: a Point produced directly by an acquisition (has readings
  // attached via its observations, see Geometry::getObservations) is
  // owned by the station that produced it, not by a fit function - it
  // has no producer function at all, which the inherited
  // Geometry::getOwnerDescription would misread as user-owned. See
  // point.cpp for the rule.
  // ###############################################################

  [[nodiscard]] std::optional<QString> getOwnerDescription() const override;

  // ###############################################################
  // D12: what this point was shot for, stamped at acquisition (or by
  // hand later - domain-model.md S3 makes these the same operation). A
  // fit's point input resolves by querying this, not by a stored
  // membership list on the consuming feature.
  // ###############################################################

  [[nodiscard]] const QList<MeasurementIntent>& getIntents() const;
  void addIntent(const MeasurementIntent& intent);
  bool removeIntent(const MeasurementIntent& intent);
  [[nodiscard]] bool hasIntent(int featureId, const QString& roleName) const;

  // #################
  // save and load XML
  // #################

  QDomElement toOpenIndyXML(QDomDocument& xmlDoc) const override;
  bool fromOpenIndyXML(QDomElement& xmlElem) override;

  [[nodiscard]] QString getDisplayDistance(const UnitType& type,
                                           const int& digits,
                                           const bool& showDiff) const override;
  double distance;  // depends on function

 protected:
  QList<MeasurementIntent> intents;
};

}  // namespace oi

#endif  // POINT_H
