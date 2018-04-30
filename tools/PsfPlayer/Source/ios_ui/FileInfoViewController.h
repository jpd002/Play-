#import <UIKit/UIKit.h>
#import <vector>
#import "PsfTags.h"

@protocol FileInfoViewControllerDelegate

- (void)onPlayButtonPress;
- (void)onPrevButtonPress;
- (void)onNextButtonPress;

@end

typedef std::vector<std::string> TagStringList;

@interface FileInfoViewController : UIViewController
{
	CPsfTags m_tags;
	TagStringList m_rawTags;
	IBOutlet UITableView* m_tagsTable;
	IBOutlet UILabel* m_trackTitleLabel;
	IBOutlet UILabel* m_trackTimeLabel;
	IBOutlet UIButton* m_playButton;
}

@property(nonatomic, assign) id<FileInfoViewControllerDelegate> delegate;

- (void)setTrackTitle:(NSString*)trackTitle;
- (void)setTrackTime:(NSString*)trackTime;
- (void)setPlayButtonText:(NSString*)playButtonText;
- (void)setTags:(const CPsfTags&)tags;
- (IBAction)onPlayButtonPress:(id)sender;
- (IBAction)onPrevButtonPress:(id)sender;
- (IBAction)onNextButtonPress:(id)sender;

@end
