#pragma once

#include <cstdint>

namespace kairo {

struct KTimeDelta {
  int64_t micros = 0;
};

struct KTimeTicks {
  int64_t micros = 0;
};

inline KTimeDelta KTimeDeltaFromSeconds(double seconds) {
  return KTimeDelta {static_cast<int64_t>(seconds * 1000000.0)};
}

inline KTimeTicks KTimeTicksFromSeconds(double seconds) {
  return KTimeTicks {static_cast<int64_t>(seconds * 1000000.0)};
}

inline double ToSeconds(KTimeDelta delta) {
  return static_cast<double>(delta.micros) / 1000000.0;
}

inline KTimeDelta operator-(KTimeTicks end, KTimeTicks start) {
  return KTimeDelta {end.micros - start.micros};
}

}  // namespace kairo
