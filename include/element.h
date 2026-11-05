#ifndef ELEMENT_H
#define ELEMENT_H

#include <QObject>
#include <QtXml>

#include "types.h"
#include "util.h"

namespace oi {

class OiJob;

/*!
 * \brief The Element class
 */
class OI_CORE_EXPORT Element : public QObject {
  friend class OiJob;
  Q_OBJECT

 public:
  explicit Element(QObject* parent = nullptr);

  Element(const Element& copy, QObject* parent = nullptr);

  Element& operator=(const Element& copy);

  ~Element() override;

  // #############
  // get or set id
  // #############

  int getId() const;

  // ################
  // save or load XML
  // ################

  virtual QDomElement toOpenIndyXML(QDomDocument& xmlDoc) const;
  virtual bool fromOpenIndyXML(QDomElement& xmlElem);

  bool isNull();

 signals:

  // #######################################
  // signals to inform about element changes
  // #######################################

  void elementAboutToBeDeleted(const int& elementId, const QString& name,
                               const QString& group, const FeatureTypes& type);

 protected:
  int id;
  // indicates null object or invalid object
  bool isNullObject;
};

}  // namespace oi

#endif  // ELEMENT_H
