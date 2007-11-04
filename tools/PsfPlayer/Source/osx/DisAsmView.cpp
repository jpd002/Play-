#include "DisAsmView.h"
#include "lexical_cast_ex.h"
#include "countof.h"
#include <string>

using namespace std;

CDisAsmView::CDisAsmView(const HIViewRef& viewRef, CMIPS& cpu) :
m_viewRef(viewRef),
m_lineCount(1),
m_cpu(cpu)
{
	m_viewAddress = m_cpu.m_State.nPC;
	
	EventTypeSpec viewEventSpec[] = { kEventClassControl, kEventControlDraw };

	InstallEventHandler(
		GetControlEventTarget(viewRef), 
		NewEventHandlerUPP(DrawEventHandlerStub), 
		GetEventTypeCount(viewEventSpec),
		viewEventSpec,
		reinterpret_cast<void*>(this),
		NULL);
}

CDisAsmView::~CDisAsmView()
{

}

void CDisAsmView::EnsureVisible(uint32 address)
{
	if((address < m_viewAddress) || (address >= m_viewAddress + (m_lineCount * 4)))
	{
		m_viewAddress = address;
	}
	SendRedraw();
}

void CDisAsmView::SendRedraw()
{
	HIViewSetNeedsDisplay(m_viewRef, true);
}

OSStatus CDisAsmView::DrawEventHandlerStub(EventHandlerCallRef handler, EventRef eventRef, void* param)
{
	return reinterpret_cast<CDisAsmView*>(param)->DrawEventHandler(handler, eventRef);
}

OSStatus CDisAsmView::DrawEventHandler(EventHandlerCallRef handler, EventRef eventRef)
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
//	CGContextSetRGBFillColor(context, 0, 0, 1, 0.5);
//	CGContextFillRect(context, CGRectMake(0, 0, 100, 200));
	
	CGContextSelectFont(context, "Helvetica", 15, kCGEncodingMacRoman);
	CGContextSetTextDrawingMode(context, kCGTextFillStroke);
	CGContextSetRGBFillColor(context, 0, 0, 0, 1);
	CGContextSetRGBStrokeColor(context, 0, 0, 0, 0);
	
	const int height = 15;
	m_lineCount = (bounds.size.height + (height - 1)) / height;
	int posY = bounds.size.height - height;
	uint32 address = m_viewAddress;
	
	for(unsigned int i = 0; i < m_lineCount; i++)
	{
		if(address == m_cpu.m_State.nPC)
		{
			CGContextSetRGBFillColor(context, 1, 0, 0, 0.5);
			CGContextFillRect(context, CGRectMake(0, posY - 2, bounds.size.width, height));
		}
		
		uint32 opcode = m_cpu.m_pMemoryMap->GetWord(address);
		char mnemonicText[256];
		char operandsText[256];
		
		string addressText = lexical_cast_hex<string>(address, 8);
		string opcodeText = lexical_cast_hex<string>(opcode, 8);
		m_cpu.m_pArch->GetInstructionMnemonic(&m_cpu, address, opcode, mnemonicText, countof(mnemonicText));
		m_cpu.m_pArch->GetInstructionOperands(&m_cpu, address, opcode, operandsText, countof(operandsText));
		
		CGContextSetRGBFillColor(context, 0, 0, 0, 1);
		CGContextSetRGBStrokeColor(context, 0, 0, 0, 0);
		
		CGContextShowTextAtPoint(context, 0, posY, addressText.c_str(), addressText.length());
		CGContextShowTextAtPoint(context, 100, posY, opcodeText.c_str(), opcodeText.length());
		CGContextShowTextAtPoint(context, 200, posY, mnemonicText, strlen(mnemonicText));
		CGContextShowTextAtPoint(context, 300, posY, operandsText, strlen(operandsText));
		
		posY -= height;
		address += 4;
	}
	
	return status;
}
