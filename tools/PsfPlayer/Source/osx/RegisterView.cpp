#include "RegisterView.h"
#include "lexical_cast_ex.h"

using namespace std;

CRegisterView::CRegisterView(const HIViewRef& viewRef, CMIPS& cpu) :
m_viewRef(viewRef),
m_cpu(cpu)
{	
	EventTypeSpec viewEventSpec[] = { kEventClassControl, kEventControlDraw };

	InstallEventHandler(
		GetControlEventTarget(viewRef), 
		NewEventHandlerUPP(DrawEventHandlerStub), 
		GetEventTypeCount(viewEventSpec),
		viewEventSpec,
		reinterpret_cast<void*>(this),
		NULL);
}

CRegisterView::~CRegisterView()
{

}

void CRegisterView::SendRedraw()
{
	HIViewSetNeedsDisplay(m_viewRef, true);
}

OSStatus CRegisterView::DrawEventHandlerStub(EventHandlerCallRef handler, EventRef eventRef, void* param)
{
	return reinterpret_cast<CRegisterView*>(param)->DrawEventHandler(handler, eventRef);
}

OSStatus CRegisterView::DrawEventHandler(EventHandlerCallRef handler, EventRef eventRef)
{
	OSStatus status = noErr;
	CGContextRef context;
	HIRect bounds;
	GetEventParameter(eventRef, kEventParamCGContextRef, typeCGContextRef, NULL, sizeof(CGContextRef), NULL, &context);
	HIViewGetBounds(m_viewRef, &bounds);
	
	CGContextTranslateCTM(context, 0, bounds.size.height);
	CGContextScaleCTM(context, 1.0, -1.0);

	CGContextSetRGBFillColor(context, 1, 1, 1, 1);
	CGContextFillRect(context, CGRectMake(0, 0, bounds.size.width, bounds.size.height));

	CGContextSelectFont(context, "Helvetica", 15, kCGEncodingMacRoman);
	CGContextSetTextDrawingMode(context, kCGTextFillStroke);
	CGContextSetRGBFillColor(context, 0, 0, 0, 1);
	CGContextSetRGBStrokeColor(context, 0, 0, 0, 0);
	
	const int height = 15;
	int posY = bounds.size.height - height;
	
	for(unsigned int i = 0; i < 32; i++)
	{
		string registerName = CMIPS::m_sGPRName[i] + string(":");
		string registerValue = "0x" + lexical_cast_hex<string>(m_cpu.m_State.nGPR[i].nV0, 8);
		
		CGContextSetRGBFillColor(context, 0, 0, 0, 1);
		CGContextSetRGBStrokeColor(context, 0, 0, 0, 0);
		
		CGContextShowTextAtPoint(context, 0, posY, registerName.c_str(), registerName.length());
		CGContextShowTextAtPoint(context, 100, posY, registerValue.c_str(), registerValue.length());
		
		posY -= height;
	}
	
	return status;
}
