#include "element.h"

using namespace oi;

Element::Element(QObject* parent) : QObject(parent) {}

Element::Element(const Element& copy, QObject* parent)
    : QObject(parent), id(copy.id), isNullObject(copy.isNullObject) {}

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
    emit this->elementIdChanged();
    return true;
}*/

/*!
 * null object or invalid object
 * @brief Element::isNull
 * @return
 */
bool Element::isNull() { return isNullObject; }
