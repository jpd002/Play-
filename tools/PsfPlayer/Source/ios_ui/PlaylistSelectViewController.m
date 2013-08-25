#import "PlaylistSelectViewController.h"

@implementation PlaylistSelectViewController

@synthesize delegate;

-(void)dealloc
{
	[m_archives release];
	[super dealloc];
}

-(void)viewDidLoad 
{
	[super viewDidLoad];
	
	NSString* path = [@"~" stringByExpandingTildeInPath];
	NSFileManager* localFileManager = [[NSFileManager alloc] init];
	
	NSDirectoryEnumerator* dirEnum = [localFileManager enumeratorAtPath: path];	
	
	m_archives = [[NSMutableArray alloc] init];
	
	NSString* file = nil;
	while(file = [dirEnum nextObject])
	{
		if([[file pathExtension] isEqualToString: @"zip"])
		{
			[m_archives addObject: file];
		}
	}
	
	[localFileManager release];
}

-(void)viewDidDisappear: (BOOL)animated
{
	[super viewDidDisappear: animated];
	//[self.tableView deselectRowAtIndexPath: [self.tableView indexPathForSelectedRow] animated:NO];
}

-(IBAction)onCancel: (id)sender
{
	[self dismissViewControllerAnimated: YES completion: nil];
}

-(NSInteger)numberOfSectionsInTableView: (UITableView*)tableView 
{
	return 1;
}

-(NSInteger)tableView: (UITableView *)tableView numberOfRowsInSection: (NSInteger)section 
{
	return [m_archives count];
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
		cell = [[[UITableViewCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier: CellIdentifier] autorelease];
	}
	
	assert(indexPath.row < [m_archives count]);
	NSString* playlistPath = [m_archives objectAtIndex: indexPath.row];
	
	cell.textLabel.text = [playlistPath lastPathComponent];
	
	return cell;
}

-(void)tableView: (UITableView*)tableView didSelectRowAtIndexPath: (NSIndexPath*)indexPath
{
	assert(indexPath.row < [m_archives count]);
	
	NSString* playlistPath = [m_archives objectAtIndex: indexPath.row];
	NSString* homeDirPath = [@"~" stringByExpandingTildeInPath];
	NSString* absolutePath = [homeDirPath stringByAppendingPathComponent: playlistPath];
	
	if(self.delegate != nil)
	{
		[self.delegate onPlaylistSelected: absolutePath];
	}

	[self dismissViewControllerAnimated: YES completion: nil];
}

@end
