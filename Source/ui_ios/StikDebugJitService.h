//
//  StikDebugJitService.h
//  Play! iOS - iOS 26 JIT Support via StikDebug
//
//  Provides JIT activation for iOS 26+ devices with TXM
//  Works alongside existing AltServerJitService
//

#import <UIKit/UIKit.h>

@interface StikDebugJitService : NSObject

/// Shared singleton instance
+ (StikDebugJitService*)sharedService;

/// Register preferences for settings UI
- (void)registerPreferences;

/// Check if iOS 26 TXM is active (requires StikDebug)
- (BOOL)hasTXM;

/// Check if JIT is currently available
- (BOOL)isJitAvailable;

/// Check if StikDebug activation is needed
- (BOOL)needsActivation;

/// Request JIT activation via StikDebug app
/// @param completion Called with success status
- (void)requestActivation:(void (^)(BOOL success))completion;

/// Detach StikDebug debugger (call after all JIT allocations)
- (void)detachDebugger;

/// JIT enabled status (read-only)
@property (nonatomic, readonly) BOOL jitEnabled;

/// TXM active status (read-only)
@property (nonatomic, readonly) BOOL txmActive;

/// iOS version (read-only)
@property (nonatomic, readonly) float iosVersion;

@end
