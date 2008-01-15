#include "Iop_Intc.h"

using namespace Iop;

CIntc::CIntc() :
m_mask(0),
m_status(0)
{

}

CIntc::~CIntc()
{

}

void CIntc::AssertLine(unsigned int line)
{
    m_status |= 1 << line;
}

void CIntc::ClearLine(unsigned int line)
{
    m_status &= ~(1 << line);
}

void CIntc::SetMask(uint32 mask)
{
    m_mask = mask;
}

void CIntc::SetStatus(uint32 status)
{
    m_status = status;
}

bool CIntc::HasPendingInterrupt()
{
    return (m_mask & m_status) != 0;
}
