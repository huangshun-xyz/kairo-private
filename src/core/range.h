#pragma once

#include <algorithm>
#include <cmath>

namespace kairo {

struct Range {
  double min = 0.0;
  double max = -1.0;

  Range() = default;
  Range(double minimum, double maximum) : min(minimum), max(maximum) {}

  bool IsValid() const {
    return std::isfinite(min) && std::isfinite(max) && min <= max;
  }

  double span() const {
    return IsValid() ? (max - min) : 0.0;
  }

  void Include(double value) {
    if (!std::isfinite(value)) {
      return;
    }

    if (!IsValid()) {
      min = value;
      max = value;
      return;
    }

    min = std::min(min, value);
    max = std::max(max, value);
  }

  void Include(const Range& range) {
    if (!range.IsValid()) {
      return;
    }

    Include(range.min);
    Include(range.max);
  }

  Range Expanded(double ratio, double minimum_padding = 1.0) const {
    if (!IsValid()) {
      return Range();
    }

    const double current_span = span();
    if (current_span <= 0.0) {
      const double padding = std::max(std::abs(max) * ratio, minimum_padding);
      return Range(min - padding, max + padding);
    }

    const double padding = std::max(current_span * ratio, minimum_padding);
    return Range(min - padding, max + padding);
  }
};

}  // namespace kairo
