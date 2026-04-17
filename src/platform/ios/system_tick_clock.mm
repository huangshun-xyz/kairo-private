#include "system_tick_clock.h"

#import <QuartzCore/CAMediaTiming.h>

#include "time_types.h"

namespace kairo {

KTimeTicks SystemTickClock::NowTicks() const {
  return KTimeTicksFromSeconds(CACurrentMediaTime());
}

}  // namespace kairo
