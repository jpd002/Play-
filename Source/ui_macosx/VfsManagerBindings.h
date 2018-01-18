#import <Cocoa/Cocoa.h>

@interface VfsManagerBinding : NSObject

@property (copy) NSString* deviceName;
@property (copy) NSString* bindingType;
@property (copy) NSString* bindingValue;

-(void)requestModification;
-(void)save;

@end

@interface VfsManagerBindings : NSObject<NSTableViewDataSource>

@property NSMutableArray* bindings;

-(VfsManagerBindings*)init;
-(void)save;
-(VfsManagerBinding*)getBindingAt: (NSUInteger)index;

@end

@interface VfsManagerDirectoryBinding : VfsManagerBinding

@property (copy) NSString* preferenceName;

-(VfsManagerDirectoryBinding*)init: (NSString*)deviceName preferenceName: (NSString*)preferenceName;
-(void)requestModification;
-(void)save;

@end

@interface VfsManagerCdrom0Binding : VfsManagerBinding

-(VfsManagerCdrom0Binding*)init;
-(void)requestModification;
-(void)save;

@end
