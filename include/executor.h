#ifndef EXECUTOR_H
#define EXECUTOR_H

#include <functional>

#include "core_export.h"

namespace oi {

/*!
 * \brief Hands a piece of work to "the owner thread" without core needing an
 * event loop.
 *
 * Sensor results are produced on the sensor worker thread; the host decides on
 * which thread they are consumed by giving the stations an Executor. A GUI
 * posts to its UI thread (the Qt application uses a QObject-based executor
 * that queues onto the UI event loop), a CLI or a test can use the
 * InlineExecutor and consume the results straight on the worker thread.
 *
 * Contract for implementations:
 *  - post() may be called from any thread, concurrently.
 *  - tasks posted by one thread run in the order they were posted (the
 *    per-station ordering of sensor results depends on this).
 *  - post() must not run the task on a thread that holds a core lock other
 *    than by design (only the InlineExecutor runs it synchronously).
 *  - the executor must outlive every object it was given to, or those objects
 *    must be destroyed first; tasks still queued when an executor is destroyed
 *    may simply be dropped.
 */
class OI_CORE_EXPORT Executor {
 public:
  Executor() = default;
  virtual ~Executor() = default;

  Executor(const Executor&) = delete;
  Executor& operator=(const Executor&) = delete;
  Executor(Executor&&) = delete;
  Executor& operator=(Executor&&) = delete;

  virtual void post(std::function<void()> task) = 0;
};

/*!
 * \brief Runs every task immediately on the posting thread. The default
 * executor: with it, sensor results are delivered on the sensor worker thread
 * itself.
 */
class OI_CORE_EXPORT InlineExecutor final : public Executor {
 public:
  void post(std::function<void()> task) override;

  // process-wide instance used wherever no executor was set
  static InlineExecutor& instance();
};

}  // namespace oi

#endif  // EXECUTOR_H
