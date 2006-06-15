#ifndef _PADHANDLER_H_
#define _PADHANDLER_H_

#include "PadListener.h"
#include "List.h"

class CPadHandler
{
public:
										CPadHandler();
	virtual								~CPadHandler();
	virtual void						Update() = 0;
	void								InsertListener(CPadListener*);
	void								RemoveAllListeners();

protected:
	Framework::CList<CPadListener>		m_Listener;
};

#endif
