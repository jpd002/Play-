#import <UIKit/UIKit.h>
#import <GameController/GameController.h>
#import "iCadeReaderView.h"
#import "VirtualPadView.h"

@interface EmulatorViewController : UIViewController <iCadeEventDelegate>

+ (void)registerPreferences;

@property(nonatomic) iCadeReaderView* iCadeReader;
@property(nonatomic) GCController* gController __attribute__((weak_import));
@property(nonatomic, strong) id connectObserver;
@property(nonatomic, strong) id disconnectObserver;

@property VirtualPadView* virtualPadView;
@property NSString* bootablePath;

@property NSTimer* fpsCounterTimer;
@property UILabel* fpsCounterLabel;
#ifdef PROFILE
@property UILabel* profilerStatsLabel;
#endif

@end
