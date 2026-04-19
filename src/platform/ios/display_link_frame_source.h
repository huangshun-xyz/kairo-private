#pragma once

#include <cstdint>
#include <vector>

#include "frame_source.h"

@class CADisplayLink;
@class KairoDisplayLinkTarget;

namespace kairo {

class DisplayLinkFrameSource final : public KFrameSource {
 public:
  DisplayLinkFrameSource();
  ~DisplayLinkFrameSource() override;

  void AddObserver(KFrameObserver* observer) override;
  void RemoveObserver(KFrameObserver* observer) override;
  void OnDisplayLink(CADisplayLink* display_link);

 private:
  void Start();
  void Stop();

  std::vector<KFrameObserver*> observers_;
  uint64_t sequence_ = 0;
  CADisplayLink* display_link_ = nullptr;
  KairoDisplayLinkTarget* target_ = nullptr;
};

}  // namespace kairo
