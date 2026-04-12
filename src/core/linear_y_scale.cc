#include "linear_y_scale.h"

namespace kairo {

LinearYScale::LinearYScale() = default;

void LinearYScale::SetRange(const Range& range) {
  range_ = range;
}

const Range& LinearYScale::range() const {
  return range_;
}

void LinearYScale::SetBounds(const SkRect& bounds) {
  bounds_ = bounds;
}

const SkRect& LinearYScale::bounds() const {
  return bounds_;
}

float LinearYScale::ValueToScreen(double value) const {
  if (!range_.IsValid() || bounds_.height() <= 0.0f) {
    return bounds_.centerY();
  }

  const double span = range_.span();
  if (span <= 0.0) {
    return bounds_.centerY();
  }

  const double ratio = (range_.max - value) / span;
  return bounds_.top() + static_cast<float>(ratio * bounds_.height());
}

double LinearYScale::ScreenToValue(float screen_y) const {
  if (!range_.IsValid() || bounds_.height() <= 0.0f) {
    return range_.max;
  }

  const double ratio = (screen_y - bounds_.top()) / bounds_.height();
  return range_.max - (ratio * range_.span());
}

}  // namespace kairo
