#include "main_run_loop_task_runner.h"

#import <dispatch/dispatch.h>

namespace kairo {

void MainRunLoopTaskRunner::PostTask(KTaskFunction task, void* context) {
  if (task == nullptr) {
    return;
  }

  dispatch_async(dispatch_get_main_queue(), ^{
    task(context);
  });
}

void MainRunLoopTaskRunner::PostDelayedTask(KTimeDelta delay,
                                            KTaskFunction task,
                                            void* context) {
  if (task == nullptr) {
    return;
  }

  const int64_t delay_ns = delay.micros * 1000;
  dispatch_after(
      dispatch_time(DISPATCH_TIME_NOW, delay_ns), dispatch_get_main_queue(), ^{
        task(context);
      });
}

}  // namespace kairo
