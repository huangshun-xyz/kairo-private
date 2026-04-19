#include "display_link_frame_source.h"

#include <algorithm>
#include <vector>

#import <Foundation/Foundation.h>
#import <QuartzCore/CADisplayLink.h>

#include "time_types.h"

@interface KairoDisplayLinkTarget : NSObject

@property(nonatomic, assign) kairo::DisplayLinkFrameSource* owner;

- (void)onDisplayLink:(CADisplayLink*)displayLink;

@end

namespace kairo {

DisplayLinkFrameSource::DisplayLinkFrameSource() {
  target_ = [[KairoDisplayLinkTarget alloc] init];
  target_.owner = this;
}

DisplayLinkFrameSource::~DisplayLinkFrameSource() {
  Stop();
  target_.owner = nullptr;
}

void DisplayLinkFrameSource::AddObserver(KFrameObserver* observer) {
  if (observer == nullptr ||
      std::find(observers_.begin(), observers_.end(), observer) != observers_.end()) {
    return;
  }

  observers_.push_back(observer);
  if (observers_.size() == 1u) {
    Start();
  }
}

void DisplayLinkFrameSource::RemoveObserver(KFrameObserver* observer) {
  observers_.erase(std::remove(observers_.begin(), observers_.end(), observer), observers_.end());
  if (observers_.empty()) {
    Stop();
  }
}

void DisplayLinkFrameSource::OnDisplayLink(CADisplayLink* display_link) {
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

void DisplayLinkFrameSource::Start() {
  if (display_link_ != nil) {
    return;
  }

  display_link_ = [CADisplayLink displayLinkWithTarget:target_
                                              selector:@selector(onDisplayLink:)];
  [display_link_ addToRunLoop:[NSRunLoop mainRunLoop] forMode:NSRunLoopCommonModes];
}

void DisplayLinkFrameSource::Stop() {
  if (display_link_ == nil) {
    return;
  }

  [display_link_ invalidate];
  display_link_ = nil;
}

}  // namespace kairo

@implementation KairoDisplayLinkTarget

- (void)onDisplayLink:(CADisplayLink*)displayLink {
  if (self.owner != nullptr) {
    self.owner->OnDisplayLink(displayLink);
  }
}

@end
