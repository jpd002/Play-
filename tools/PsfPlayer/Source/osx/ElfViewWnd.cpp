/*
 *  ElfViewWnd.cpp
 *  ElfView
 *
 *  Created by Jean-Philip Desjardins on 24/10/07.
 *  Copyright 2007 __MyCompanyName__. All rights reserved.
 *
 */

#include "ElfViewWnd.h"
#include "Elf.h"
#include "StdStream.h"
#include <string>

using namespace Framework;
using namespace std;

#define VIEW_SIGNATURE 'mVue'
#define VIEW_FIELDID 130

CElfViewWnd::CElfViewWnd() :
TWindow(CFSTR("ElfViewWnd"))
{
//	CStdStream stream(fopen("test.elf", "rb"));
//	CELF elf(&stream);
}

CElfViewWnd::~CElfViewWnd()
{

}

OSStatus DrawEventHandler(EventHandlerCallRef handler, EventRef eventRef, void* userData)
{
	OSStatus status = noErr;
	CGContextRef context;
	HIRect bounds;
	GetEventParameter(eventRef, kEventParamCGContextRef, typeCGContextRef, NULL, sizeof(CGContextRef), NULL, &context);
	HIViewGetBounds(reinterpret_cast<HIViewRef>(userData), &bounds);
	
	CGContextTranslateCTM(context, 0, bounds.size.height);
	CGContextScaleCTM(context, 1.0, -1.0);
	
//	CGContextSetRGBFillColor(context, 1, 0, 0, 1);
//	CGContextFillRect(context, CGRectMake(0, 0, bounds.size.width, bounds.size.height));
//	CGContextSetRGBFillColor(context, 0, 0, 1, 0.5);
//	CGContextFillRect(context, CGRectMake(0, 0, 100, 200));
	
	CGContextSelectFont(context, "Helvetica", 15, kCGEncodingMacRoman);
	CGContextSetTextDrawingMode(context, kCGTextFillStroke);
	CGContextSetRGBFillColor(context, 0, 0, 0, 1);
	CGContextSetRGBStrokeColor(context, 0, 0, 0, 0);
//	CGAffineTransform textTransform = CGAffineTransformMakeRotation(3.1416);
//	CGContextSetTextMatrix(context, textTransform);
	string text = "0x01EF0BAC   ADDIU R0, R0, 0x0004";
	const int height = 15;
	int lines = bounds.size.height + (height - 1) / height;
	
	int posY = bounds.size.height - height;
	for(unsigned int i = 0; i < lines; i++)
	{
		if(i == 2)
		{
			CGContextSetRGBFillColor(context, 1, 0, 0, 0.5);
			CGContextFillRect(context, CGRectMake(0, posY - 2, bounds.size.width, height));
		}
		
		CGContextSetRGBFillColor(context, 0, 0, 0, 1);
		CGContextSetRGBStrokeColor(context, 0, 0, 0, 0);
		CGContextShowTextAtPoint(context, 0, posY, text.c_str(), text.length());
		posY -= height;
	}
	
	return status;
}

void CElfViewWnd::Create()
{
    CElfViewWnd* wind = new CElfViewWnd();

    // Position new windows in a staggered arrangement on the main screen
//    RepositionWindow(*wind, NULL, kWindowCascadeOnMainScreen);
    
	HIViewID viewId = { VIEW_SIGNATURE, VIEW_FIELDID };
	EventTypeSpec viewEventSpec[] = { kEventClassControl, kEventControlDraw };

	HIViewRef view;
	
	HIViewFindByID(HIViewGetRoot(wind->GetWindowRef()), viewId, &view);
	InstallEventHandler(
		GetControlEventTarget(view), 
		NewEventHandlerUPP(DrawEventHandler), 
		GetEventTypeCount(viewEventSpec),
		viewEventSpec,
		reinterpret_cast<void*>(view),
		NULL);
		
    // The window was created hidden, so show it
    wind->Show();
}

Boolean CElfViewWnd::HandleCommand(const HICommandExtended& inCommand)
{
    switch (inCommand.commandID)
    {
	default:
		return false;
    }
}
