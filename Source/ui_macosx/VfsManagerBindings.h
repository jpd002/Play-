#import <Cocoa/Cocoa.h>

@interface VfsManagerBinding : NSObject

@property NSString* deviceName;

-(NSString*)bindingType;
-(NSString*)bindingValue;
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

@property NSString* preferenceName;
@property NSString* value;

-(VfsManagerDirectoryBinding*)init: (NSString*)deviceName preferenceName: (NSString*)preferenceName;
-(NSString*)bindingType;
-(NSString*)bindingValue;
-(void)requestModification;
-(void)save;

@end

@interface VfsManagerCdrom0Binding : VfsManagerBinding

@property NSString* value;

-(VfsManagerCdrom0Binding*)init;
-(NSString*)bindingType;
-(NSString*)bindingValue;
-(void)requestModification;
-(void)save;

@end
