#pragma once

#include "time_types.h"

namespace kairo {

class KTickClock {
 public:
  virtual ~KTickClock() = default;

  virtual KTimeTicks NowTicks() const = 0;
};

}  // namespace kairo
