#import "ViewController.h"

#import "MainView.h"

@implementation ViewController

- (void)loadView {
  self.view = [[MainView alloc] initWithFrame:UIScreen.mainScreen.bounds];
}

- (UIStatusBarStyle)preferredStatusBarStyle {
  return UIStatusBarStyleDarkContent;
}

@end
