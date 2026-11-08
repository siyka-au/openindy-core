#ifndef OI_CORE_ELEMENT_H_
#define OI_CORE_ELEMENT_H_
#include <map>
#include <string>

#include "core_export.h"
#include "guarded.h"
#include "types.h"
#include "util.h"
#include "value.h"

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

  // #####################################################################
  // transient attributes: free-form values one party attaches to an object
  // for another to pick up (replaces the dynamic properties of QObject).
  // Not persisted and not copied with the object.
  // #####################################################################

  void setTransientAttribute(const std::string& name, const oi::Value& value);
  [[nodiscard]] oi::Value getTransientAttribute(const std::string& name) const;

 protected:
  int id{-1};
  // indicates null object or invalid object
  bool isNullObject{false};

 private:
  oi::ValueMap transientAttributes;
};

}  // namespace oi

#endif  // OI_CORE_ELEMENT_H_
