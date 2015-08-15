#import "MainViewController.h"
#import "EmulatorViewController.h"
#import "IosUtils.h"
#import "DiskUtils.h"
#import "CoverViewCell.h"

@interface MainViewController ()

@end

@implementation MainViewController

-(void)viewDidLoad
{
    self.database = [[SqliteDatabase alloc] init];
    
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
    
    self.images = [images sortedArrayUsingComparator:^NSComparisonResult(id a, id b) {
        NSString *first = [[a objectForKey:@"file"] lastPathComponent];
        NSString *second = [[b objectForKey:@"file"] lastPathComponent];
        return [first caseInsensitiveCompare:second];
    }];

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

- (CGFloat)tableView:(UITableView *)tableView heightForRowAtIndexPath:(NSIndexPath *)indexPath {
    return 160;
    // Assign the specific cell height to prevent issues with custom size
}

-(UITableViewCell*)tableView: (UITableView*)tableView cellForRowAtIndexPath: (NSIndexPath*)indexPath 
{
    static NSString *CellIdentifier = @"coverCell";
    
    CoverViewCell *cell = [tableView dequeueReusableCellWithIdentifier:CellIdentifier forIndexPath:indexPath];
    NSDictionary *disk = [self.images objectAtIndex: indexPath.row];
    NSString* diskId = [disk objectForKey:@"serial"];
    
    NSDictionary *game = [self.database getDiskInfo:diskId];
    if ([[game objectForKey:@"title"] isEqual:@""]) {
        cell.nameLabel.text = [[disk objectForKey:@"file"] lastPathComponent];
    } else {
        cell.nameLabel.text = [game objectForKey:@"title"];
    }
    if (![[game objectForKey:@"boxart"] isEqual:@"404"]) {
        NSString *imageIcon = [[NSString alloc] initWithFormat:@"http://thegamesdb.net/banners/%@", [game objectForKey:@"boxart"]];
        [cell.coverImage setImageWithURL:[NSURL URLWithString:imageIcon] placeholderImage:[UIImage imageNamed:@"boxart.png"]];
    }
    cell.overviewLabel.text = [game objectForKey:@"overview"];
    
	return cell;
}

-(void)tableView: (UITableView*)tableView didSelectRowAtIndexPath: (NSIndexPath*)indexPath
{
	[self performSegueWithIdentifier: @"showEmulator" sender: self];
}

@end
