#ifndef HYPERBOLOID_H
#define HYPERBOLOID_H

#include <QObject>
#include <QtXml>

#include "core_export.h"
#include "direction.h"
#include "geometry.h"
#include "position.h"

namespace oi {

/*!
 * \brief The Hyperboloid class
 * Defines a single shell hyperboloid of rotation of the form 1 = x^2/a^2 +
 * y^2/b^2 - z^2/c^2, a=b>0, c>0
 */
class OI_CORE_EXPORT Hyperboloid : public Geometry {
  Q_OBJECT

 public:
  explicit Hyperboloid(QObject* parent = nullptr);
  Hyperboloid(const Position& center, const Direction& axis, const double& a,
              const double& c, QObject* parent = nullptr);

  Hyperboloid(const Hyperboloid& copy, QObject* parent = nullptr);

  Hyperboloid& operator=(const Hyperboloid& copy);

  ~Hyperboloid() override;

  // ########################################
  // order of unknown parameters (Qxx-matrix)
  // ########################################

  enum HyperboloidUnknowns {
    unknownCenterX = 0,
    unknownCenterY,
    unknownCenterZ,
    unknownAxisI,
    unknownAxisJ,
    unknownAxisK,
    unknownA,
    unknownC
  };

  // ###################################################################
  // check wether this geometry has a direction, a position and a radius
  // ###################################################################

  [[nodiscard]] bool hasDirection() const override;
  [[nodiscard]] bool hasPosition() const override;

  // #################################
  // get or set hyperboloid parameters
  // #################################

  [[nodiscard]] const double& getA() const;
  [[nodiscard]] const double& getC() const;

  void setHyperboloid(const Position& center, const Direction& axis,
                      const double& a, const double& c);

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

  // #################
  // save and load XML
  // #################

  QDomElement toOpenIndyXML(QDomDocument& xmlDoc) const override;
  bool fromOpenIndyXML(QDomElement& xmlElem) override;

  // ###############
  // display methods
  // ###############

  [[nodiscard]] QString getDisplayA(
      const UnitType& type, const int& digits,
      const bool& showDiff = false) const override;
  [[nodiscard]] QString getDisplayC(
      const UnitType& type, const int& digits,
      const bool& showDiff = false) const override;

 private:
  // ######################
  // hyperboloid attributes
  // ######################

  double a{};
  double c{};
};

}  // namespace oi

#endif  // HYPERBOLOID_H
