#ifndef OI_CORE_PLUGIN_FUNCTION_FUNCTION_OBSERVER_H_
#define OI_CORE_PLUGIN_FUNCTION_FUNCTION_OBSERVER_H_

#include "core_export.h"

namespace oi {

class Function;

/*!
 * \brief Receiver of the change notifications of a Function.
 *
 * Replaces the inputElementsChanged and scalarInputParametersChanged Qt
 * signals. A function holds a plain, non-owning pointer to at most one
 * observer (Function::setObserver); every method does nothing by default.
 * Notifications are synchronous, on the thread that changed the function.
 *
 * ~Function calls onFunctionDestroyed first; the observer must not use the
 * function afterwards and should drop its reference to it.
 */
class OI_CORE_EXPORT FunctionObserver {
 public:
  FunctionObserver() = default;
  virtual ~FunctionObserver() = default;

  FunctionObserver(const FunctionObserver&) = delete;
  FunctionObserver& operator=(const FunctionObserver&) = delete;
  FunctionObserver(FunctionObserver&&) = delete;
  FunctionObserver& operator=(FunctionObserver&&) = delete;

  virtual void onFunctionDestroyed(Function* /*function*/) {}
  virtual void onInputElementsChanged() {}
  virtual void onScalarInputParametersChanged() {}
};

}  // namespace oi

#endif  // OI_CORE_PLUGIN_FUNCTION_FUNCTION_OBSERVER_H_
