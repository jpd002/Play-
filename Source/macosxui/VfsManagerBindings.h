#import <Cocoa/Cocoa.h>

@interface CVfsManagerBinding : NSObject
{

}

-(NSString*)deviceName;
-(NSString*)bindingType;
-(NSString*)bindingValue;
-(void)requestModification;
-(void)save;

@end

@interface CVfsManagerBindings : NSObject
{
	NSMutableArray*		m_bindings;
}

-(CVfsManagerBindings*)init;
-(void)save;
-(int)numberOfRowsInTableView: (NSTableView*)tableView;
-(id)tableView: (NSTableView*)tableView objectValueForTableColumn:(NSTableColumn*)tableColumn row:(int)row;
-(CVfsManagerBinding*)getBindingAt: (unsigned int)index;

@end

@interface CVfsManagerDirectoryBinding : CVfsManagerBinding
{
	NSString*	m_deviceName;
	NSString*	m_preference;
	NSString*	m_value;
}

-(CVfsManagerDirectoryBinding*)init: (NSString*)deviceName preferenceName: (NSString*)preferenceName;
-(NSString*)deviceName;
-(NSString*)bindingType;
-(NSString*)bindingValue;
-(void)requestModification;
-(void)save;

@end