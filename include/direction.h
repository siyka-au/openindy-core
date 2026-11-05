#ifndef DIRECTION_H
#define DIRECTION_H

#include <QObject>

#include "element.h"
#include "oivec.h"

namespace oi {

using namespace math;

class OI_CORE_EXPORT Direction : public Element {
  Q_OBJECT

 public:
  explicit Direction(QObject* parent = nullptr);
  explicit Direction(const OiVec& v, QObject* parent = nullptr);
  explicit Direction(const double& x, const double& y, const double& z,
                     const double& h = 1.0, QObject* parent = nullptr);

  Direction(const Direction& copy, QObject* parent = nullptr);

  Direction& operator=(const Direction& copy);

  ~Direction() override;

  // ###########################
  // get or set direction vector
  // ###########################

  void setVector(const double& x, const double& y, const double& z,
                 const double& h = 1.0);
  void setVector(const OiVec& v);

  [[nodiscard]] const OiVec& getVector() const;
  [[nodiscard]] const OiVec& getVectorH() const;

 private:
  OiVec ijk;   // vector of size 3 (i, j, k)
  OiVec ijkH;  // vector of size 4 (i, j, k, h)
};

}  // namespace oi

#endif  // DIRECTION_H
