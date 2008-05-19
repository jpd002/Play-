#import <Cocoa/Cocoa.h>

@interface CVfsManagerBindings : NSObject
{
	NSMutableArray*		m_bindings;
}

-(CVfsManagerBindings*)init;
-(void)dealloc;
-(int)numberOfRowsInTableView: (NSTableView*)tableView;
-(id)tableView: (NSTableView*)tableView objectValueForTableColumn:(NSTableColumn*)tableColumn row:(int)row;

@end

@interface CVfsManagerBinding : NSObject
{

}

-(NSString*)deviceName;
-(NSString*)bindingType;
-(NSString*)bindingValue;

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

@end