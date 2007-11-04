#ifndef _ELFVIEWWND_H_
#define _ELFVIEWWND_H_

#include "TWindow.h"

class CElfViewWnd : public TWindow
{
public:
							CElfViewWnd();
	virtual					~CElfViewWnd();
	
	static void				Create();
	
protected:
	virtual Boolean			HandleCommand(const HICommandExtended& inCommand);
};

#endif
