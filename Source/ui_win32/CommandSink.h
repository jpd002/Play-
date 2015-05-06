#ifndef _COMMANDSINK_H_
#define _COMMANDSINK_H_

#include "win32/Window.h"
#include <map>
#include <boost/function.hpp>

class CCommandSink
{
public:
	typedef boost::function< long () > CallbackType;

						CCommandSink();
	virtual				~CCommandSink();

	void				RegisterCallback(HWND, CallbackType);
	long				OnCommand(unsigned short, unsigned short, HWND);

private:

	typedef std::map<HWND, CallbackType> CallbackList;

	CallbackList		m_Callbacks;


};

#endif
