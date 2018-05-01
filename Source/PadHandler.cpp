#include "PadHandler.h"

CPadHandler::CPadHandler()
{
}

CPadHandler::~CPadHandler()
{
}

void CPadHandler::InsertListener(CPadListener* pListener)
{
	m_listeners.push_back(pListener);
}

void CPadHandler::RemoveAllListeners()
{
	m_listeners.clear();
}
