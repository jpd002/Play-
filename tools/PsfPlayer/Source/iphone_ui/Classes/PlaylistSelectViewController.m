#import "PlaylistSelectViewController.h"

@implementation PlaylistSelectViewController

-(void)dealloc 
{
	[m_playlists release];
    [super dealloc];
}

-(void)viewDidLoad 
{
    [super viewDidLoad];
	
	NSString* path = [@"~" stringByExpandingTildeInPath];
	NSFileManager* localFileManager = [[NSFileManager alloc] init];
	
	NSDirectoryEnumerator* dirEnum = [localFileManager enumeratorAtPath: path];	

	NSString* file;
	
	m_playlists = [[NSMutableArray alloc] init];
	
	while(file = [dirEnum nextObject]) 
	{
		if([[file pathExtension] isEqualToString: @"psfpl"])
		{
			[m_playlists addObject: file];
		}
	}
	
	[localFileManager release];
	
	m_selectionHandler = nil;
	m_selectionHandlerSelector = 0;
}

-(void)setSelectionHandler: (id)handler selector: (SEL)sel
{
	m_selectionHandler = handler;
	m_selectionHandlerSelector = sel;
}

-(void)viewWillAppear:(BOOL)animated 
{
    [super viewWillAppear:animated];
}

-(void)viewDidDisappear: (BOOL)animated 
{
    [super viewDidDisappear: animated];
	[self.tableView deselectRowAtIndexPath: [self.tableView indexPathForSelectedRow] animated:NO];
}

-(IBAction)onCancel: (id)sender
{
	[self dismissModalViewControllerAnimated: YES];
}

-(NSInteger)numberOfSectionsInTableView: (UITableView*)tableView 
{
	return 1;
}


-(NSInteger)tableView: (UITableView *)tableView numberOfRowsInSection: (NSInteger)section 
{
	return [m_playlists count];
}

-(NSString*)tableView: (UITableView*)tableView titleForHeaderInSection: (NSInteger)section 
{
	return @"";
}

-(UITableViewCell*)tableView: (UITableView*)tableView cellForRowAtIndexPath: (NSIndexPath*)indexPath 
{
	static NSString *CellIdentifier = @"Cell";
    
	UITableViewCell *cell = [tableView dequeueReusableCellWithIdentifier: CellIdentifier];
    if (cell == nil)
	{
        cell = [[[UITableViewCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier: CellIdentifier] autorelease];
    }
    
	assert(indexPath.row < [m_playlists count]);
	NSString* playlistPath = [m_playlists objectAtIndex: indexPath.row];
	
	cell.textLabel.text = [playlistPath lastPathComponent];
	
    return cell;
}

-(void)tableView: (UITableView*)tableView didSelectRowAtIndexPath: (NSIndexPath*)indexPath
{	
	if(m_selectionHandler != nil)
	{
		[m_selectionHandler performSelector: m_selectionHandlerSelector];
	}

	[self dismissModalViewControllerAnimated: YES];
}

-(NSString*)selectedPlaylistPath
{
	NSIndexPath* indexPath = [self.tableView indexPathForSelectedRow];
	assert(indexPath.row < [m_playlists count]);

	NSString* playlistPath = [m_playlists objectAtIndex: indexPath.row];
	NSString* homeDirPath = [@"~" stringByExpandingTildeInPath];
	NSString* absolutePath = [homeDirPath stringByAppendingPathComponent: playlistPath];

	return absolutePath;
}

@end
