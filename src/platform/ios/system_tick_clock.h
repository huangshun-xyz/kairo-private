#pragma once

#include "tick_clock.h"

namespace kairo {

class SystemTickClock final : public KTickClock {
 public:
  SystemTickClock() = default;
  ~SystemTickClock() override = default;

  KTimeTicks NowTicks() const override;
};

}  // namespace kairo
