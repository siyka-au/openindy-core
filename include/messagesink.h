#ifndef MESSAGESINK_H
#define MESSAGESINK_H

#include <QString>

#include "core_export.h"
#include "types.h"

namespace oi {

/*!
 * \brief Receiver of the user-facing messages that plugin-facing worker
 * objects (Function, BundleAdjustment, ExchangeInterface, SimulationModel)
 * produce while they work.
 *
 * Replaces their former sendMessage Qt signals. Such an object holds a plain,
 * non-owning pointer to at most one sink and forwards every message to it
 * synchronously, on the calling thread; without a sink the message is dropped.
 * A sink that is handed messages from a worker thread and touches a UI has to
 * marshal them to that UI's thread itself.
 *
 * The sink must outlive the objects it is set on, or be removed from them
 * (setMessageSink(nullptr)) first - OiJob does so for everything it owns when
 * the object is detached and when the job is destroyed.
 */
class OI_CORE_EXPORT MessageSink {
 public:
  MessageSink() = default;
  virtual ~MessageSink() = default;

  MessageSink(const MessageSink&) = delete;
  MessageSink& operator=(const MessageSink&) = delete;
  MessageSink(MessageSink&&) = delete;
  MessageSink& operator=(MessageSink&&) = delete;

  virtual void onMessage(const QString& msg, const MessageTypes& type,
                         const MessageDestinations& dest) = 0;
};

}  // namespace oi

#endif  // MESSAGESINK_H
