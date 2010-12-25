#include "Utf8.h"
#import "PlaylistViewController.h"

@implementation PlaylistViewController

-(void)dealloc 
{
    [super dealloc];
}

-(void)viewDidLoad
{
	m_playlist = NULL;
	[super viewDidLoad];
}

-(void)setPlaylist: (CPlaylist*)playlist
{
	m_playlist = playlist;
	[self.tableView reloadData];
}

-(void)setSelectionHandler: (id)handler selector: (SEL)sel
{
	m_selectionHandler = handler;
	m_selectionHandlerSelector = sel;
}

-(NSString*)selectedPlaylistItemPath
{
	NSIndexPath* indexPath = [self.tableView indexPathForSelectedRow];
	assert(indexPath.row < m_playlist->GetItemCount());
	
	const CPlaylist::ITEM& item(m_playlist->GetItem(indexPath.row));
	return [NSString stringWithUTF8String: item.path.c_str()];
}

-(NSInteger)numberOfSectionsInTableView: (UITableView *)tableView 
{
    return 1;
}

-(NSInteger)tableView: (UITableView*)tableView numberOfRowsInSection: (NSInteger)section 
{
	if(m_playlist == NULL)
	{
		return 0;
	}
	else
	{
		return m_playlist->GetItemCount();
	}
}

// Customize the appearance of table view cells.
-(UITableViewCell*)tableView: (UITableView *)tableView cellForRowAtIndexPath: (NSIndexPath*)indexPath
{
    static NSString *CellIdentifier = @"Cell";
    
    UITableViewCell *cell = [tableView dequeueReusableCellWithIdentifier:CellIdentifier];
    if (cell == nil) 
	{
        cell = [[[UITableViewCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:CellIdentifier] autorelease];
    }
    
	if(m_playlist != NULL)
	{
		const CPlaylist::ITEM& item(m_playlist->GetItem(indexPath.row));
		std::string convString = Framework::Utf8::ConvertTo(item.title);
		cell.textLabel.text = [NSString stringWithUTF8String: convString.c_str()];
	}
	
    return cell;
}

-(void)tableView: (UITableView*)tableView didSelectRowAtIndexPath: (NSIndexPath*)indexPath 
{
	if(m_selectionHandler != nil)
	{
		[m_selectionHandler performSelector: m_selectionHandlerSelector];
	}
}

@end
