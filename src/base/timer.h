#pragma once

#include "task_runner.h"

namespace kairo {

class KTimer {
 public:
  virtual ~KTimer() = default;

  virtual void Start(KTimeDelta delay, KTaskFunction task, void* context) = 0;
  virtual void Stop() = 0;
  virtual bool IsRunning() const = 0;
};

}  // namespace kairo
