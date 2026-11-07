#ifndef COORDINATESYSTEM_H
#define COORDINATESYSTEM_H
#include <map>
#include <vector>

#include "core_export.h"
#include "direction.h"
#include "feature.h"
#include "guarded.h"
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

 public:
  explicit CoordinateSystem();
  explicit CoordinateSystem(bool isBundleSystem);
  explicit CoordinateSystem(const oi::Guarded<Station>& station);

  CoordinateSystem(const CoordinateSystem& copy);

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
  [[nodiscard]] const oi::Guarded<Station>& getStation() const;

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
  [[nodiscard]] const std::vector<oi::Guarded<Observation>>& getObservations()
      const;
  [[nodiscard]] oi::Guarded<Observation> getObservation(
      const int& observationId) const;
  bool addObservation(const oi::Guarded<Observation>& observation);

  // transformation parameters
  [[nodiscard]] const std::vector<oi::Guarded<TrafoParam>>&
  getTransformationParameters() const;
  [[nodiscard]] const std::vector<oi::Guarded<TrafoParam>>
  getTransformationParameters(const oi::Guarded<CoordinateSystem>& to) const;
  bool addTransformationParameter(const oi::Guarded<TrafoParam>& trafoParam);
  bool removeTransformationParameter(const oi::Guarded<TrafoParam>& trafoParam);

  // nominals
  [[nodiscard]] const std::vector<oi::Guarded<FeatureWrapper>>& getNominals()
      const;
  bool addNominal(const oi::Guarded<FeatureWrapper>& nominal);
  bool addNominals(const std::vector<oi::Guarded<FeatureWrapper>>& nominals);
  bool removeNominal(const oi::Guarded<FeatureWrapper>& nominal);
  bool removeNominal(const int& featureId);

  // bundle template
  // the bundle template as JSON text; the model keeps it opaque
  [[nodiscard]] const std::string& getBundleTemplate() const;
  void setBundleTemplate(const std::string& bundleTemplate);

  // bundle adjustment
  [[nodiscard]] const oi::Guarded<BundleAdjustment>& getBundlePlugin() const;
  void setBundlePlugin(const oi::Guarded<BundleAdjustment>& bundle);

  // ###########################
  // reexecute the function list
  // ###########################

  void recalc() override;

  // ###############
  // display methods
  // ###############

  [[nodiscard]] std::string getDisplayX(
      const UnitType& type, const int& digits,
      const bool& showDiff = false) const override;
  [[nodiscard]] std::string getDisplayY(
      const UnitType& type, const int& digits,
      const bool& showDiff = false) const override;
  [[nodiscard]] std::string getDisplayZ(
      const UnitType& type, const int& digits,
      const bool& showDiff = false) const override;
  [[nodiscard]] std::string getDisplayPrimaryI(
      const int& digits, const bool& showDiff = false) const override;
  [[nodiscard]] std::string getDisplayPrimaryJ(
      const int& digits, const bool& showDiff = false) const override;
  [[nodiscard]] std::string getDisplayPrimaryK(
      const int& digits, const bool& showDiff = false) const override;

  [[nodiscard]] std::string getDisplayExpansionOriginX(
      const UnitType& type, const int& digits) const override;
  [[nodiscard]] std::string getDisplayExpansionOriginY(
      const UnitType& type, const int& digits) const override;
  [[nodiscard]] std::string getDisplayExpansionOriginZ(
      const UnitType& type, const int& digits) const override;
  [[nodiscard]] std::string getDisplayXAxisI(const int& digits) const override;
  [[nodiscard]] std::string getDisplayXAxisJ(const int& digits) const override;
  [[nodiscard]] std::string getDisplayXAxisK(const int& digits) const override;
  [[nodiscard]] std::string getDisplayYAxisI(const int& digits) const override;
  [[nodiscard]] std::string getDisplayYAxisJ(const int& digits) const override;
  [[nodiscard]] std::string getDisplayYAxisK(const int& digits) const override;
  [[nodiscard]] std::string getDisplayZAxisI(const int& digits) const override;
  [[nodiscard]] std::string getDisplayZAxisJ(const int& digits) const override;
  [[nodiscard]] std::string getDisplayZAxisK(const int& digits) const override;

 protected:
  // ######################################
  // update feature id when the job was set
  // ######################################

  void setUpFeatureId() override;

 private:
  // ##############
  // helper methods
  // ##############

  void removeObservation(const oi::Guarded<Observation>& obs);

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
  oi::Guarded<Station> station;

  // the bundle plugin that is used to solve the bundle
  std::string bundleTemplate;
  oi::Guarded<BundleAdjustment> bundlePlugin;

  // #######################################
  // observations, trafo params and nominals
  // #######################################

  // observations
  std::vector<oi::Guarded<Observation>> observationsList;
  std::map<int, oi::Guarded<Observation>>
      observationsMap;  // same observations but id as key

  // trafo params
  std::vector<oi::Guarded<TrafoParam>> trafoParams;

  // nominals
  std::vector<oi::Guarded<FeatureWrapper>> nominalsList;
  std::map<int, oi::Guarded<FeatureWrapper>>
      nominalsMap;  // same geometries but id as key
};

}  // namespace oi

#endif  // COORDINATESYSTEM_H
