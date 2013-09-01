#import "Utf8.h"
#import "PlaylistViewController.h"
#import <boost/filesystem.hpp>
#import "string_cast.h"
#import "PsfLoader.h"

@implementation PlaylistViewController

@synthesize delegate;

-(void)viewDidLoad
{
	m_playlist = nullptr;
	[super viewDidLoad];
}

-(void)onOpenPlaylist: (id)sender
{
	PlaylistSelectViewController* vc = (PlaylistSelectViewController*)[self.storyboard instantiateViewControllerWithIdentifier: @"PlaylistSelectViewController"];
	vc.delegate = self;
	[self presentViewController:vc animated: YES completion: nil];
}

-(void)onPlaylistSelected: (NSString*)selectedPlaylistPath
{
	delete m_playlist;
	m_playlist = new CPlaylist();
	
	{
		auto path = boost::filesystem::path([selectedPlaylistPath fileSystemRepresentation]);
		auto archive(CPsfArchive::CreateFromPath(path));
		
		unsigned int archiveId = m_playlist->InsertArchive(path.wstring().c_str());
		for(const auto& file : archive->GetFiles())
		{
			boost::filesystem::path filePath(file.name);
			std::string fileExtension = filePath.extension().string();
			if((fileExtension.length() != 0) && CPlaylist::IsLoadableExtension(fileExtension.c_str() + 1))
			{
				CPlaylist::ITEM newItem;
				newItem.path = filePath.wstring();
				newItem.title = string_cast<std::wstring>(newItem.path);
				newItem.length = 0;
				newItem.archiveId = archiveId;
				m_playlist->InsertItem(newItem);
			}
		}
	}
	
	[m_tableView reloadData];
	[m_tableView setContentOffset: CGPointZero animated: NO];
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
	assert(indexPath.row < m_playlist->GetItemCount());
	const CPlaylist::ITEM& item(m_playlist->GetItem(indexPath.row));
	
	if(self.delegate != nil)
	{
		[self.delegate onPlaylistItemSelected: item playlist: m_playlist];
	}
}

@end
