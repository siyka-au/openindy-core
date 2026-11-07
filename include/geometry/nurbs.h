#ifndef NURBS_H
#define NURBS_H

#include <QObject>
#include <QtCore>

#include "core_export.h"
#include "geometry.h"

namespace oi {

/*!
 * \brief The Nurbs class
 */
class OI_CORE_EXPORT Nurbs : public Geometry {
  Q_OBJECT

 public:
  explicit Nurbs(QObject* parent = nullptr);

  Nurbs(const Nurbs& copy, QObject* parent = nullptr);

  Nurbs& operator=(const Nurbs& copy);

  ~Nurbs() override;

  // ########################################
  // order of unknown parameters (Qxx-matrix)
  // ########################################

  enum NurbsUnknowns {

  };

  // ###################################################################
  // check wether this geometry has a direction, a position and a radius
  // ###################################################################

  // ###########################
  // get or set nurbs parameters
  // ###########################

  // ###########################
  // reexecute the function list
  // ###########################

  void recalc() override;
};

}  // namespace oi

#endif  // NURBS_H
