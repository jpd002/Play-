#import <UIKit/UIKit.h>

NS_ASSUME_NONNULL_BEGIN

@protocol SaveStateDelegate
- (void)saveStateUsingPosition:(uint32_t)position;
- (void)loadStateUsingPosition:(uint32_t)position;
@end

typedef NS_ENUM(NSInteger, SaveStateAction) {
	SaveStateActionSave,
	SaveStateActionLoad
};

@interface SaveStateViewController : UIViewController
@property(nonatomic, weak) id<SaveStateDelegate> delegate;
@property(nonatomic, assign) SaveStateAction action;

@property(retain, nonatomic) IBOutlet UILabel* label;
@property(retain, nonatomic) IBOutlet UISegmentedControl* saveSlotSegmentedControl;
@property(retain, nonatomic) IBOutlet UIButton* button;

@end

NS_ASSUME_NONNULL_END
