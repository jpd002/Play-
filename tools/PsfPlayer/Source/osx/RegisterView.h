#ifndef _REGISTERVIEW_H_
#define _REGISTERVIEW_H_

#include <Carbon/Carbon.h>
#include "MIPS.h"

class CRegisterView
{
public:
						CRegisterView(const HIViewRef&, CMIPS&);
	virtual				~CRegisterView();
	
	void				SendRedraw();

private:
	OSStatus			DrawEventHandler(EventHandlerCallRef, EventRef);
	static OSStatus		DrawEventHandlerStub(EventHandlerCallRef, EventRef, void*);

	HIViewRef			m_viewRef;
	CMIPS&				m_cpu;
};

#endif
