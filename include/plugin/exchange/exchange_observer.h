#ifndef OI_CORE_PLUGIN_EXCHANGE_EXCHANGE_OBSERVER_H_
#define OI_CORE_PLUGIN_EXCHANGE_EXCHANGE_OBSERVER_H_

#include <string>

#include "core_export.h"

namespace oi {

/*!
 * \brief Receiver of the completion reports of an import/export
 * plugin (ExchangeInterface).
 *
 * Replaces the importFinished and exportFinished Qt signals. Progress is not
 * reported here: it goes through the oi::OperationContext given to
 * importOiData/exportOiData (see progress.h).
 * The host sets it with ExchangeInterface::setObserver; every method does
 * nothing by default.
 *
 * Exchanges usually run on a worker thread and call the observer from there,
 * synchronously. An observer that feeds a UI must marshal to the UI thread
 * itself (the application's DataExchanger does).
 */
class OI_CORE_EXPORT ExchangeObserver {
 public:
  ExchangeObserver() = default;
  virtual ~ExchangeObserver() = default;

  ExchangeObserver(const ExchangeObserver&) = delete;
  ExchangeObserver& operator=(const ExchangeObserver&) = delete;
  ExchangeObserver(ExchangeObserver&&) = delete;
  ExchangeObserver& operator=(ExchangeObserver&&) = delete;

  virtual void onImportFinished(bool /*success*/) {}
  virtual void onExportFinished(bool /*success*/) {}
};

}  // namespace oi

#endif  // OI_CORE_PLUGIN_EXCHANGE_EXCHANGE_OBSERVER_H_
