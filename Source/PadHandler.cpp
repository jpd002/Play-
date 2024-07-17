#include "PadHandler.h"
#include <algorithm>

void CPadHandler::InsertListener(CPadInterface* listener)
{
	m_interfaces.push_back(listener);
}

bool CPadHandler::HasListener(CPadInterface* listener) const
{
	return std::find(m_interfaces.begin(), m_interfaces.end(), listener) != m_interfaces.end();
}

void CPadHandler::RemoveAllListeners()
{
	m_interfaces.clear();
}
