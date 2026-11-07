#ifndef OI_GUARDED_H
#define OI_GUARDED_H

#include <atomic>
#include <cstddef>
#include <memory>
#include <mutex>
#include <type_traits>
#include <utility>

namespace oi {

class Trackable;

namespace detail {

// shared between a Trackable and every Guarded that points at it; outlives
// the Trackable as long as a Guarded still holds it
struct TrackControl {
  std::atomic<Trackable*> target;
  explicit TrackControl(Trackable* t) : target(t) {}
};

}  // namespace detail

/*!
 * \brief Base class of every object that may be referred to through a
 * Guarded<T> (the std-only replacement for QObject + QPointer).
 *
 * The first Guarded created for an object allocates a small control block;
 * destroying the object clears it, so all Guarded copies read null from then
 * on. A copy of a Trackable is a new object: it does not inherit the control
 * block of its source, and assigning to one leaves its identity untouched.
 * Creating Guarded pointers and reading them is thread safe, as is
 * destroying the target on another thread; as with QPointer, a raw pointer
 * obtained from data() is only valid while the target lives.
 */
class Trackable {
 public:
  Trackable() = default;
  Trackable(const Trackable&) {}
  Trackable& operator=(const Trackable&) { return *this; }
  virtual ~Trackable() {
    if (control_) control_->target.store(nullptr, std::memory_order_release);
  }

  // used by Guarded; allocates the control block on first use
  [[nodiscard]] std::shared_ptr<detail::TrackControl> trackControl() const {
    std::lock_guard<std::mutex> lock(controlMutex());
    if (!control_)
      control_ =
          std::make_shared<detail::TrackControl>(const_cast<Trackable*>(this));
    return control_;
  }

 private:
  static std::mutex& controlMutex() {
    static std::mutex m;
    return m;
  }
  mutable std::shared_ptr<detail::TrackControl> control_;
};

/*!
 * \brief Non-owning pointer to a Trackable-derived T that becomes null when
 * the target is destroyed. Behaves like oi::Guarded<T>.
 */
template <class T>
class Guarded {
 public:
  Guarded() = default;
  Guarded(std::nullptr_t) {}
  Guarded(T* p) { assign(p); }
  template <class U, class = std::enable_if_t<std::is_convertible_v<U*, T*>>>
  Guarded(const Guarded<U>& other) : control_(other.control_) {}

  Guarded(const Guarded&) = default;
  Guarded(Guarded&&) noexcept = default;
  Guarded& operator=(const Guarded&) = default;
  Guarded& operator=(Guarded&&) noexcept = default;
  Guarded& operator=(T* p) {
    assign(p);
    return *this;
  }
  template <class U, class = std::enable_if_t<std::is_convertible_v<U*, T*>>>
  Guarded& operator=(const Guarded<U>& other) {
    control_ = other.control_;
    return *this;
  }
  Guarded& operator=(std::nullptr_t) {
    control_.reset();
    return *this;
  }

  [[nodiscard]] T* data() const {
    if (!control_) return nullptr;
    // upcast at assign time, downcast here; T is complete wherever this is
    // instantiated
    return static_cast<T*>(control_->target.load(std::memory_order_acquire));
  }
  [[nodiscard]] bool isNull() const { return data() == nullptr; }
  void clear() { control_.reset(); }

  T* operator->() const { return data(); }
  T& operator*() const { return *data(); }
  operator T*() const { return data(); }

 private:
  template <class U>
  friend class Guarded;

  void assign(T* p) {
    if (p)
      control_ = static_cast<const Trackable*>(p)->trackControl();
    else
      control_.reset();
  }

  std::shared_ptr<detail::TrackControl> control_;
};

}  // namespace oi

#endif  // OI_GUARDED_H
