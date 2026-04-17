#pragma once

#include <cstdint>

#include "time_types.h"

namespace kairo {

struct KFrameArgs {
  KTimeTicks frame_time;
  KTimeTicks deadline;
  KTimeDelta interval;
  uint64_t sequence = 0;
};

class KFrameObserver {
 public:
  virtual ~KFrameObserver() = default;

  virtual void OnFrame(const KFrameArgs& args) = 0;
};

class KFrameSource {
 public:
  virtual ~KFrameSource() = default;

  virtual void AddObserver(KFrameObserver* observer) = 0;
  virtual void RemoveObserver(KFrameObserver* observer) = 0;
};

}  // namespace kairo
