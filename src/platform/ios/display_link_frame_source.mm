#include "display_link_frame_source.h"

#include <algorithm>
#include <vector>

#import <Foundation/Foundation.h>
#import <QuartzCore/CADisplayLink.h>

#include "time_types.h"

namespace kairo {
class DisplayLinkFrameSourceImpl;
}  // namespace kairo

@interface KairoDisplayLinkTarget : NSObject

@property(nonatomic, assign) kairo::DisplayLinkFrameSourceImpl* owner;

- (void)onDisplayLink:(CADisplayLink*)displayLink;

@end

namespace kairo {

class DisplayLinkFrameSourceImpl {
 public:
  DisplayLinkFrameSourceImpl() {
    target_ = [[KairoDisplayLinkTarget alloc] init];
    target_.owner = this;
  }

  ~DisplayLinkFrameSourceImpl() {
    Stop();
    target_.owner = nullptr;
  }

  void AddObserver(KFrameObserver* observer) {
    if (observer == nullptr ||
        std::find(observers_.begin(), observers_.end(), observer) != observers_.end()) {
      return;
    }

    observers_.push_back(observer);
    if (observers_.size() == 1u) {
      Start();
    }
  }

  void RemoveObserver(KFrameObserver* observer) {
    observers_.erase(std::remove(observers_.begin(), observers_.end(), observer), observers_.end());
    if (observers_.empty()) {
      Stop();
    }
  }

  void OnDisplayLink(CADisplayLink* display_link) {
    if (display_link == nil || observers_.empty()) {
      return;
    }

    KFrameArgs args;
    args.frame_time = KTimeTicksFromSeconds(display_link.timestamp);
    args.deadline = KTimeTicksFromSeconds(display_link.targetTimestamp);

    const double interval_seconds =
        display_link.targetTimestamp > display_link.timestamp
            ? (display_link.targetTimestamp - display_link.timestamp)
            : (display_link.duration > 0.0 ? display_link.duration : (1.0 / 60.0));
    args.interval = KTimeDeltaFromSeconds(interval_seconds);
    args.sequence = ++sequence_;

    const std::vector<KFrameObserver*> snapshot = observers_;
    for (KFrameObserver* observer : snapshot) {
      if (observer == nullptr) {
        continue;
      }
      if (std::find(observers_.begin(), observers_.end(), observer) == observers_.end()) {
        continue;
      }
      observer->OnFrame(args);
    }
  }

 private:
  void Start() {
    if (display_link_ != nil) {
      return;
    }

    display_link_ = [CADisplayLink displayLinkWithTarget:target_
                                                selector:@selector(onDisplayLink:)];
    [display_link_ addToRunLoop:[NSRunLoop mainRunLoop] forMode:NSRunLoopCommonModes];
  }

  void Stop() {
    if (display_link_ == nil) {
      return;
    }

    [display_link_ invalidate];
    display_link_ = nil;
  }

  std::vector<KFrameObserver*> observers_;
  uint64_t sequence_ = 0;
  CADisplayLink* display_link_ = nil;
  KairoDisplayLinkTarget* target_ = nil;
};

struct DisplayLinkFrameSource::Impl : public DisplayLinkFrameSourceImpl {};

DisplayLinkFrameSource::DisplayLinkFrameSource() : impl_(std::make_unique<Impl>()) {}

DisplayLinkFrameSource::~DisplayLinkFrameSource() = default;

void DisplayLinkFrameSource::AddObserver(KFrameObserver* observer) {
  impl_->AddObserver(observer);
}

void DisplayLinkFrameSource::RemoveObserver(KFrameObserver* observer) {
  impl_->RemoveObserver(observer);
}

}  // namespace kairo

@implementation KairoDisplayLinkTarget

- (void)onDisplayLink:(CADisplayLink*)displayLink {
  if (self.owner != nullptr) {
    self.owner->OnDisplayLink(displayLink);
  }
}

@end
