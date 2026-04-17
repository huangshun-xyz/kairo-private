#pragma once

#include <memory>

#include "frame_source.h"

namespace kairo {

class DisplayLinkFrameSource final : public KFrameSource {
 public:
  DisplayLinkFrameSource();
  ~DisplayLinkFrameSource() override;

  void AddObserver(KFrameObserver* observer) override;
  void RemoveObserver(KFrameObserver* observer) override;

 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

}  // namespace kairo
