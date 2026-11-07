#include "nurbs.h"

#include "featurewrapper.h"

using namespace oi;

/*!
 * \brief Nurbs::Nurbs
 * \param parent
 */
Nurbs::Nurbs(QObject* parent) : Geometry(parent) {
  // set up feature wrapper
  if (!this->selfFeature.isNull()) {
    this->selfFeature->setNurbs(this);
  }
}

/*!
 * \brief Nurbs::Nurbs
 * \param copy
 * \param parent
 */
Nurbs::Nurbs(const Nurbs& copy, QObject* parent) : Geometry(copy, parent) {
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
