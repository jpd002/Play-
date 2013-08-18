#import <UIKit/UIKit.h>
#import "Playlist.h"

@interface PlaylistViewController : UITableViewController 
{
	id						m_selectionHandler;
	SEL						m_selectionHandlerSelector;	
	CPlaylist*				m_playlist;
}

-(void)setSelectionHandler: (id)handler selector: (SEL)sel;
-(void)setPlaylist: (CPlaylist*)playlist;
-(void)selectedPlaylistItem: (CPlaylist::ITEM*)itemPtr;

@end
