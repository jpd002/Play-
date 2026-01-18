#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@interface JITInitializer : NSObject

/// Initializes the CodeGen JIT system with the appropriate mode based on iOS version and device capabilities
+ (void)initializeJITSystem;

@end

NS_ASSUME_NONNULL_END
