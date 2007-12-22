#import "RegView.h"
#import "lexical_cast_ex.h"
#import "ObjCCall.h"
#import "Globals.h"
#import <string>

using namespace std;

@implementation CRegView

-(id)initWithFrame : (NSRect)frameRect
{
	m_context = NULL;
	return [super initWithFrame:frameRect];
}

-(bool)isFlipped
{
	return true;
}

-(void)drawRect : (NSRect)rect
{
	[[NSColor whiteColor] set];
	NSRectFill(rect);

	int xMargin = 2;
	NSFont* font = [NSFont fontWithName:@"Courier" size:14];
	float textHeight = [font defaultLineHeightForFont];

	if(m_context != NULL)
	{
		NSMutableDictionary *textAttrs = [NSMutableDictionary dictionary];
		[textAttrs setObject:font forKey:NSFontAttributeName];

		for(unsigned int i = 0; i < 32; i++)
		{
			string value;
			for(int j = 3; j >= 0; j--)
			{
				value += lexical_cast_hex<string>(m_context->m_State.nGPR[i].nV[j], 8);
			}
			NSString* regNameString = [NSString stringWithCString : CMIPS::m_sGPRName[i]];
			NSString* valueString = [NSString stringWithCString : value.c_str()];
			
			[regNameString drawAtPoint:NSMakePoint(xMargin +  0, i * textHeight) withAttributes:textAttrs];
			[valueString   drawAtPoint:NSMakePoint(xMargin + 25, i * textHeight) withAttributes:textAttrs];
		}
	}
}

-(void)setContext : (CMIPS*)context
{
	m_context = context;
	&g_virtualMachine->m_OnMachineStateChange.connect(ObjCCall(self, "onMachineStateChange"));
	[self setNeedsDisplay:true];
}

-(void)onMachineStateChange
{
	[self setNeedsDisplay:true];
}

@end
