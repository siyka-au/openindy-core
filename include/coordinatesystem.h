#ifndef COORDINATESYSTEM_H
#define COORDINATESYSTEM_H

#include <QJsonObject>
#include <QList>
#include <QObject>
#include <QPointer>

#include "core_export.h"
#include "direction.h"
#include "feature.h"
#include "position.h"

class TrafoController;
class ProjectExchanger;

namespace oi {

class Observation;
class TrafoParam;
class Geometry;
class Station;
class BundleAdjustment;

/*!
 * \brief The CoordinateSystem class
 */
class OI_CORE_EXPORT CoordinateSystem : public Feature {
  friend class Station;
  friend class OiJob;
  friend class ::TrafoController;
  friend class ::ProjectExchanger;
  friend class Observation;
  friend struct codec::Access;  // the protobuf codec (see src/codec)
  Q_OBJECT

 public:
  explicit CoordinateSystem(QObject* parent = nullptr);
  explicit CoordinateSystem(bool isBundleSystem, QObject* parent = nullptr);
  explicit CoordinateSystem(const QPointer<Station>& station,
                            QObject* parent = nullptr);

  CoordinateSystem(const CoordinateSystem& copy, QObject* parent = nullptr);

  CoordinateSystem& operator=(const CoordinateSystem& copy);

  ~CoordinateSystem() override;

  // #######################################
  // get or set coordinate system attributes
  // #######################################

  // is active system
  [[nodiscard]] const bool& getIsActiveCoordinateSystem() const;
  void setActiveCoordinateSystemState(const bool& isActiveCoordinateSystem);

  // is stations system
  [[nodiscard]] const bool& getIsStationSystem() const;
  [[nodiscard]] const QPointer<Station>& getStation() const;

  // is bundle system
  [[nodiscard]] const bool& getIsBundleSystem() const;

  // system parameters
  [[nodiscard]] const Position& getOrigin() const;
  [[nodiscard]] const Direction& getXAxis() const;
  [[nodiscard]] const Direction& getYAxis() const;
  [[nodiscard]] const Direction& getZAxis() const;
  void setCoordinateSystem(const Position& origin, const Direction& xAxis,
                           const Direction& yAxis, const Direction& zAxis);
  void resetOriginAndAxis();                    // TODO OI-42 to feature ?
  void transformOriginAndAxis(OiMat trafoMat);  // TODO OI-42 to feature ?

  // expansion origin
  [[nodiscard]] const Position& getExpansionOrigin() const;
  void setExpansionOrigin(const Position& expansionOrigin);

  // ##################################################
  // get or set observations, trafo params and nominals
  // ##################################################

  // observations
  [[nodiscard]] const QList<QPointer<Observation>>& getObservations() const;
  [[nodiscard]] QPointer<Observation> getObservation(
      const int& observationId) const;
  bool addObservation(const QPointer<Observation>& observation);

  // transformation parameters
  [[nodiscard]] const QList<QPointer<TrafoParam>>& getTransformationParameters()
      const;
  [[nodiscard]] const QList<QPointer<TrafoParam>> getTransformationParameters(
      const QPointer<CoordinateSystem>& to) const;
  bool addTransformationParameter(const QPointer<TrafoParam>& trafoParam);
  bool removeTransformationParameter(const QPointer<TrafoParam>& trafoParam);

  // nominals
  [[nodiscard]] const QList<QPointer<FeatureWrapper>>& getNominals() const;
  bool addNominal(const QPointer<FeatureWrapper>& nominal);
  bool addNominals(const QList<QPointer<FeatureWrapper>>& nominals);
  bool removeNominal(const QPointer<FeatureWrapper>& nominal);
  bool removeNominal(const int& featureId);

  // bundle template
  [[nodiscard]] const QJsonObject& getBundleTemplate() const;
  void setBundleTemplate(const QJsonObject& bundleTemplate);

  // bundle adjustment
  [[nodiscard]] const QPointer<BundleAdjustment>& getBundlePlugin() const;
  void setBundlePlugin(const QPointer<BundleAdjustment>& bundle);

  // ###########################
  // reexecute the function list
  // ###########################

  void recalc() override;

  // ###############
  // display methods
  // ###############

  [[nodiscard]] QString getDisplayX(
      const UnitType& type, const int& digits,
      const bool& showDiff = false) const override;
  [[nodiscard]] QString getDisplayY(
      const UnitType& type, const int& digits,
      const bool& showDiff = false) const override;
  [[nodiscard]] QString getDisplayZ(
      const UnitType& type, const int& digits,
      const bool& showDiff = false) const override;
  [[nodiscard]] QString getDisplayPrimaryI(
      const int& digits, const bool& showDiff = false) const override;
  [[nodiscard]] QString getDisplayPrimaryJ(
      const int& digits, const bool& showDiff = false) const override;
  [[nodiscard]] QString getDisplayPrimaryK(
      const int& digits, const bool& showDiff = false) const override;

  [[nodiscard]] QString getDisplayExpansionOriginX(
      const UnitType& type, const int& digits) const override;
  [[nodiscard]] QString getDisplayExpansionOriginY(
      const UnitType& type, const int& digits) const override;
  [[nodiscard]] QString getDisplayExpansionOriginZ(
      const UnitType& type, const int& digits) const override;
  [[nodiscard]] QString getDisplayXAxisI(const int& digits) const override;
  [[nodiscard]] QString getDisplayXAxisJ(const int& digits) const override;
  [[nodiscard]] QString getDisplayXAxisK(const int& digits) const override;
  [[nodiscard]] QString getDisplayYAxisI(const int& digits) const override;
  [[nodiscard]] QString getDisplayYAxisJ(const int& digits) const override;
  [[nodiscard]] QString getDisplayYAxisK(const int& digits) const override;
  [[nodiscard]] QString getDisplayZAxisI(const int& digits) const override;
  [[nodiscard]] QString getDisplayZAxisJ(const int& digits) const override;
  [[nodiscard]] QString getDisplayZAxisK(const int& digits) const override;

 signals:

  // #################################################
  // signals to inform about coordinate system changes
  // #################################################

  void observationsChanged(const int& coordId, const int& obsId);
  void transformationParametersChanged(const int& coordId);
  void nominalsChanged(const int& coordId);
  void activeCoordinateSystemChanged(const int& coordId);
  void bundlePluginChanged(const int& coordId);

 protected:
  // ######################################
  // update feature id when the job was set
  // ######################################

  void setUpFeatureId() override;

 private slots:

  // ##############
  // helper methods
  // ##############

  void removeObservation(const QPointer<Observation>& obs);

 private:
  // ####################################
  // general coordinate system attributes
  // ####################################

  // system parameters
  Position origin;  // origin of the coordinate system in the current display
                    // coordinate system
  Direction xAxis;  // x axis of the coordinate system in the current display
                    // coordinate system
  Direction yAxis;  // y axis of the coordinate system in the current display
                    // coordinate system
  Direction zAxis;  // z (ijk / normal) axis of the coordinate system in the
                    // current display coordinate system

  bool isStationSystem;  // true if the coordinate system is a station system
  bool isBundleSystem;   // true if the coordinate system is a bundle system
  bool isActiveCoordinateSystem;

  // represents the point from which the Part object expands with temperature
  Position expansionOrigin;

  // holds the station object (only if this is a station system)
  QPointer<Station> station;

  // the bundle plugin that is used to solve the bundle
  QJsonObject bundleTemplate;
  QPointer<BundleAdjustment> bundlePlugin;

  // #######################################
  // observations, trafo params and nominals
  // #######################################

  // observations
  QList<QPointer<Observation>> observationsList;
  QMap<int, QPointer<Observation>>
      observationsMap;  // same observations but id as key

  // trafo params
  QList<QPointer<TrafoParam>> trafoParams;

  // nominals
  QList<QPointer<FeatureWrapper>> nominalsList;
  QMap<int, QPointer<FeatureWrapper>>
      nominalsMap;  // same geometries but id as key
};

}  // namespace oi

#endif  // COORDINATESYSTEM_H
