#include "PadHandler.h"

void CPadHandler::InsertListener(CPadInterface* pListener)
{
	m_interfaces.push_back(pListener);
}

void CPadHandler::RemoveAllListeners()
{
	m_interfaces.clear();
}
