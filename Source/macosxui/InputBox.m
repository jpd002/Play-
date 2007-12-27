#import "InputBox.h"

@implementation CInputBox

-(void)doModal: (NSWindow*)parentWindow callbackObj: (id)callbackObj callback: (SEL)callback
{
	[NSApp beginSheet: self modalForWindow: parentWindow modalDelegate: callbackObj didEndSelector: callback contextInfo: nil];	
}

-(void)onOk: (id)sender
{
	[NSApp endSheet: self returnCode: YES];
	[super orderOut: sender];
}

-(void)orderOut: (id)sender
{
	[NSApp endSheet: self returnCode: NO];
	[super orderOut: sender];
}

-(void)setLabelText: (NSString*)value
{
	[m_inputLabel setStringValue: value];
}

-(void)setStringValue: (NSString*)value
{
	[m_inputText setStringValue: value];
}

-(NSString*)stringValue
{
	return [m_inputText stringValue];
}

@end
