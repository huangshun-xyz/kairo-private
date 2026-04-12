#pragma once

#include <memory>

namespace kairo {

class Chart;

std::unique_ptr<Chart> CreateDemoChart();

}  // namespace kairo
