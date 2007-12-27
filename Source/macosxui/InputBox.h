#import <Cocoa/Cocoa.h>

@interface CInputBox : NSWindow
{
	IBOutlet NSTextField* m_inputText;
	IBOutlet NSControl* m_inputLabel;
}

-(void)doModal: (NSWindow*)parentWindow callbackObj: (id)callbackObj callback: (SEL)callback;
-(void)orderOut: (id)sender;
-(void)onOk : (id)sender;
-(void)setStringValue : (NSString*)value;
-(void)setLabelText : (NSString*)value;
-(NSString*)stringValue;

@end
