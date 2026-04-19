#pragma once

#include "frame_source.h"
#include "invalidation_sink.h"

namespace kairo {

struct KRuntimeHost {
  KFrameSource* frame_source = nullptr;
  KInvalidationSink* invalidation_sink = nullptr;
};

}  // namespace kairo
