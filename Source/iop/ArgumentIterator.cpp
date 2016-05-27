#include "ArgumentIterator.h"

CArgumentIterator::CArgumentIterator(CMIPS& context) :
m_context(context),
m_current(0),
m_args(nullptr)
{
   
}

CArgumentIterator::CArgumentIterator(CMIPS& context, const char* args) :
	m_context(context),
	m_current(0),
	m_args(args)
{

}

CArgumentIterator::~CArgumentIterator()
{

}

uint32 CArgumentIterator::GetNext()
{
	if (m_args)
	{
		// m_args is not null if we have a va_list style of aguments.
		// In this case, we never read from registers or the stack pointer.
		const char* address = m_args + 4 * m_current;
		m_current++;
		return *((uint32*)address);
	}
	else if(m_current > 3)
    {
        uint32 address = m_context.m_State.nGPR[CMIPS::SP].nV0 + (m_current++ - 4) * 4 + 0x10;
        return m_context.m_pMemoryMap->GetWord(address);
    }
    else
    {
        return m_context.m_State.nGPR[CMIPS::A0 + m_current++].nV[0];
    }
}
