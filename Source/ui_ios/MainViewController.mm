#import "MainViewController.h"
#import "EmulatorViewController.h"
#import "IosUtils.h"
#import "DiskUtils.h"

@interface MainViewController ()

@end

@implementation MainViewController

-(void)viewDidLoad
{
	NSString* path = [@"~" stringByExpandingTildeInPath];
	NSFileManager* localFileManager = [[NSFileManager alloc] init];
	
	NSDirectoryEnumerator* dirEnum = [localFileManager enumeratorAtPath: path];	
	
	NSMutableArray* images = [[NSMutableArray alloc] init];
	
	NSString* file = nil;
	while(file = [dirEnum nextObject])
	{
        std::string diskId;
		if(
			IosUtils::IsLoadableExecutableFileName(file) ||
			(IosUtils::IsLoadableDiskImageFileName(file) &&
             DiskUtils::TryGetDiskId([[NSString stringWithFormat:@"%@/%@", path, file] UTF8String], &diskId))
		)
		{
            NSMutableDictionary *disk = [[NSMutableDictionary alloc] init];
            [disk setValue:[NSString stringWithUTF8String:diskId.c_str()] forKey:@"serial"];
            [disk setValue:file forKey:@"file"];
			[images addObject: disk];
            NSLog(@"%@: %s", file, diskId.c_str());
		}
	}
	
	self.images = [images sortedArrayUsingSelector: @selector(caseInsensitiveCompare:)];
    [self.tableView reloadData];
}

-(void)prepareForSegue: (UIStoryboardSegue*)segue sender: (id)sender
{
	NSIndexPath* indexPath = self.tableView.indexPathForSelectedRow;
    NSString* filePath = [[self.images objectAtIndex: indexPath.row] objectForKey:@"file"];
	NSString* homeDirPath = [@"~" stringByExpandingTildeInPath];
	NSString* absolutePath = [homeDirPath stringByAppendingPathComponent: filePath];
	EmulatorViewController* emulatorViewController = segue.destinationViewController;
	emulatorViewController.imagePath = absolutePath;
}

-(NSInteger)numberOfSectionsInTableView: (UITableView*)tableView 
{
	return 1;
}

-(NSInteger)tableView: (UITableView *)tableView numberOfRowsInSection: (NSInteger)section 
{
	return [self.images count];
}

-(NSString*)tableView: (UITableView*)tableView titleForHeaderInSection: (NSInteger)section 
{
	return @"";
}

-(UITableViewCell*)tableView: (UITableView*)tableView cellForRowAtIndexPath: (NSIndexPath*)indexPath 
{
	static NSString *CellIdentifier = @"Cell";

	UITableViewCell *cell = [tableView dequeueReusableCellWithIdentifier: CellIdentifier];
	if(cell == nil)
	{
		cell = [[UITableViewCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier: CellIdentifier];
	}
	
	assert(indexPath.row < [self.images count]);
	NSString* imagePath = [[self.images objectAtIndex: indexPath.row] objectForKey:@"file"];
	
	cell.textLabel.text = [imagePath lastPathComponent];
	
	return cell;
}

-(void)tableView: (UITableView*)tableView didSelectRowAtIndexPath: (NSIndexPath*)indexPath
{
	assert(indexPath.row < [self.images count]);
	[self performSegueWithIdentifier: @"showEmulator" sender: self];
}

@end
