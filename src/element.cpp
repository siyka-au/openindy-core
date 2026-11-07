#include "element.h"

#include "containers.h"

using namespace oi;

Element::Element() {}

Element::Element(const Element& copy)
    : id(copy.id), isNullObject(copy.isNullObject) {}

Element& Element::operator=(const Element& copy) {
  this->id = copy.id;
  this->isNullObject = copy.isNullObject;
  return *this;
}

Element::~Element() {}

/*!
 * \brief Element::getId
 * \return
 */
int Element::getId() const { return this->id; }

/*!
 * \brief Element::setId
 * \param id
 */
/*bool Element::setId(const int &id){
    this->id = id;
    this->elementIdChanged();
    return true;
}*/

/*!
 * null object or invalid object
 * @brief Element::isNull
 * @return
 */
bool Element::isNull() { return isNullObject; }

void Element::setTransientAttribute(const QString& name,
                                    const QVariant& value) {
  this->transientAttributes.insert_or_assign(name, value);
}

QVariant Element::getTransientAttribute(const QString& name) const {
  return oi::value(this->transientAttributes, name);
}
