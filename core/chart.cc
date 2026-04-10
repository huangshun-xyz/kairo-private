#include "chart.h"

#include <algorithm>
#include <utility>

namespace kairo::core {

Chart::Chart() = default;

Chart::Chart(std::vector<BarEntry> entries) : entries_(std::move(entries)) {}

Chart Chart::Demo() {
  return Chart({
      {18.0f},
      {34.0f},
      {51.0f},
      {43.0f},
      {62.0f},
  });
}

const std::vector<BarEntry>& Chart::entries() const {
  return entries_;
}

float Chart::max_value() const {
  if (entries_.empty()) {
    return 1.0f;
  }

  const auto max_it = std::max_element(
      entries_.begin(),
      entries_.end(),
      [](const BarEntry& lhs, const BarEntry& rhs) {
        return lhs.value < rhs.value;
      });
  return max_it->value;
}

}  // namespace kairo::core
