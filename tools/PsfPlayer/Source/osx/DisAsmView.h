#ifndef _DISASMVIEW_H_
#define _DISASMVIEW_H_

#include <Carbon/Carbon.h>
#include "MIPS.h"

class CDisAsmView
{
public:
						CDisAsmView(const HIViewRef&, CMIPS&);
	virtual				~CDisAsmView();
	
	void				EnsureVisible(uint32);
	void				SendRedraw();

private:
	OSStatus			DrawEventHandler(EventHandlerCallRef, EventRef);
	static OSStatus		DrawEventHandlerStub(EventHandlerCallRef, EventRef, void*);

	HIViewRef			m_viewRef;
	CMIPS&				m_cpu;
	uint32				m_viewAddress;
	unsigned int		m_lineCount;
};

#endif
