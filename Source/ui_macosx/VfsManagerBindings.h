#import <Cocoa/Cocoa.h>

@interface VfsManagerBinding : NSObject
{

}

-(NSString*)deviceName;
-(NSString*)bindingType;
-(NSString*)bindingValue;
-(void)requestModification;
-(void)save;

@end

@interface VfsManagerBindings : NSObject
{
	NSMutableArray*		m_bindings;
}

-(VfsManagerBindings*)init;
-(void)save;
-(int)numberOfRowsInTableView: (NSTableView*)tableView;
-(id)tableView: (NSTableView*)tableView objectValueForTableColumn:(NSTableColumn*)tableColumn row:(int)row;
-(VfsManagerBinding*)getBindingAt: (unsigned int)index;

@end

@interface VfsManagerDirectoryBinding : VfsManagerBinding
{
	NSString*	m_deviceName;
	NSString*	m_preference;
	NSString*	m_value;
}

-(VfsManagerDirectoryBinding*)init: (NSString*)deviceName preferenceName: (NSString*)preferenceName;
-(NSString*)deviceName;
-(NSString*)bindingType;
-(NSString*)bindingValue;
-(void)requestModification;
-(void)save;

@end

@interface VfsManagerCdrom0Binding : VfsManagerBinding
{
	NSString*	m_value;
}

-(VfsManagerCdrom0Binding*)init;
-(NSString*)deviceName;
-(NSString*)bindingType;
-(NSString*)bindingValue;
-(void)requestModification;
-(void)save;

@end
