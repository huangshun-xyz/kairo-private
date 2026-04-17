#pragma once

#include "frame_source.h"
#include "invalidation_sink.h"
#include "task_runner.h"
#include "tick_clock.h"

namespace kairo {

struct KRuntimeHost {
  KTickClock* tick_clock = nullptr;
  KTaskRunner* task_runner = nullptr;
  KFrameSource* frame_source = nullptr;
  KInvalidationSink* invalidation_sink = nullptr;
};

}  // namespace kairo
