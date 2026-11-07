#ifndef EXCHANGEOBSERVER_H
#define EXCHANGEOBSERVER_H

#include <string>

#include "core_export.h"

namespace oi {

/*!
 * \brief Receiver of the progress and completion reports of an import/export
 * plugin (ExchangeInterface).
 *
 * Replaces the updateProgress, importFinished and exportFinished Qt signals.
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

  // 0 <= percent <= 100
  virtual void onProgress(int /*percent*/, const std::string& /*message*/) {}
  virtual void onImportFinished(bool /*success*/) {}
  virtual void onExportFinished(bool /*success*/) {}
};

}  // namespace oi

#endif  // EXCHANGEOBSERVER_H
