#pragma once

namespace kairo {

class KInvalidationSink {
 public:
  virtual ~KInvalidationSink() = default;

  virtual void RequestRedraw() = 0;
};

}  // namespace kairo
