#ifndef RADIUS_H
#define RADIUS_H

#include <QObject>

#include "core_export.h"
#include "element.h"

namespace oi {

/*!
 * \brief The Radius class
 */
class OI_CORE_EXPORT Radius : public Element {
  Q_OBJECT

 public:
  explicit Radius(QObject* parent = nullptr);
  explicit Radius(const double& r, QObject* parent = nullptr);

  Radius(const Radius& copy, QObject* parent = nullptr);

  Radius& operator=(const Radius& copy);

  ~Radius() override;

  // #################
  // get or set radius
  // #################

  void setRadius(const double& r);

  [[nodiscard]] const double& getRadius() const;

 protected:
  double radius;
};

}  // namespace oi

#endif  // RADIUS_H
