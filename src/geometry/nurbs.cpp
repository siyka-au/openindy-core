#include "nurbs.h"

#include "feature_wrapper.h"

using namespace oi;

/*!
 * \brief Nurbs::Nurbs
 */
Nurbs::Nurbs() : Geometry() {
  // set up feature wrapper
  if (!this->selfFeature.isNull()) {
    this->selfFeature->setNurbs(this);
  }
}

/*!
 * \brief Nurbs::Nurbs
 * \param copy
 */
Nurbs::Nurbs(const Nurbs& copy) : Geometry(copy) {
  // set up feature wrapper
  if (!this->selfFeature.isNull()) {
    this->selfFeature->setNurbs(this);
  }
}

/*!
 * \brief Nurbs::operator =
 * \param copy
 * \return
 */
Nurbs& Nurbs::operator=(const Nurbs& /*copy*/) {
  // set up feature wrapper
  if (!this->selfFeature.isNull()) {
    this->selfFeature->setNurbs(this);
  }

  return *this;
}

/*!
 * \brief Nurbs::~Nurbs
 */
Nurbs::~Nurbs() {}

/*!
 * \brief Nurbs::recalc
 */
void Nurbs::recalc() { Geometry::recalc(); }
