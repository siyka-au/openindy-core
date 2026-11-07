#ifndef ELEMENT_H
#define ELEMENT_H

#include <QObject>
#include <QtCore>

#include "core_export.h"
#include "types.h"
#include "util.h"

namespace oi {

class OiJob;

namespace codec {
struct Access;
}  // namespace codec

/*!
 * \brief The Element class
 */
class OI_CORE_EXPORT Element : public QObject {
  friend class OiJob;
  friend struct codec::Access;  // the protobuf codec (see src/codec)
  Q_OBJECT

 public:
  explicit Element(QObject* parent = nullptr);

  Element(const Element& copy, QObject* parent = nullptr);

  Element& operator=(const Element& copy);

  ~Element() override;

  // #############
  // get or set id
  // #############

  [[nodiscard]] int getId() const;

  bool isNull();

 protected:
  int id{-1};
  // indicates null object or invalid object
  bool isNullObject{false};
};

}  // namespace oi

#endif  // ELEMENT_H
