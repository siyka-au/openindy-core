#include "executor.h"

#include <utility>

namespace oi {

void InlineExecutor::post(std::function<void()> task) {
  if (task) {
    task();
  }
}

InlineExecutor& InlineExecutor::instance() {
  static InlineExecutor executor;
  return executor;
}

}  // namespace oi
