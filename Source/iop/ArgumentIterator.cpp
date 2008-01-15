#include "ArgumentIterator.h"

CArgumentIterator::CArgumentIterator(CMIPS& context) :
m_context(context),
m_current(0)
{
   
}

CArgumentIterator::~CArgumentIterator()
{

}

uint32 CArgumentIterator::GetNext()
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
