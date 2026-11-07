#ifndef PLANE_H
#define PLANE_H

#include <QObject>
#include <QtXml>

#include "core_export.h"
#include "direction.h"
#include "geometry.h"
#include "position.h"

namespace oi {

/*!
 * \brief The Plane class
 */
class OI_CORE_EXPORT Plane : public Geometry {
  friend struct codec::Access;  // the protobuf codec (see src/codec)
  Q_OBJECT

 public:
  explicit Plane(QObject* parent = nullptr);
  Plane(const Position& xyz, const Direction& normal,
        QObject* parent = nullptr);
  Plane(const Direction& normal, const double& d, QObject* parent = nullptr);

  Plane(const Plane& copy, QObject* parent = nullptr);

  Plane& operator=(const Plane& copy);

  ~Plane() override;

  // ########################################
  // order of unknown parameters (Qxx-matrix)
  // ########################################

  enum PlaneUnknowns {
    unknownPositionX = 0,
    unknownPositionY,
    unknownPositionZ,
    unknownNormalI,
    unknownNormalJ,
    unknownNormalK
  };

  // ###################################################################
  // check wether this geometry has a direction, a position and a radius
  // ###################################################################

  [[nodiscard]] bool hasDirection() const override;
  [[nodiscard]] bool hasPosition() const override;

  // ###########################
  // get or set plane parameters
  // ###########################

  // const Direction &getDirection() const;
  // const Position &getPosition() const;

  [[nodiscard]] const double& getDistanceFromOrigin() const;

  void setPlane(const Position& xyz, const Direction& normal);
  void setPlane(const Direction& normal, const double& d);

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

 private:
  // ################
  // plane attributes
  // ################

  double d{};  // smallest distance from origin
};

}  // namespace oi

#endif  // PLANE_H
