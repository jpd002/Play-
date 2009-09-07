#import "VfsManagerBindings.h"
#import "../AppConfig.h"

#define PREFERENCE_CDROM0_PATH ("ps2.cdrom0.path")

@implementation CVfsManagerBindings

-(CVfsManagerBindings*)init
{
	CVfsManagerBindings* result = [super init];
	m_bindings = [[NSMutableArray alloc] init];
	{
		NSObject* objectToAdd = [[CVfsManagerCdrom0Binding alloc] init];
		[objectToAdd autorelease];
		[m_bindings addObject: objectToAdd];
	}
	{
		NSObject* objectToAdd = [[CVfsManagerDirectoryBinding alloc] init: @"mc0" preferenceName: @"ps2.mc0.directory"];
		[objectToAdd autorelease];
		[m_bindings addObject: objectToAdd];
	}
	{
		NSObject* objectToAdd = [[CVfsManagerDirectoryBinding alloc] init: @"mc1" preferenceName: @"ps2.mc1.directory"];
		[objectToAdd autorelease];
		[m_bindings addObject: objectToAdd];
	}
	{
		NSObject* objectToAdd = [[CVfsManagerDirectoryBinding alloc] init: @"host" preferenceName: @"ps2.host.directory"];
		[objectToAdd autorelease];
		[m_bindings addObject: objectToAdd];
	}
	return result;
}

-(void)dealloc
{
	[m_bindings release];
	[super dealloc];
}

-(void)save
{
	NSEnumerator* bindingIterator = [m_bindings objectEnumerator];
	CVfsManagerBinding* binding = nil;
	while(binding = [bindingIterator nextObject])
	{
		[binding save];
	}
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

-(void)save
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
	const char* preferenceValue = CAppConfig::GetInstance().GetPreferenceString([preferenceName UTF8String]);
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

-(void)dealloc
{
	[m_deviceName release];
	[m_value release];
	[m_preference release];
	[super dealloc];
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
	[openPanel setCanChooseDirectories: YES];
	[openPanel setCanCreateDirectories: YES];
	[openPanel setCanChooseFiles: NO];
	if([openPanel runModal] != NSOKButton)
	{
		return;
	}
	NSString* filename = [openPanel filename];
	[filename retain];
	[m_value release];
	m_value = filename;
}

-(void)save
{
	CAppConfig::GetInstance().SetPreferenceString([m_preference UTF8String], [m_value UTF8String]);
}

@end

@implementation CVfsManagerCdrom0Binding

-(CVfsManagerCdrom0Binding*)init
{
	CVfsManagerCdrom0Binding* result = [super init];
	const char* preferenceValue = CAppConfig::GetInstance().GetPreferenceString(PREFERENCE_CDROM0_PATH);
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

-(void)dealloc
{
	[m_value release];
	[super dealloc];
}

-(NSString*)deviceName
{
	return @"cdrom0";
}

-(NSString*)bindingType
{
	return @"Disk Image";
}

-(NSString*)bindingValue
{
	return m_value;
}

-(void)requestModification
{
	NSOpenPanel* openPanel = [NSOpenPanel openPanel];
	NSArray* fileTypes = [NSArray arrayWithObjects: @"iso", @"isz", nil];
	if([openPanel runModalForTypes:fileTypes] != NSOKButton)
	{
		return;
	}
	NSString* filename = [openPanel filename];
	[filename retain];
	[m_value release];
	m_value = filename;	
}

-(void)save
{
	CAppConfig::GetInstance().SetPreferenceString(PREFERENCE_CDROM0_PATH, [m_value UTF8String]);
}

@end
