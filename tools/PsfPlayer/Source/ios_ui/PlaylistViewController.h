#import <UIKit/UIKit.h>
#import "Playlist.h"
#import "PlaylistDiscoveryService.h"
#import "PlaylistSelectViewController.h"

@protocol PlaylistViewControllerDelegate

-(void)onPlaylistSelected: (CPlaylist*)playlist;
-(void)onPlaylistItemSelected: (unsigned int)itemIndex;

@end

@interface PlaylistViewController : UIViewController<PlaylistSelectViewControllerDelegate>
{
	CPlaylist*					m_playlist;
	CPlaylistDiscoveryService*	m_playlistDiscoveryService;
	IBOutlet UITableView*		m_tableView;
	unsigned int				m_playingItemIndex;
}

@property (nonatomic, assign) IBOutlet id<PlaylistViewControllerDelegate> delegate;

-(IBAction)onOpenPlaylist: (id)sender;
-(void)setPlayingItemIndex: (unsigned int)playingItemIndex;

@end
