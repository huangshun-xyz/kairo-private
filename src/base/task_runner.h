#pragma once

#include "time_types.h"

namespace kairo {

using KTaskFunction = void (*)(void*);

class KTaskRunner {
 public:
  virtual ~KTaskRunner() = default;

  virtual void PostTask(KTaskFunction task, void* context) = 0;
  virtual void PostDelayedTask(KTimeDelta delay, KTaskFunction task, void* context) = 0;
};

}  // namespace kairo
