#import <UIKit/UIKit.h>
#import <vector>
#import "PsfTags.h"

typedef std::vector<std::string> TagStringList;

@interface FileInfoViewController : UITableViewController 
{
	CPsfTags			m_tags;
	TagStringList		m_rawTags;
}

-(void)setTags: (const CPsfTags&)tags;

@end
