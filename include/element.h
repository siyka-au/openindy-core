#ifndef ELEMENT_H
#define ELEMENT_H

#include <QtCore>

#include "core_export.h"
#include "guarded.h"
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
class OI_CORE_EXPORT Element : public oi::Trackable {
  friend class OiJob;
  friend struct codec::Access;  // the protobuf codec (see src/codec)

 public:
  explicit Element();

  Element(const Element& copy);

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
