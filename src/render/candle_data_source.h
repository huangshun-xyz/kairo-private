#pragma once

#include <cstddef>
#include <utility>
#include <vector>

namespace kairo {

struct CandleData {
  double open = 0.0;
  double high = 0.0;
  double low = 0.0;
  double close = 0.0;
  double volume = 0.0;
};

class ICandleDataSource {
 public:
  virtual ~ICandleDataSource() = default;

  virtual size_t GetCount() const = 0;
  virtual bool GetCandle(size_t index, CandleData* out) const = 0;
};

class VectorCandleDataSource final : public ICandleDataSource {
 public:
  VectorCandleDataSource() = default;
  explicit VectorCandleDataSource(std::vector<CandleData> candles)
      : candles_(std::move(candles)) {}

  size_t GetCount() const override {
    return candles_.size();
  }

  bool GetCandle(size_t index, CandleData* out) const override {
    if (index >= candles_.size() || out == nullptr) {
      return false;
    }

    *out = candles_[index];
    return true;
  }

  const std::vector<CandleData>& candles() const {
    return candles_;
  }

 private:
  std::vector<CandleData> candles_;
};

}  // namespace kairo
