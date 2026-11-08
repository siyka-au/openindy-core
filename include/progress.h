#ifndef OI_CORE_PROGRESS_H_
#define OI_CORE_PROGRESS_H_

#include <algorithm>
#include <cstddef>
#include <exception>
#include <stop_token>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace oi {

/*!
 * \brief A snapshot of how far a long-running operation has got.
 *
 * `fraction` is in [0, 1], or kIndeterminate when the amount of work is not
 * known (a busy indicator rather than a bar). `message` is a short,
 * human-readable description of the current step and may be empty.
 */
struct Progress {
  static constexpr double kIndeterminate = -1.0;

  double fraction = kIndeterminate;
  std::string message;

  [[nodiscard]] bool isDeterminate() const noexcept { return fraction >= 0.0; }
};

/*!
 * \brief Receives progress from an operation. Implemented by the host.
 *
 * A command-line host prints it, the Qt application updates a dialog, a
 * network host sends it to a client. The operation knows nothing about which.
 *
 * onProgress() may be called from any thread, including a worker thread, and
 * must not block for long. A sink that touches a UI has to marshal the call
 * to the UI thread itself. Sinks are free to throttle: operations may report
 * from hot loops.
 */
class ProgressSink {
 public:
  ProgressSink() = default;
  ProgressSink(const ProgressSink&) = delete;
  ProgressSink& operator=(const ProgressSink&) = delete;
  ProgressSink(ProgressSink&&) = delete;
  ProgressSink& operator=(ProgressSink&&) = delete;
  virtual ~ProgressSink() = default;

  virtual void onProgress(const Progress& progress) = 0;
};

/*!
 * \brief Thrown by OperationContext::throwIfCancelled() when the host has
 * asked the operation to stop. Operations let it propagate; the host treats
 * it as "cancelled by the user", not as a failure.
 */
class OperationCancelled : public std::exception {
 public:
  [[nodiscard]] const char* what() const noexcept override {
    return "operation cancelled";
  }
};

/*!
 * \brief What a long-running operation is given so it can report progress and
 * notice a cancellation request.
 *
 * Pass it by const reference as the last parameter of any operation that can
 * take noticeable time (fits, bundle adjustment, import/export, simulation,
 * segmentation). A default-constructed context has no sink and can never be
 * cancelled, so callers that do not care pass `{}`.
 *
 * Cancellation is cooperative: the operation polls cancelRequested() (or calls
 * throwIfCancelled()) at points where it can stop cleanly. It is never forced.
 *
 * Nested operations use subrange() so that a child reporting 0..1 fills only
 * its slice of the parent's bar.
 */
class OperationContext {
 public:
  OperationContext() = default;

  OperationContext(ProgressSink* sink, std::stop_token stop = {})
      : sink_(sink), stop_(std::move(stop)) {}

  /*! Report an absolute fraction (clamped to [0, 1]) with an optional note. */
  void report(double fraction, std::string_view message = {}) const {
    if (sink_ == nullptr) {
      return;
    }
    const double clamped = std::clamp(fraction, 0.0, 1.0);
    sink_->onProgress(
        Progress{begin_ + (end_ - begin_) * clamped, std::string(message)});
  }

  /*! Report `done` of `total` steps. total == 0 reports as complete. */
  void report(std::size_t done, std::size_t total,
              std::string_view message = {}) const {
    report(total == 0 ? 1.0
                      : static_cast<double>(done) / static_cast<double>(total),
           message);
  }

  /*! Report that work is happening but how much is unknown. */
  void reportIndeterminate(std::string_view message = {}) const {
    if (sink_ != nullptr) {
      sink_->onProgress(
          Progress{Progress::kIndeterminate, std::string(message)});
    }
  }

  [[nodiscard]] bool cancelRequested() const noexcept {
    return stop_.stop_requested();
  }

  /*! Throws OperationCancelled if the host asked the operation to stop. */
  void throwIfCancelled() const {
    if (cancelRequested()) {
      throw OperationCancelled();
    }
  }

  /*!
   * A context for a child operation whose own 0..1 range maps onto
   * [begin, end] of this one. Cancellation is shared with the parent.
   */
  [[nodiscard]] OperationContext subrange(double begin, double end) const {
    OperationContext child = *this;
    const double lo = std::clamp(begin, 0.0, 1.0);
    const double hi = std::clamp(end, lo, 1.0);
    child.end_ = begin_ + (end_ - begin_) * hi;
    child.begin_ = begin_ + (end_ - begin_) * lo;
    return child;
  }

 private:
  ProgressSink* sink_ = nullptr;  // non-owning; the host outlives the operation
  std::stop_token stop_;
  double begin_ = 0.0;
  double end_ = 1.0;
};

/*!
 * \brief Splits an operation into named, weighted steps and keeps the overall
 * fraction consistent by construction, so callers never do the arithmetic.
 *
 *     ProgressScope scope(ctx, "Bundle adjustment",
 *                         {{"setup", 1}, {"iterate", 8}, {"report", 1}});
 *     setup();
 *     scope.next();               // -> "iterate" (10% .. 90% of ctx)
 *     iterate(scope.child());     // child fills exactly that slice
 *     scope.next();               // -> "report"
 *     report();
 *     scope.next();               // -> complete (reports 100%)
 *
 * Weights are relative: only their ratios matter. Non-positive weights count
 * as zero; if every weight is zero the steps share the range equally.
 * Reports the start of the first step on construction and the start of each
 * following step in next(). Nothing is reported on destruction, so an
 * exception or cancellation leaves the bar where the work actually got to.
 */
class ProgressScope {
 public:
  struct Step {
    std::string name;
    double weight = 1.0;
  };

  ProgressScope(const OperationContext& parent, std::string name,
                std::vector<Step> steps)
      : parent_(parent), name_(std::move(name)), steps_(std::move(steps)) {
    computeBounds();
    finished_ = steps_.empty();
    if (!finished_) {
      reportStart();
    }
  }

  /*! `count` unnamed steps of equal weight. */
  ProgressScope(const OperationContext& parent, std::string name,
                std::size_t count)
      : ProgressScope(parent, std::move(name), std::vector<Step>(count)) {}

  /*! Index of the step in progress; steps().size() once finished. */
  [[nodiscard]] std::size_t currentStep() const noexcept { return index_; }
  [[nodiscard]] bool finished() const noexcept { return finished_; }

  /*! The context for the current step: a child fills only that step's slice. */
  [[nodiscard]] OperationContext child() const {
    if (finished_) {
      return parent_.subrange(1.0, 1.0);
    }
    return parent_.subrange(bounds_[index_], bounds_[index_ + 1]);
  }

  /*!
   * Finish the current step and start the next. Finishing the last step
   * reports 100%. This is a cancellation point: it throws OperationCancelled
   * if the host asked the operation to stop. Calling it when already
   * finished does nothing.
   */
  void next() {
    parent_.throwIfCancelled();
    if (finished_) {
      return;
    }
    ++index_;
    if (index_ >= steps_.size()) {
      finished_ = true;
      parent_.report(1.0, name_);
      return;
    }
    reportStart();
  }

 private:
  void computeBounds() {
    double total = 0.0;
    for (const Step& step : steps_) {
      total += std::max(step.weight, 0.0);
    }
    const bool equal = total <= 0.0;
    const double n = static_cast<double>(steps_.size());
    bounds_.assign(1, 0.0);
    double acc = 0.0;
    for (const Step& step : steps_) {
      acc += equal ? 1.0 / n : std::max(step.weight, 0.0) / total;
      bounds_.push_back(std::min(acc, 1.0));
    }
    if (!bounds_.empty()) {
      bounds_.back() = 1.0;  // no rounding drift at the end
    }
  }

  void reportStart() const {
    const std::string& step = steps_[index_].name;
    if (name_.empty()) {
      parent_.report(bounds_[index_], step);
    } else if (step.empty()) {
      parent_.report(bounds_[index_], name_);
    } else {
      parent_.report(bounds_[index_], name_ + ": " + step);
    }
  }

  OperationContext parent_;
  std::string name_;
  std::vector<Step> steps_;
  std::vector<double> bounds_;  // steps_.size() + 1 cumulative boundaries
  std::size_t index_ = 0;
  bool finished_ = false;
};

}  // namespace oi

#endif  // OI_CORE_PROGRESS_H_
