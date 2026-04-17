#pragma once

#include "task_runner.h"

namespace kairo {

class MainRunLoopTaskRunner final : public KTaskRunner {
 public:
  MainRunLoopTaskRunner() = default;
  ~MainRunLoopTaskRunner() override = default;

  void PostTask(KTaskFunction task, void* context) override;
  void PostDelayedTask(KTimeDelta delay, KTaskFunction task, void* context) override;
};

}  // namespace kairo
