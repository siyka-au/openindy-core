#include "exchangedefinedformat.h"

#include <vector>

using namespace oi;

/*!
 * \brief ExchangeDefinedFormat::ExchangeDefinedFormat
 */
ExchangeDefinedFormat::ExchangeDefinedFormat() : ExchangeInterface() {}

/*!
 * \brief ExchangeDefinedFormat::~ExchangeDefinedFormat
 */
ExchangeDefinedFormat::~ExchangeDefinedFormat() {}

/*!
 * \brief ExchangeDefinedFormat::getSupportedFileExtensions
 * \return
 */
const std::vector<QString>& ExchangeDefinedFormat::getSupportedFileExtensions()
    const {
  return this->supportedFileExtensions;
}

/*!
 * \brief ExchangeDefinedFormat::getUsedElements
 * \return
 */
const std::vector<ElementTypes>& ExchangeDefinedFormat::getUsedElements()
    const {
  return this->usedElementTypes;
}

/*!
 * \brief ExchangeDefinedFormat::setUsedElements
 * Specify the types of elements that shall be im- or exported
 * \param usedElementTypes
 */
void ExchangeDefinedFormat::setUsedElements(
    const std::vector<ElementTypes>& usedElementTypes) {
  this->usedElementTypes = usedElementTypes;
}
