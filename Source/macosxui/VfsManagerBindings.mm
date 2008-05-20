#import "VfsManagerBindings.h"
#import "../Config.h"

@implementation CVfsManagerBindings

-(CVfsManagerBindings*)init
{
	CVfsManagerBindings* result = [super init];
	m_bindings = [[NSMutableArray alloc] init];
	[m_bindings addObject: [[CVfsManagerDirectoryBinding alloc] init: @"mc0" preferenceName: @"ps2.mc0.directory"]];
	[m_bindings addObject: [[CVfsManagerDirectoryBinding alloc] init: @"mc1" preferenceName: @"ps2.mc1.directory"]];
	[m_bindings addObject: [[CVfsManagerDirectoryBinding alloc] init: @"host" preferenceName: @"ps2.host.directory"]];
	return result;
}

-(void)dealloc
{
	[m_bindings release];
	[super dealloc];
}

-(int)numberOfRowsInTableView: (NSTableView*)tableView
{
	return [m_bindings count];
}

-(id)tableView: (NSTableView*)tableView objectValueForTableColumn: (NSTableColumn*)tableColumn row: (int)row
{
	if(row >= [m_bindings count]) return @"";
	CVfsManagerBinding* binding = [m_bindings objectAtIndex: row];
	if([[tableColumn identifier] isEqualToString: @"deviceName"])
	{
		return [binding deviceName];
	}
	else if([[tableColumn identifier] isEqualToString: @"bindingType"])
	{
		return [binding bindingType];
	}
	else if([[tableColumn identifier] isEqualToString: @"bindingValue"])
	{
		return [binding bindingValue];
	}
	return @"";
}

-(CVfsManagerBinding*)getBindingAt: (unsigned int)index
{
	if(index >= [m_bindings count]) return nil;
	return [m_bindings objectAtIndex: index];
}

@end

//---------------------------------------------------------------------------

@implementation CVfsManagerBinding

-(NSString*)deviceName
{
	[self doesNotRecognizeSelector: _cmd];
	return nil;
}

-(NSString*)bindingValue
{
	[self doesNotRecognizeSelector: _cmd];
	return nil;
}

-(NSString*)bindingType
{
	[self doesNotRecognizeSelector: _cmd];
	return nil;
}

-(void)requestModification
{
	[self doesNotRecognizeSelector: _cmd];
}

@end

//---------------------------------------------------------------------------

@implementation CVfsManagerDirectoryBinding

-(CVfsManagerDirectoryBinding*)init: (NSString*)deviceName preferenceName: (NSString*)preferenceName
{
	CVfsManagerDirectoryBinding* result = [super init];
	m_deviceName = deviceName;
	m_preference = preferenceName;
	const char* preferenceValue = CConfig::GetInstance().GetPreferenceString([preferenceName UTF8String]);
	if(preferenceValue == NULL)
	{
		m_value = @"";
	}
	else
	{
		m_value = [[NSString alloc] initWithCString: preferenceValue];
	}
	return result;
}

-(NSString*)deviceName
{
	return m_deviceName;
}

-(NSString*)bindingType
{
	return @"Directory";
}

-(NSString*)bindingValue
{
	return m_value;
}

-(void)requestModification
{
	NSOpenPanel* openPanel = [NSOpenPanel openPanel];
	NSArray* fileTypes = [NSArray arrayWithObject:@"elf"];
	if([openPanel runModalForTypes:fileTypes] != NSOKButton)
	{
		return;
	}
}

@end
