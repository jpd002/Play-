#import <Cocoa/Cocoa.h>

@interface VfsManagerBinding : NSObject

@property (copy) NSString* deviceName;
@property (readonly, copy) NSString* bindingType;
@property (readonly, copy) NSString* bindingValue;
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
@property (copy) NSString* value;

-(VfsManagerDirectoryBinding*)init: (NSString*)deviceName preferenceName: (NSString*)preferenceName;
@property (readonly, copy) NSString* bindingType;
@property (readonly, copy) NSString* bindingValue;
-(void)requestModification;
-(void)save;

@end

@interface VfsManagerCdrom0Binding : VfsManagerBinding

@property (copy) NSString* value;

-(VfsManagerCdrom0Binding*)init;
@property (readonly, copy) NSString* bindingType;
@property (readonly, copy) NSString* bindingValue;
-(void)requestModification;
-(void)save;

@end
