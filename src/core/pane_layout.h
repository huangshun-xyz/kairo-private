#pragma once

namespace kairo {

struct Insets {
  float left = 0.0f;
  float top = 0.0f;
  float right = 0.0f;
  float bottom = 0.0f;
};

enum class PaneSizeMode {
  kFixed,
  kStretch,
};

struct PaneLayout {
  PaneSizeMode size_mode = PaneSizeMode::kStretch;
  float height = 0.0f;
  Insets insets;
};

}  // namespace kairo
