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
		return binding.deviceName;
	}
	else if([[tableColumn identifier] isEqualToString: @"bindingType"])
	{
		return binding.bindingType;
	}
	else if([[tableColumn identifier] isEqualToString: @"bindingValue"])
	{
		return binding.bindingValue;
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
		self.bindingType = @"Directory";
		self.preferenceName = preferenceName;
		auto preferenceValue = CAppConfig::GetInstance().GetPreferenceString([preferenceName UTF8String]);
		self.bindingValue = preferenceValue ? [NSString stringWithUTF8String: preferenceValue] : @"";
	}
	return self;
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
	self.bindingValue = [[openPanel URL] path];
}

-(void)save
{
	CAppConfig::GetInstance().SetPreferenceString([self.preferenceName UTF8String], [self.bindingValue UTF8String]);
}

@end

@implementation VfsManagerCdrom0Binding

-(VfsManagerCdrom0Binding*)init
{
	if(self = [super init])
	{
		self.deviceName = @"cdrom0";
		self.bindingType = @"Disk Image";
		auto preferenceValue = CAppConfig::GetInstance().GetPreferencePath(PREF_PS2_CDROM0_PATH);
		auto nativeString = preferenceValue.native();
		self.bindingValue = [[NSFileManager defaultManager] stringWithFileSystemRepresentation: nativeString.c_str() length: nativeString.size()];
	}
	return self;
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
	self.bindingValue = [[openPanel URL] path];
}

-(void)save
{
	auto pathString = [self.bindingValue length] ? [self.bindingValue fileSystemRepresentation] : "";
	CAppConfig::GetInstance().SetPreferencePath(PREF_PS2_CDROM0_PATH, pathString);
}

@end
