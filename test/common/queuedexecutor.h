#ifndef QUEUEDEXECUTOR_H
#define QUEUEDEXECUTOR_H

#include <functional>
#include <utility>
#include <vector>

#include "executor.h"

// test executor that holds the posted tasks until flush() (the stand-in for
// an event loop)
class QueuedExecutor final : public oi::Executor {
 public:
  void post(std::function<void()> task) override {
    tasks.push_back(std::move(task));
  }
  void flush() {
    auto pending = std::move(tasks);
    tasks.clear();
    for (auto& t : pending) t();
  }
  std::vector<std::function<void()>> tasks;
};

#endif  // QUEUEDEXECUTOR_H
