#ifndef FUNCTION_H
#define FUNCTION_H
#include <math.h>

#include <cmath>
#include <expected>
#include <map>
#include <optional>
#include <string>
#include <vector>

#include "circle.h"
#include "cone.h"
#include "coordinatesystem.h"
#include "core_export.h"
#include "cylinder.h"
#include "ellipse.h"
#include "ellipsoid.h"
#include "featurewrapper.h"
#include "functionobserver.h"
#include "guarded.h"
#include "hyperboloid.h"
#include "line.h"
#include "messagesink.h"
#include "nurbs.h"
#include "observation.h"
#include "paraboloid.h"
#include "pipelinestage.h"
#include "plane.h"
#include "pluginmetadata.h"
#include "point.h"
#include "pointcloud.h"
#include "producertransform.h"
#include "reading.h"
#include "scalarentityangle.h"
#include "scalarentitydistance.h"
#include "scalarentitymeasurementseries.h"
#include "scalarentitytemperature.h"
#include "slottedhole.h"
#include "sphere.h"
#include "station.h"
#include "statistic.h"
#include "torus.h"
#include "trafoparam.h"
#include "types.h"
#include "util.h"

namespace oi {

// ############################################
// helper classes to define function parameters
// ############################################

/*!
 * \brief The NeededElement class
 * Save the needed element types that are necessary to solve a function
 *
 * D12/domain-model.md S3: roleName replaces the old positional
 * InputElementKey ({eNotSet=-1, eDefault=0, ..., eDummyPoint=10000}, with
 * NeededElement's position in the function's input map determined by its
 * list index unconditionally - no more sentinel value doubling as a
 * position). A stable role *name* is what lets a MeasurementIntent tag
 * survive a fit function being swapped out (the tracker-dies scenario):
 * the tag anchors to (featureId, roleName), and any function that declares
 * the same roleName picks it back up, regardless of position. Functions
 * with a single, undistinguished input can leave this empty; BestFitPlane's
 * two same-typed inputs are the case that needed it - "default" and
 * "dummyPoint" (formerly eDefault/eDummyPoint).
 */
class NeededElement {
 public:
  ElementTypes typeOfElement;  // type of the needed input element
  std::string description;     // optional description for the needed element to
                               // show in the GUI
  bool infinite{};       // only one element or as many elements as one wants?
  std::string roleName;  // stable role name for tag-based resolution (D12);
                         // empty for an undistinguished single input

  // D16/domain-model.md S5: the ordered, declarative pipeline over this
  // role's tag-resolved candidates - Java-Stream-shaped, evaluated by
  // Function::exec() before (and, for a residual-based stage, again
  // after) the type-specific fit runs. Empty by default and left empty
  // for every function that doesn't declare one: a pipeline is opt-in
  // per role, and an undeclared pipeline must never override whatever
  // InputElement::shouldBeUsed a user or prior mechanism already set.
  std::vector<PipelineStage> pipeline;
};

/*!
 * \brief The InputElement class
 * Save the input elements that are used to solve a function
 */
class InputElement {
 public:
  InputElement() : id(-1), shouldBeUsed(true), isUsed(true) {}
  explicit InputElement(const int& id)
      : id(id), shouldBeUsed(true), isUsed(true) {}

  //! custom comparison operator to compare input elements by their id
  bool operator==(const InputElement& other) const {
    return this->id == other.id;
  }

  int id;  // the id of the element
  std::string label;
  ElementTypes typeOfElement;  // the type of the element

  // single parameters of this element that shall not be used in function
  // calculation e.g. ignore the radius of an input sphere
  std::vector<GeometryParameters> ignoredDestinationParams;

  // single parameters of the feature to be calculated that this element shall
  // not have effect on e.g. use a point only for a circle's plane, not the
  // radius
  std::vector<GeometryParameters> ignoredTargetParams;

  bool shouldBeUsed;  // true if this element should be used in function
                      // calculation (user specified)
  bool isUsed;  // true if this element is used in function calculation (plugin
                // specified)

  // element pointers (only valid for the specified element type)
  oi::Guarded<Station> station;
  oi::Guarded<CoordinateSystem> coordSystem;
  oi::Guarded<TrafoParam> trafoParam;

  oi::Guarded<Circle> circle;
  oi::Guarded<Cone> cone;
  oi::Guarded<Cylinder> cylinder;
  oi::Guarded<Ellipse> ellipse;
  oi::Guarded<Ellipsoid> ellipsoid;
  oi::Guarded<Hyperboloid> hyperboloid;
  oi::Guarded<Line> line;
  oi::Guarded<Nurbs> nurbs;
  oi::Guarded<Paraboloid> paraboloid;
  oi::Guarded<Plane> plane;
  oi::Guarded<Point> point;
  oi::Guarded<PointCloud> pointCloud;
  oi::Guarded<ScalarEntityAngle> scalarEntityAngle;
  oi::Guarded<ScalarEntityDistance> scalarEntityDistance;
  oi::Guarded<ScalarEntityMeasurementSeries> scalarEntityMeasurementSeries;
  oi::Guarded<ScalarEntityTemperature> scalarEntityTemperature;
  oi::Guarded<SlottedHole> slottedHole;
  oi::Guarded<Sphere> sphere;
  oi::Guarded<Torus> torus;

  oi::Guarded<Geometry> geometry;

  oi::Guarded<Observation> observation;
  oi::Guarded<Reading> polarReading;
  oi::Guarded<Reading> directionReading;
  oi::Guarded<Reading> distanceReading;
  oi::Guarded<Reading> cartesianReading;
  oi::Guarded<Reading> temperatureReading;
  oi::Guarded<Reading> levelReading;
  oi::Guarded<Reading> undefinedReading;
};

/*!
 * \brief The FixedParameter class
 * Save a value for each parameter that shall be fixed
 */
class FixedParameter {
 public:
  FixedParameter() : value(0.0) {}
  explicit FixedParameter(const GeometryParameters& param)
      : parameter(param), value(0.0) {}

  //! custom comparison operator to compare fixed parameters
  bool operator==(const FixedParameter& other) const {
    if (this->parameter == other.parameter) {
      return true;
    }
    return false;
  }

  GeometryParameters parameter;
  double value;
};

class OI_CORE_EXPORT FunctionUtil {
 protected:
  void rectifyNormalToDirection(OiVec& normal, OiVec& direction) {
    double cos = 0.0;  // cosine between normal and direction
    OiVec::dot(cos, normal, direction);
    double angle = std::abs(std::acos(cos));
    if (angle > (PI / 2.0) && angle < PI) {
      normal = normal * -1.0;
    }
  }

  void registerPointToPlane(OiVec& point, OiVec planeOrigin,
                            OiVec planeNormal) {
    // calculate the distance of the plane from the origin
    double d = NAN;
    OiVec::dot(d, planeOrigin, planeNormal);
    if (d < 0.0) {
      planeNormal = -1.0 * planeNormal;
      d = -d;
    }

    // calculate the distance of the point position from the plane
    double s = NAN;
    OiVec::dot(s, point, planeNormal);
    s = s - d;

    // project the point position into the plane
    point = point - s * planeNormal;
  }
};

// #####################
// function class itself
// #####################

/*!
 * \brief The Function class
 */
class OI_CORE_EXPORT Function : public oi::Trackable {
  friend class Feature;
  friend class BestFitPlaneUtil;

  friend struct codec::Access;  // the protobuf codec (see src/codec)

 public:
  explicit Function();

  ~Function() override;

  // ##############################
  // function initialization method
  // ##############################

  virtual void init();

  // ###################################################################
  // public exec method that calls the appropriate protected exec method
  // ###################################################################

  // Stage 7d/D21 (domain-model.md S7): execChecked() is the std::expected
  // boundary Feature::recalc() calls directly - producer/transform
  // dispatch, pipeline application, all of it, with a reason on failure
  // instead of a bare bool. exec() is kept, unchanged in signature, for
  // every pre-7d caller (tests calling a function directly without going
  // through Feature::recalc(), cfvisitor.cpp, OiJob) - it is now a thin
  // wrapper over execChecked().
  virtual std::expected<void, std::string> execChecked(
      const oi::Guarded<FeatureWrapper>& feature);
  virtual bool exec(const oi::Guarded<FeatureWrapper>& feature);

  // ###############################################################
  // Stage 7d/D21: attach-time capability check - does `function`
  // implement the mixin (Producer<T>/Transform<T>) required to act in
  // that slot for a feature of type `type`? Used by Feature::addFunction
  //(the runtime enforcement point - domain-model.md S7 "Dispatch
  // boundary") and safe to call speculatively (e.g. from a UI function
  // picker) without side effects, unlike execByFeatureType() which
  // actually runs the function.
  // ###############################################################

  static bool canActAsProducer(const FeatureTypes& type, Function* function);
  static bool canActAsTransform(const FeatureTypes& type, Function* function);

  // #############################################################
  // methods to get or set further information to solve a function
  // #############################################################

  [[nodiscard]] const std::vector<NeededElement>& getNeededElements() const;
  [[nodiscard]] const std::vector<FeatureTypes>& getApplicableFor() const;
  [[nodiscard]] const PluginMetaData& getMetaData() const;

  [[nodiscard]] const std::map<std::string, int>& getIntegerParameter() const;
  [[nodiscard]] const std::map<std::string, double>& getDoubleParameter() const;
  [[nodiscard]] const std::multimap<std::string, std::string>&
  getStringParameter() const;

  void setScalarInputParams(const ScalarInputParams& params);
  const ScalarInputParams& getScalarInputParams();

  [[nodiscard]] const std::vector<FixedParameter>& getFixedParameters() const;
  void fixParameter(const FixedParameter& parameter);
  void unfixParameter(const GeometryParameters& parameter);
  void unfixAllParameters();

  // ####################
  // get function results
  // ####################

  [[nodiscard]] const std::vector<std::string>& getResultProtocol() const;

  [[nodiscard]] const Statistic& getStatistic() const;

  // ###############
  // general getters
  // ###############

  [[nodiscard]] const int& getId() const;

  const bool& getSupportWeights();
  void setSupportWeights(bool supports);

  // ###################
  // get or set elements
  // ###################

  [[nodiscard]] const std::map<int, std::vector<InputElement>>&
  getInputElements() const;
  [[nodiscard]] InputElement getInputElement(const int& id) const;
  [[nodiscard]] InputElement getInputElement(const int& id,
                                             const int& position) const;
  [[nodiscard]] bool hasInputElement(const int& id) const;
  virtual void addInputElement(const InputElement& element,
                               const int& position);
  void removeInputElement(const int& id, const int& position);
  void removeInputElement(const int& id);
  void replaceInputElement(const InputElement& element, const int& position);

  // #################################
  // mark elements as used or not used
  // #################################

  bool getIsUsed(const int& position, const int& id);
  bool getShouldBeUsed(const int& position, const int& id);
  void setShouldBeUsed(const int& position, const int& id, const bool& state);

  // ###############################################
  // declare and inspect a NeededElement's pipeline
  // ###############################################

  // D16/domain-model.md S5: declare/read/mutate the ordered filter
  // pipeline for the NeededElement at `position` (same index space as
  // getInputElements()). Evaluated automatically by exec() - callers
  // that mutate this still need to trigger an ordinary recalc for it to
  // take effect, exactly like every other input-set change (OiJob does
  // not auto-recalc on tagPoint either; see oijob.h).
  [[nodiscard]] const std::vector<PipelineStage>& getPipeline(
      const int& position) const;
  void setPipeline(const int& position,
                   const std::vector<PipelineStage>& stages);
  void addPipelineStage(const int& position, const PipelineStage& stage);
  void clearPipeline(const int& position);

  // traceability requirement (D16/domain-model.md S5): what the pipeline
  // most recently excluded at `position`, and why. Repopulated every
  // exec(); empty until the function has run at least once, the same
  // way isUsed is only meaningful post-exec.
  [[nodiscard]] const std::vector<PipelineExclusion>& getPipelineExclusions(
      const int& position) const;

  // #############
  // clear results
  // #############

  virtual void clear();
  virtual void clearResults();

  // #####################################################
  // report function issues and changes to the host (formerly signals)
  // #####################################################

  // messages go to the sink if there is one (non-owning; see MessageSink for
  // its lifetime rules), otherwise they are dropped
  void setMessageSink(MessageSink* sink) { this->messageSink = sink; }
  [[nodiscard]] MessageSink* getMessageSink() const {
    return this->messageSink;
  }

  void sendMessage(const std::string& msg, const MessageTypes& msgType,
                   const MessageDestinations& msgDest =
                       MessageDestinations::eConsoleMessage) {
    if (this->messageSink != nullptr) {
      this->messageSink->onMessage(msg, msgType, msgDest);
    }
  }

  // change notifications go to the observer if there is one (non-owning)
  void setObserver(FunctionObserver* observer) {
    this->functionObserver = observer;
  }
  [[nodiscard]] FunctionObserver* getObserver() const {
    return this->functionObserver;
  }

 protected:
  // #################################
  // mark elements as used or not used
  // #################################

  void setIsUsed(const int& position, const int& id, const bool& state);

  // ###########################
  // input and output parameters
  // ###########################

  // meta information about the function (to tell OpenIndy what this function
  // does)
  std::vector<NeededElement> neededElements;
  std::vector<FeatureTypes> applicableFor;
  PluginMetaData metaData;

  // the needed scalar input parameters including default values
  std::map<std::string, int> integerParameters;
  std::map<std::string, double> doubleParameters;
  std::multimap<std::string, std::string> stringParameters;

  // user defined scalar input parameters
  ScalarInputParams scalarInputParams;

  // parameters of a geometry that should have a fixed value
  std::vector<FixedParameter> fixedParameters;

  std::vector<std::string> resultProtocol;

  // ##################
  // general attributes
  // ##################

  int id{};

  bool supportsWeights;

  Statistic statistic;

  std::map<int, std::vector<InputElement>> inputElements;

  // D16/domain-model.md S5: what each NeededElement position's pipeline
  // most recently excluded, and why - keyed the same way inputElements
  // is. Rebuilt every exec(), not persisted separately (it is fully
  // re-derivable from the declared pipeline plus current data, exactly
  // like isUsed).
  std::map<int, std::vector<PipelineExclusion>> pipelineExclusions;

  void filterObservations(
      std::vector<oi::Guarded<Observation>>& allUsableObservations,
      std::vector<oi::Guarded<Observation>>& inputObservations);
  void addDisplayResidual(int elementId, double vr);
  void addDisplayResidual(int elementId, double vx, double vy, double vz,
                          double v);
  void addDisplayResidual(int elementId, double vx, double vy, double vz,
                          double v, double vi, double vj, double vk);

 private:
  MessageSink* messageSink = nullptr;            // non-owning
  FunctionObserver* functionObserver = nullptr;  // non-owning

  void notifyInputElementsChanged() {
    if (this->functionObserver != nullptr) {
      this->functionObserver->onInputElementsChanged();
    }
  }

  // the pre-Stage-7b switch-on-feature-type dispatch, run once (or, for
  // a residual-based pipeline stage, twice - see execChecked()). Stage
  // 7d/D21: this is now also the producer/transform dispatch boundary -
  // dynamic_cast to Producer<ConcreteType>*/Transform<ConcreteType>* per
  // domain-model.md S7. A failed cast on both mixins *is* the "this
  // function doesn't belong in this slot" rejection - no separate
  // validation step required here (attach-time validation is a distinct,
  // additional check - see canActAsProducer/canActAsTransform above).
  std::expected<void, std::string> execByFeatureType(
      const oi::Guarded<FeatureWrapper>& feature);

  // deterministic pipeline stages (ExplicitList, Exclude) - need no fit
  // result, so they run before the first execByFeatureType() pass.
  // Returns whether any position had a non-empty pipeline declared.
  bool applyDeterministicPipelineStages();

  // eResidualOutlierRejectionStage - needs a completed fit's residuals,
  // so it runs after the first execByFeatureType() pass, using this
  // pass's Statistic. Returns whether it changed any shouldBeUsed, i.e.
  // whether a second execByFeatureType() pass is warranted.
  bool applyResidualPipelineStages();

  // looks up the scalar residual magnitude Function::addDisplayResidual
  // recorded for elementId in the current statistic, trying the "v" key
  //(most fit functions) then "vr" (e.g. BestFitSphere). Not every fit
  // function reports one under either key (domain-model.md S9) - this
  // degrades to nullopt rather than guessing.
  [[nodiscard]] std::optional<double> residualMagnitude(int elementId) const;
};

}  // namespace oi

#ifndef STR
#define STR(x) #x
#endif
#ifndef STR2
#define STR2(x) STR(x)
#endif

#ifndef CONCAT
#define CONCAT(a, b) a##b
#endif
#ifndef CONCAT2
#define CONCAT2(a, b) CONCAT(a, b)
#endif

#include "pluginabi.h"

#define FUNCTION_PREFIX de.openIndy.plugin.function.v
#define Function_iid STR2(CONCAT2(FUNCTION_PREFIX, OI_PLUGIN_ABI_VERSION))

#endif  // FUNCTION_H
