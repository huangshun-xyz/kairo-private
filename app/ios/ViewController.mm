#import "ViewController.h"

@implementation ViewController

- (void)loadView {
  Class main_view_class = NSClassFromString(@"MainView");
  if (main_view_class == Nil) {
    main_view_class = NSClassFromString(@"kairo_ios_demo.MainView");
  }

  UIView* root_view = nil;
  if (main_view_class != Nil) {
    root_view = [[main_view_class alloc] initWithFrame:UIScreen.mainScreen.bounds];
  }

  self.view = root_view ?: [[UIView alloc] initWithFrame:UIScreen.mainScreen.bounds];
}

- (UIStatusBarStyle)preferredStatusBarStyle {
  return UIStatusBarStyleDarkContent;
}

@end
