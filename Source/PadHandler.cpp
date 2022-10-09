#include "PadHandler.h"

void CPadHandler::InsertListener(CPadListener* pListener)
{
	m_listeners.push_back(pListener);
}

void CPadHandler::RemoveAllListeners()
{
	m_listeners.clear();
}
