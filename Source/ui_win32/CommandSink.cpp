#include "CommandSink.h"

using namespace boost;
using namespace std;

CCommandSink::CCommandSink()
{

}

CCommandSink::~CCommandSink()
{

}

void CCommandSink::RegisterCallback(HWND hWnd, CallbackType Callback)
{
	m_Callbacks[hWnd] = Callback;
}

long CCommandSink::OnCommand(unsigned short nId, unsigned short nCmd, HWND hWnd)
{
	CallbackList::iterator itCallback;
	itCallback = m_Callbacks.find(hWnd);

	if(itCallback != m_Callbacks.end())
	{
		return (itCallback->second)();
	}

	return TRUE;
}
