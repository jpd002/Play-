#include "ArgumentIterator.h"

CCallArgumentIterator::CCallArgumentIterator(CMIPS& context) 
: m_context(context)
, m_current(0)
{

}

uint32 CCallArgumentIterator::GetNext()
{
	if(m_current > 3)
	{
		uint32 address = m_context.m_State.nGPR[CMIPS::SP].nV0 + (m_current++ - 4) * 4 + 0x10;
		return m_context.m_pMemoryMap->GetWord(address);
	}
	else
	{
		return m_context.m_State.nGPR[CMIPS::A0 + m_current++].nV[0];
	}
}

CValistArgumentIterator::CValistArgumentIterator(CMIPS& context, uint32 argsPtr)
: m_context(context)
, m_argsPtr(argsPtr)
{

}

uint32 CValistArgumentIterator::GetNext()
{
	uint32 arg = m_context.m_pMemoryMap->GetWord(m_argsPtr);
	m_argsPtr += 4;
	return arg;
}
