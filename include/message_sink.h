#ifndef OI_CORE_MESSAGE_SINK_H_
#define OI_CORE_MESSAGE_SINK_H_

#include <string>

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

  virtual void onMessage(const std::string& msg, const MessageTypes& type,
                         const MessageDestinations& dest) = 0;

  // A Function that has this sink (its job) changed something that is saved
  // with the project: an input element, its use state, a pipeline, a fixed
  // parameter. Nothing by default. The function has no feature id to report;
  // the host only learns that the project's content changed.
  virtual void onOwnedObjectChanged() {}
};

}  // namespace oi

#endif  // OI_CORE_MESSAGE_SINK_H_
