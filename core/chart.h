#pragma once

#include <vector>

namespace kairo::core {

struct BarEntry {
  float value = 0.0f;
};

class Chart {
 public:
  Chart();
  explicit Chart(std::vector<BarEntry> entries);

  static Chart Demo();

  const std::vector<BarEntry>& entries() const;
  float max_value() const;

 private:
  std::vector<BarEntry> entries_;
};

}  // namespace kairo::core
