#import "Utf8.h"
#import "PlaylistViewController.h"
#import <boost/filesystem.hpp>
#import "string_cast.h"
#import "PsfLoader.h"
#import "TimeToString.h"

@implementation PlaylistViewController

@synthesize delegate;

-(void)viewDidLoad
{
	m_playlist = nullptr;
	m_playlistDiscoveryService = new CPlaylistDiscoveryService();
	m_playingItemIndex = -1;
	[NSTimer scheduledTimerWithTimeInterval: 0.1 target: self selector: @selector(onUpdateDiscoveryItems) userInfo: nil repeats: YES];
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
	m_playlistDiscoveryService->ResetRun();
	
	{
		auto path = boost::filesystem::path([selectedPlaylistPath fileSystemRepresentation]);
		auto archive(CPsfArchive::CreateFromPath(path));
		
		unsigned int archiveId = m_playlist->InsertArchive(path.wstring().c_str());
		for(const auto& file : archive->GetFiles())
		{
			auto filePath = CArchivePsfStreamProvider::GetPathTokenFromFilePath(file.name);
			auto fileExtension = CPsfStreamProvider::GetPathTokenExtension(filePath);
			if(!fileExtension.empty() && CPlaylist::IsLoadableExtension(fileExtension))
			{
				CPlaylist::ITEM newItem;
				newItem.path = filePath;
				newItem.title = filePath;
				newItem.length = 0;
				newItem.archiveId = archiveId;
				unsigned int itemId = m_playlist->InsertItem(newItem);
				
				m_playlistDiscoveryService->AddItemInRun(filePath, path, itemId);
			}
		}
	}
	
	[m_tableView reloadData];
	[m_tableView setContentOffset: CGPointZero animated: NO];
	
	if(self.delegate != nil)
	{
		[self.delegate onPlaylistSelected: m_playlist];
	}
}

-(void)setPlayingItemIndex: (unsigned int)playingItemIndex
{
	m_playingItemIndex = playingItemIndex;
	[m_tableView reloadData];
}

-(void)onUpdateDiscoveryItems
{
	if(m_playlist)
	{
		bool hasUpdate = false;
		auto onItemUpdateConnection = m_playlist->OnItemUpdate.connect(
			[&hasUpdate](unsigned int itemId, const CPlaylist::ITEM& item)
			{
				hasUpdate = true;
			});
		m_playlistDiscoveryService->ProcessPendingItems(*m_playlist);
		onItemUpdateConnection.disconnect();
		//NSLog(@"Updating %d", hasUpdate);
		if(hasUpdate)
		{
			[m_tableView reloadData];
		}
	}
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
		cell = [[[UITableViewCell alloc] initWithStyle: UITableViewCellStyleValue1 reuseIdentifier: CellIdentifier] autorelease];
	}
	
	if(m_playlist != NULL)
	{
		const CPlaylist::ITEM& item(m_playlist->GetItem(indexPath.row));
		std::string titleString = Framework::Utf8::ConvertTo(item.title);
		std::string lengthString = TimeToString<std::string>(item.length);
		cell.textLabel.text = [NSString stringWithUTF8String: titleString.c_str()];
		if(m_playingItemIndex == indexPath.row)
		{
			[cell.textLabel setFont: [UIFont boldSystemFontOfSize: 16]];
		}
		else
		{
			[cell.textLabel setFont: [UIFont systemFontOfSize: 16]];
		}
		cell.detailTextLabel.text = [NSString stringWithUTF8String: lengthString.c_str()];
	}
	
	return cell;
}

-(void)tableView: (UITableView*)tableView didSelectRowAtIndexPath: (NSIndexPath*)indexPath
{
	assert(indexPath.row < m_playlist->GetItemCount());
	if(self.delegate != nil)
	{
		[self.delegate onPlaylistItemSelected: indexPath.row];
	}
}

@end
