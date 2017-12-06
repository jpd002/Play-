#import "VfsManagerBindings.h"
#import "../AppConfig.h"
#import "../PS2VM_Preferences.h"

@implementation VfsManagerBindings

-(VfsManagerBindings*)init
{
	if(self = [super init])
	{
		self.bindings = [[NSMutableArray alloc] init];
		[self.bindings addObject: [[VfsManagerCdrom0Binding alloc] init]];
		[self.bindings addObject: [[VfsManagerDirectoryBinding alloc] init: @"mc0" preferenceName: @PREF_PS2_MC0_DIRECTORY]];
		[self.bindings addObject: [[VfsManagerDirectoryBinding alloc] init: @"mc1" preferenceName: @PREF_PS2_MC1_DIRECTORY]];
		[self.bindings addObject: [[VfsManagerDirectoryBinding alloc] init: @"host" preferenceName: @PREF_PS2_HOST_DIRECTORY]];
	}
	return self;
}

-(void)save
{
	NSEnumerator* bindingIterator = [self.bindings objectEnumerator];
	VfsManagerBinding* binding = nil;
	while(binding = [bindingIterator nextObject])
	{
		[binding save];
	}
}

-(NSInteger)numberOfRowsInTableView: (NSTableView*)tableView
{
	return [self.bindings count];
}

-(id)tableView: (NSTableView*)tableView objectValueForTableColumn: (NSTableColumn*)tableColumn row: (NSInteger)row
{
	if(row >= [self.bindings count]) return @"";
	VfsManagerBinding* binding = [self.bindings objectAtIndex: row];
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

-(VfsManagerBinding*)getBindingAt: (NSUInteger)index
{
	if(index >= [self.bindings count]) return nil;
	return [self.bindings objectAtIndex: index];
}

@end

//---------------------------------------------------------------------------

@implementation VfsManagerBinding

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

@implementation VfsManagerDirectoryBinding

-(VfsManagerDirectoryBinding*)init: (NSString*)deviceName preferenceName: (NSString*)preferenceName
{
	if(self = [super init])
	{
		self.deviceName = deviceName;
		self.preferenceName = preferenceName;
		auto preferenceValue = CAppConfig::GetInstance().GetPreferenceString([preferenceName UTF8String]);
		self.value = preferenceValue ? [NSString stringWithUTF8String: preferenceValue] : @"";
	}
	return self;
}

-(NSString*)bindingType
{
	return @"Directory";
}

-(NSString*)bindingValue
{
	return self.value;
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
	self.value = [[openPanel URL] path];
}

-(void)save
{
	CAppConfig::GetInstance().SetPreferenceString([self.preferenceName UTF8String], [self.value UTF8String]);
}

@end

@implementation VfsManagerCdrom0Binding

-(VfsManagerCdrom0Binding*)init
{
	if(self = [super init])
	{
		auto preferenceValue = CAppConfig::GetInstance().GetPreferenceString(PS2VM_CDROM0PATH);
		self.value = preferenceValue ? [NSString stringWithUTF8String: preferenceValue] : @"";
	}
	return self;
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
	return self.value;
}

-(void)requestModification
{
	NSOpenPanel* openPanel = [NSOpenPanel openPanel];
	NSArray* fileTypes = [NSArray arrayWithObjects: @"iso", @"isz", @"cso", @"bin", nil];
	openPanel.allowedFileTypes = fileTypes;
	if([openPanel runModal] != NSOKButton)
	{
		return;
	}
	self.value = [[openPanel URL] path];
}

-(void)save
{
	CAppConfig::GetInstance().SetPreferenceString(PS2VM_CDROM0PATH, [self.value UTF8String]);
}

@end
