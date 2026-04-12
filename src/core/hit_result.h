#pragma once

#include "include/core/SkPoint.h"

namespace kairo {

struct HitResult {
  SkPoint position = SkPoint::Make(0.0f, 0.0f);
  double logical_x = 0.0;
  double value = 0.0;
};

}  // namespace kairo
