#import "SaveStateViewController.h"

@interface SaveStateViewController ()

@end

@implementation SaveStateViewController

- (void)viewDidLoad
{
	[super viewDidLoad];
	UITapGestureRecognizer* tap = [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(didTapOutside:)];
	[self.view addGestureRecognizer:tap];
	self.button.layer.borderColor = UIColor.whiteColor.CGColor;
	self.button.layer.borderWidth = 1.0;
	[self update];
}

- (void)setAction:(SaveStateAction)action
{
	_action = action;
	[self update];
}

- (void)update
{
	NSString* actionText = nil;
	switch(self.action)
	{
	case SaveStateActionSave:
		actionText = @"Save";
		break;
	case SaveStateActionLoad:
		actionText = @"Load";
		break;
	default:
		break;
	}
	self.label.text = [NSString stringWithFormat:@"Select a state to %@:", actionText];
	[self.button setTitle:actionText forState:UIControlStateNormal];
}

- (IBAction)didTapButton:(id)sender
{
	uint32_t slot = UInt32(self.saveSlotSegmentedControl.selectedSegmentIndex);
	switch(self.action)
	{
	case SaveStateActionSave:
		[self.delegate saveStateUsingPosition:slot];
		break;
	case SaveStateActionLoad:
		[self.delegate loadStateUsingPosition:slot];
		break;
	default:
		break;
	}
	self.view.hidden = YES;
}

- (void)didTapOutside:(id)sender
{
	self.view.hidden = YES;
}

@end
