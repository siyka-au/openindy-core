#include "trafo_param.h"

#include <cmath>
#include <map>
#include <vector>

#include "containers.h"
#include "feature_wrapper.h"
#include "oi_job.h"
#include "snapshot.h"
#include "string_util.h"

using namespace oi;
using namespace oi::math;

/*!
 * \brief TrafoParam::TrafoParam
 */
TrafoParam::TrafoParam()
    : Feature(), translation(3), rotation(3), scale(3), homogenMatrix(4, 4) {
  // set up feature wrapper
  if (!this->selfFeature.isNull()) {
    this->selfFeature->setTrafoParam(this);
  }

  // default values for trafo param attributes
  this->isUsed = false;
  this->isDatumTrafo = false;

  // default values for transformation parameters
  this->scale.setAt(0, 1.0);
  this->scale.setAt(1, 1.0);
  this->scale.setAt(2, 1.0);
  this->homogenMatrix.setAt(0, 0, 1.0);
  this->homogenMatrix.setAt(1, 1, 1.0);
  this->homogenMatrix.setAt(2, 2, 1.0);
  this->homogenMatrix.setAt(3, 3, 1.0);
}

/*!
 * \brief TrafoParam::TrafoParam
 * \param copy
 */
TrafoParam::TrafoParam(const TrafoParam& copy) : Feature(copy) {
  // set up feature wrapper
  if (!this->selfFeature.isNull()) {
    this->selfFeature->setTrafoParam(this);
  }

  // copy attributes
  this->isUsed = copy.isUsed;
  this->isDatumTrafo = copy.isDatumTrafo;

  // copy transformation parameters
  this->rotation = copy.rotation;
  this->translation = copy.translation;
  this->scale = copy.scale;
  this->homogenMatrix = copy.homogenMatrix;

  // Stage 7d/D21 trap (domain-model.md S10): unlike Geometry's copy
  // constructor, this one never copied `statistic` - harmless while
  // exec(TrafoParam&) only ever mutated a live object in place, but a
  // real bug once a Producer<TrafoParam> builds a fresh local and hands
  // it back by value (Helmert7Param::exec()), because *this* copy
  // constructor runs on the way into std::expected<TrafoParam, std::string>,
  // before execAs<T>'s explicit statistic propagation ever gets a chance
  // to see it. Found via tst_calibratedconstellation while migrating.
  this->statistic = copy.statistic;
}

/*!
 * \brief TrafoParam::operator =
 * \param copy
 * \return
 */
TrafoParam& TrafoParam::operator=(const TrafoParam& copy) {
  // set up feature wrapper
  if (!this->selfFeature.isNull()) {
    this->selfFeature->setTrafoParam(this);
  }

  // copy attributes
  this->isUsed = copy.isUsed;
  this->isDatumTrafo = copy.isDatumTrafo;

  // copy transformation parameters
  this->rotation = copy.rotation;
  this->translation = copy.translation;
  this->scale = copy.scale;
  this->homogenMatrix = copy.homogenMatrix;

  // see the copy constructor above for why this was missing
  this->statistic = copy.statistic;

  return *this;
}

/*!
 * \brief TrafoParam::~TrafoParam
 */
TrafoParam::~TrafoParam() {
  // delete this trafo set in from system
  if (!this->from.isNull()) {
    this->from->removeTransformationParameter(this);
  }

  // delete this trafo set in to system
  if (!this->to.isNull()) {
    this->to->removeTransformationParameter(this);
  }
}

/*!
 * \brief TrafoParam::getIsUsed
 * \return
 */
const bool& TrafoParam::getIsUsed() const { return this->isUsed; }

/*!
 * \brief TrafoParam::setIsUsed
 * \param isUsed
 */
void TrafoParam::setIsUsed(const bool& isUsed) {
  if (this->isUsed != isUsed) {
    // set used state
    this->isUsed = isUsed;

    this->notify(&FeatureObserver::onIsUsedChanged, this->id);
  }
}

/*!
 * \brief TrafoParam::getStartSystem
 * \return
 */
const oi::Guarded<CoordinateSystem>& TrafoParam::getStartSystem() const {
  return this->from;
}

/*!
 * \brief TrafoParam::getDestinationSystem
 * \return
 */
const oi::Guarded<CoordinateSystem>& TrafoParam::getDestinationSystem() const {
  return this->to;
}

/*!
 * \brief TrafoParam::setCoordinateSystems
 * \param from
 * \param to
 * \return
 */
bool TrafoParam::setCoordinateSystems(const oi::Guarded<CoordinateSystem>& from,
                                      const oi::Guarded<CoordinateSystem>& to) {
  // check if to and from were already set
  if (!this->to.isNull() || !this->from.isNull()) {
    return false;
  }

  if (!from.isNull() && !to.isNull()) {
    // check if to and from are in the same job
    if (!this->job.isNull()) {
      oi::Guarded<FeatureWrapper> jobFrom =
          this->job->getFeatureById(from->getId());
      oi::Guarded<FeatureWrapper> jobTo =
          this->job->getFeatureById(to->getId());
      if (jobFrom.isNull() || jobTo.isNull() ||
          jobFrom->getCoordinateSystem().isNull() ||
          jobTo->getCoordinateSystem().isNull() ||
          jobFrom->getCoordinateSystem() != from ||
          jobTo->getCoordinateSystem() != to) {
        return false;
      }
    }

    this->from = from;
    this->to = to;
    from->addTransformationParameter(this);
    to->addTransformationParameter(this);

    this->notify(&FeatureObserver::onCoordinateSystemsChanged, this->id);

    return true;
  }

  return false;
}

/*!
 * \brief TrafoParam::getIsBundle
 * \return
 */
bool TrafoParam::getIsBundle() const {
  if (!this->from.isNull() && !this->to.isNull()) {
    if (this->from->getIsBundleSystem() || this->to->getIsBundleSystem()) {
      return true;
    }
  }
  return false;
}

/*!
 * \brief TrafoParam::getStatistic
 * \return
 */
const Statistic& TrafoParam::getStatistic() const { return this->statistic; }

/*!
 * \brief TrafoParam::setStatistic
 * \param statistic
 */
void TrafoParam::setStatistic(const Statistic& statistic) {
  this->statistic = statistic;
}

/*!
 * \brief TrafoParam::getIsDatumTrafo
 * \return
 */
const bool& TrafoParam::getIsDatumTrafo() const { return this->isDatumTrafo; }

/*!
 * \brief TrafoParam::setIsDatumTrafo
 * \param isDatumTrafo
 */
void TrafoParam::setIsDatumTrafo(const bool& isDatumTrafo) {
  if (this->isDatumTrafo != isDatumTrafo) {
    this->isDatumTrafo = isDatumTrafo;
  }

  this->notify(&FeatureObserver::onIsDatumTrafoChanged, this->id);
}

/*!
 * \brief TrafoParam::getHomogenMatrix
 * \return
 */
const OiMat& TrafoParam::getHomogenMatrix() const {
  return this->homogenMatrix;
}

/*!
 * \brief TrafoParam::getTranslation
 * \return
 */
const OiVec& TrafoParam::getTranslation() const { return this->translation; }

/*!
 * \brief TrafoParam::getRotation
 * \return
 */
const OiVec& TrafoParam::getRotation() const { return this->rotation; }

/*!
 * \brief TrafoParam::getScale
 * \return
 */
const OiVec& TrafoParam::getScale() const { return this->scale; }

/*!
 * \brief TrafoParam::setTransformationParameters
 * Sets the transformation parameters. Input are 3 vectors of size 3.
 * \param rotation
 * \param translation
 * \param scale
 * \return
 */
bool TrafoParam::setTransformationParameters(const OiVec& rotation,
                                             const OiVec& translation,
                                             const OiVec& scale) {
  if (rotation.getSize() == 3 && translation.getSize() == 3 &&
      scale.getSize() == 3) {
    // update transformation parameters
    this->translation = translation;
    this->scale = scale;
    this->rotation = rotation;

    // set up translation matrix
    OiMat tMat(4, 4);
    for (int i = 0; i < 4; i++) {
      tMat.setAt(i, i, 1.0);
    }
    tMat.setAt(0, 3, translation.getAt(0));
    tMat.setAt(1, 3, translation.getAt(1));
    tMat.setAt(2, 3, translation.getAt(2));

    // set up scale matrix
    OiMat sMat(4, 4);
    sMat.setAt(0, 0, scale.getAt(0));
    sMat.setAt(1, 1, scale.getAt(1));
    sMat.setAt(2, 2, scale.getAt(2));
    sMat.setAt(3, 3, 1.0);

    // set up rotation matrix
    double rx = rotation.getAt(0);
    double ry = rotation.getAt(1);
    double rz = rotation.getAt(2);
    OiMat rMat(4, 4);
    rMat.setAt(0, 0, std::cos(ry) * std::cos(rz));
    rMat.setAt(0, 1,
               std::cos(rx) * std::sin(rz) +
                   std::sin(rx) * std::sin(ry) * std::cos(rz));
    rMat.setAt(0, 2,
               std::sin(rx) * std::sin(rz) -
                   std::cos(rx) * std::sin(ry) * std::cos(rz));
    rMat.setAt(0, 3, 0.0);
    rMat.setAt(1, 0, -std::cos(ry) * std::sin(rz));
    rMat.setAt(1, 1,
               std::cos(rx) * std::cos(rz) -
                   std::sin(rx) * std::sin(ry) * std::sin(rz));
    rMat.setAt(1, 2,
               std::sin(rx) * std::cos(rz) +
                   std::cos(rx) * std::sin(ry) * std::sin(rz));
    rMat.setAt(1, 3, 0.0);
    rMat.setAt(2, 0, std::sin(ry));
    rMat.setAt(2, 1, -std::sin(rx) * std::cos(ry));
    rMat.setAt(2, 2, std::cos(rx) * std::cos(ry));
    rMat.setAt(2, 3, 0.0);
    rMat.setAt(3, 0, 0.0);
    rMat.setAt(3, 1, 0.0);
    rMat.setAt(3, 2, 0.0);
    rMat.setAt(3, 3, 1.0);

    // update homogeneous matrix
    this->homogenMatrix = tMat * rMat * sMat;
    this->isSolved = true;

    this->notify(&FeatureObserver::onTransformationParameterChanged, this->id);

    return true;
  }

  return false;
}

/*!
 * \brief TrafoParam::setTransformationParameters
 * Sets the transformation parameters. Input are 3 homogeneous matrices of size
 * 4x4.
 * \param rotation
 * \param translation
 * \param scale
 * \return
 */
bool TrafoParam::setTransformationParameters(const OiMat& rotation,
                                             const OiMat& translation,
                                             const OiMat& scale) {
  if (rotation.getColCount() == 4 && rotation.getRowCount() == 4 &&
      translation.getColCount() == 4 && translation.getRowCount() == 4 &&
      scale.getColCount() == 4 && scale.getRowCount() == 4) {
    OiVec t(3);
    OiVec r(3);
    OiVec s(3);
    for (int i = 0; i < 3; i++) {
      t.setAt(i, translation.getAt(i, 3));
      s.setAt(i, scale.getAt(i, i));
    }

    r.setAt(0,
            std::atan2(-rotation.getAt(2, 1), rotation.getAt(2, 2)));  // alpha
    r.setAt(1, std::asin(rotation.getAt(2, 0)));                       // beta
    r.setAt(2,
            std::atan2(-rotation.getAt(1, 0), rotation.getAt(0, 0)));  // gamma
    if (std::fabs(std::cos(r.getAt(1)) * std::cos(r.getAt(2))) -
            std::fabs(rotation.getAt(0, 0)) >
        0.01) {
      r.setAt(1, kPi - r.getAt(1));
    }

    this->translation = t;
    this->scale = s;
    this->rotation = r;

    this->homogenMatrix = translation * rotation * scale;
    this->isSolved = true;

    this->notify(&FeatureObserver::onTransformationParameterChanged, this->id);

  } else {
    return false;
  }

  return true;
}

/*!
 * \brief TrafoParam::setTransformationParameters
 * Sets the transformation parameters. Input is one matrix of size 4x4.
 * \param homogenMatrix
 * \return
 */
bool TrafoParam::setTransformationParameters(const OiMat& /*homogenMatrix*/) {
  return false;
}

/*!
 * \brief TrafoParam::getUnknownParameters
 * \param displayUnits
 * \param displayDigits
 * \return
 */
std::map<TrafoParamParameters, std::string> TrafoParam::getUnknownParameters(
    const std::map<DimensionType, UnitType>& displayUnits,
    const std::map<DimensionType, int>& displayDigits) const {
  std::map<TrafoParamParameters, std::string> parameters;

  parameters.insert_or_assign(
      TrafoParamParameters::eUnknownTX,
      this->getDisplayTranslationX(
          oi::value(displayUnits, DimensionType::eMetric, UnitType::eUnitMeter),
          oi::value(displayDigits, DimensionType::eMetric, 0)));
  parameters.insert_or_assign(
      TrafoParamParameters::eUnknownTY,
      this->getDisplayTranslationY(
          oi::value(displayUnits, DimensionType::eMetric, UnitType::eUnitMeter),
          oi::value(displayDigits, DimensionType::eMetric, 0)));
  parameters.insert_or_assign(
      TrafoParamParameters::eUnknownTZ,
      this->getDisplayTranslationZ(
          oi::value(displayUnits, DimensionType::eMetric, UnitType::eUnitMeter),
          oi::value(displayDigits, DimensionType::eMetric, 0)));
  parameters.insert_or_assign(
      TrafoParamParameters::eUnknownRX,
      this->getDisplayRotationX(
          oi::value(displayUnits, DimensionType::eAngular,
                    UnitType::eUnitRadiant),
          oi::value(displayDigits, DimensionType::eAngular, 0)));
  parameters.insert_or_assign(
      TrafoParamParameters::eUnknownRY,
      this->getDisplayRotationY(
          oi::value(displayUnits, DimensionType::eAngular,
                    UnitType::eUnitRadiant),
          oi::value(displayDigits, DimensionType::eAngular, 0)));
  parameters.insert_or_assign(
      TrafoParamParameters::eUnknownRZ,
      this->getDisplayRotationZ(
          oi::value(displayUnits, DimensionType::eAngular,
                    UnitType::eUnitRadiant),
          oi::value(displayDigits, DimensionType::eAngular, 0)));
  parameters.insert_or_assign(
      TrafoParamParameters::eUnknownSX,
      this->getDisplayScaleX(
          oi::value(displayDigits, DimensionType::eDimensionless, 0)));
  parameters.insert_or_assign(
      TrafoParamParameters::eUnknownSY,
      this->getDisplayScaleY(
          oi::value(displayDigits, DimensionType::eDimensionless, 0)));
  parameters.insert_or_assign(
      TrafoParamParameters::eUnknownSZ,
      this->getDisplayScaleZ(
          oi::value(displayDigits, DimensionType::eDimensionless, 0)));

  return parameters;
}

/*!
 * \brief TrafoParam::setUnknownParameters
 * \param parameters
 */
void TrafoParam::setUnknownParameters(
    const std::map<TrafoParamParameters, double>& parameters) {
  // get current parameters
  OiVec translation = this->translation;
  OiVec rotation = this->rotation;
  OiVec scale = this->scale;

  // update parameters
  std::vector<TrafoParamParameters> keys = oi::keys(parameters);
  for (const TrafoParamParameters& key : oi::snapshot(keys)) {
    switch (key) {
      case TrafoParamParameters::eUnknownTX:
        translation.setAt(
            0, oi::value(parameters, TrafoParamParameters::eUnknownTX));
        break;
      case TrafoParamParameters::eUnknownTY:
        translation.setAt(
            1, oi::value(parameters, TrafoParamParameters::eUnknownTY));
        break;
      case TrafoParamParameters::eUnknownTZ:
        translation.setAt(
            2, oi::value(parameters, TrafoParamParameters::eUnknownTZ));
        break;
      case TrafoParamParameters::eUnknownRX:
        rotation.setAt(0,
                       oi::value(parameters, TrafoParamParameters::eUnknownRX));
        break;
      case TrafoParamParameters::eUnknownRY:
        rotation.setAt(1,
                       oi::value(parameters, TrafoParamParameters::eUnknownRY));
        break;
      case TrafoParamParameters::eUnknownRZ:
        rotation.setAt(2,
                       oi::value(parameters, TrafoParamParameters::eUnknownRZ));
        break;
      case TrafoParamParameters::eUnknownSX:
        scale.setAt(0, oi::value(parameters, TrafoParamParameters::eUnknownSX));
        break;
      case TrafoParamParameters::eUnknownSY:
        scale.setAt(1, oi::value(parameters, TrafoParamParameters::eUnknownSY));
        break;
      case TrafoParamParameters::eUnknownSZ:
        scale.setAt(2, oi::value(parameters, TrafoParamParameters::eUnknownSZ));
        break;
    }
  }

  // update trafo param definition
  bool check = this->setTransformationParameters(rotation, translation, scale);
  if (check) {
    this->isSolved = true;
  }
}

/*!
 * \brief TrafoParam::recalc
 */
void TrafoParam::recalc() { Feature::recalc(); }

/*!
 * \brief TrafoParam::getDisplayStDev
 * \param type
 * \param digits
 * \return
 */
std::string TrafoParam::getDisplayStDev(const UnitType& type,
                                        const int& digits) const {
  return oi::number(convertFromDefault(this->statistic.getStdev(), type), 'f',
                    digits);
}

/*!
 * \brief TrafoParam::getDisplayStartSystem
 * \return
 */
std::string TrafoParam::getDisplayStartSystem() const {
  if (!this->from.isNull()) {
    return this->from->getFeatureName();
  }
  return std::string("");
}

/*!
 * \brief TrafoParam::getDisplayDestinationSystem
 * \return
 */
std::string TrafoParam::getDisplayDestinationSystem() const {
  if (!this->to.isNull()) {
    return this->to->getFeatureName();
  }
  return std::string("");
}

/*!
 * \brief TrafoParam::getDisplayTranslationX
 * \param type
 * \param digits
 * \return
 */
std::string TrafoParam::getDisplayTranslationX(const UnitType& type,
                                               const int& digits) const {
  if (this->isSolved) {
    return oi::number(convertFromDefault(this->translation.getAt(0), type), 'f',
                      digits);
  }
  return std::string("");
}

/*!
 * \brief TrafoParam::getDisplayTranslationY
 * \param type
 * \param digits
 * \return
 */
std::string TrafoParam::getDisplayTranslationY(const UnitType& type,
                                               const int& digits) const {
  if (this->isSolved) {
    return oi::number(convertFromDefault(this->translation.getAt(1), type), 'f',
                      digits);
  }
  return std::string("");
}

/*!
 * \brief TrafoParam::getDisplayTranslationZ
 * \param type
 * \param digits
 * \return
 */
std::string TrafoParam::getDisplayTranslationZ(const UnitType& type,
                                               const int& digits) const {
  if (this->isSolved) {
    return oi::number(convertFromDefault(this->translation.getAt(2), type), 'f',
                      digits);
  }
  return std::string("");
}

/*!
 * \brief TrafoParam::getDisplayRotationX
 * \param type
 * \param digits
 * \return
 */
std::string TrafoParam::getDisplayRotationX(const UnitType& type,
                                            const int& digits) const {
  if (this->isSolved) {
    return oi::number(convertFromDefault(this->rotation.getAt(0), type), 'f',
                      digits);
  }
  return std::string("");
}

/*!
 * \brief TrafoParam::getDisplayRotationY
 * \param type
 * \param digits
 * \return
 */
std::string TrafoParam::getDisplayRotationY(const UnitType& type,
                                            const int& digits) const {
  if (this->isSolved) {
    return oi::number(convertFromDefault(this->rotation.getAt(1), type), 'f',
                      digits);
  }
  return std::string("");
}

/*!
 * \brief TrafoParam::getDisplayRotationZ
 * \param type
 * \param digits
 * \return
 */
std::string TrafoParam::getDisplayRotationZ(const UnitType& type,
                                            const int& digits) const {
  if (this->isSolved) {
    return oi::number(convertFromDefault(this->rotation.getAt(2), type), 'f',
                      digits);
  }
  return std::string("");
}

/*!
 * \brief TrafoParam::getDisplayScaleX
 * \param digits
 * \return
 */
std::string TrafoParam::getDisplayScaleX(const int& digits) const {
  if (this->isSolved) {
    return oi::number(this->scale.getAt(0), 'f', digits);
  }
  return std::string("");
}

/*!
 * \brief TrafoParam::getDisplayScaleY
 * \param digits
 * \return
 */
std::string TrafoParam::getDisplayScaleY(const int& digits) const {
  if (this->isSolved) {
    return oi::number(this->scale.getAt(1), 'f', digits);
  }
  return std::string("");
}

/*!
 * \brief TrafoParam::getDisplayScaleZ
 * \param digits
 * \return
 */
std::string TrafoParam::getDisplayScaleZ(const int& digits) const {
  if (this->isSolved) {
    return oi::number(this->scale.getAt(2), 'f', digits);
  }
  return std::string("");
}

/*!
 * \brief TrafoParam::getDisplayIsUsed
 * \return
 */
std::string TrafoParam::getDisplayIsUsed() const {
  return this->isUsed ? "true" : "false";
}

/*!
 * \brief TrafoParam::getDisplayIsDatumTransformation
 * \return
 */
std::string TrafoParam::getDisplayIsDatumTransformation() const {
  return this->isDatumTrafo ? "true" : "false";
}
