#include "PadHandler.h"

CPadHandler::CPadHandler()
{

}

CPadHandler::~CPadHandler()
{

}

void CPadHandler::InsertListener(CPadListener* pListener)
{
	m_Listener.Insert(pListener);
}

void CPadHandler::RemoveAllListeners()
{
	while(m_Listener.Count() != 0)
	{
		m_Listener.Pull();
	}
}
