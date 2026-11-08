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

#ifndef STR
#define STR(x) #x
#endif
#ifndef STR2
#define STR2(x) STR(x)
#endif

#ifndef CONCAT
#define CONCAT(a, b) a##b
#endif
#ifndef CONCAT2
#define CONCAT2(a, b) CONCAT(a, b)
#endif

#include "plugin_abi.h"

#define EXCHANGE_DEFINEDFORMAT_PREFIX \
  de.openIndy.plugin.exchange.exchangeDefinedFormat.v
#define ExchangeDefinedFormat_iidd \
  STR2(CONCAT2(EXCHANGE_DEFINEDFORMAT_PREFIX, OI_PLUGIN_ABI_VERSION))

#endif  // OI_CORE_PLUGIN_EXCHANGE_EXCHANGE_DEFINED_FORMAT_H_
