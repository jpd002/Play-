#import <UIKit/UIKit.h>

@interface AltServerJitService : NSObject

+ (AltServerJitService*)sharedAltServerJitService;
- (void)registerPreferences;
- (void)startProcess;

@property bool processStarted;

@end
