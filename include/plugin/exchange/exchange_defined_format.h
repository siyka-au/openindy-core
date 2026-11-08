#ifndef OI_CORE_PLUGIN_EXCHANGE_EXCHANGE_DEFINED_FORMAT_H_
#define OI_CORE_PLUGIN_EXCHANGE_EXCHANGE_DEFINED_FORMAT_H_
#include <vector>

#include "core_export.h"
#include "exchange_interface.h"

namespace oi {

/*!
 * \brief The OiExchangeDefinedFormat class
 * Interface that shall be used for files with a special standardized format
 * (like *.obj or *.pts etc.)
 */
class OI_CORE_EXPORT ExchangeDefinedFormat : public ExchangeInterface {
 public:
  explicit ExchangeDefinedFormat();

  ~ExchangeDefinedFormat() override;

  // #########################################
  // methods to get or set further information
  // #########################################

  [[nodiscard]] const std::vector<std::string>& getSupportedFileExtensions()
      const;

  [[nodiscard]] const std::vector<ElementTypes>& getUsedElements() const;
  void setUsedElements(const std::vector<ElementTypes>& usedElementTypes);

 protected:
  // ###########################
  // input and output parameters
  // ###########################

  std::vector<ElementTypes>
      usedElementTypes;  // types of elements that shall be im- or exported

  std::vector<std::string> supportedFileExtensions;
};

}  // namespace oi

#include "plugin_abi.h"

inline const std::string ExchangeDefinedFormat_iidd =
    oi::pluginIid("de.openIndy.plugin.exchange.exchangeDefinedFormat.v");

#endif  // OI_CORE_PLUGIN_EXCHANGE_EXCHANGE_DEFINED_FORMAT_H_
