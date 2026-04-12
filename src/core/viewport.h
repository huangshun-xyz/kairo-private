#pragma once

namespace kairo {

struct Viewport {
  double visible_from = 0.0;
  double visible_to = 1.0;

  bool IsValid() const {
    return visible_to > visible_from;
  }

  double width() const {
    return IsValid() ? (visible_to - visible_from) : 0.0;
  }
};

}  // namespace kairo
