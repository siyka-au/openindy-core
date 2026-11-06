#ifndef POSITION_H
#define POSITION_H

#include <QObject>

#include "core_export.h"
#include "element.h"
#include "oivec.h"

namespace oi {

using namespace math;

/*!
 * \brief The Position class
 */
class OI_CORE_EXPORT Position : public Element {
  Q_OBJECT

 public:
  explicit Position(QObject* parent = nullptr);
  explicit Position(bool isNullObject, QObject* parent = nullptr);
  explicit Position(const OiVec& v, QObject* parent = nullptr);
  explicit Position(const double& x, const double& y, const double& z,
                    const double& h = 1.0, QObject* parent = nullptr);

  Position(const Position& copy, QObject* parent = nullptr);

  Position& operator=(const Position& copy);

  ~Position() override;

  // ##########################
  // get or set position vector
  // ##########################

  void setVector(const OiVec& v);
  void setVector(const double& x, const double& y, const double& z,
                 const double& h = 1.0);

  [[nodiscard]] const OiVec& getVector() const;
  [[nodiscard]] const OiVec& getVectorH() const;

  const static Position NullObject;

 protected:
  OiVec xyz;   // vector of size 3 (x, y, z)
  OiVec xyzH;  // homogeneous vector of size 4 (x, y, z, h)
};

}  // namespace oi

#endif  // POSITION_H
