#import <UIKit/UIKit.h>

@interface PlaylistSelectViewController : UITableViewController 
{
	id					m_selectionHandler;
	SEL					m_selectionHandlerSelector;
	NSMutableArray*		m_archives;
}

-(void)setSelectionHandler: (id)handler selector: (SEL)sel;
-(IBAction)onCancel: (id)handler;
-(NSString*)selectedPlaylistPath;

@end
