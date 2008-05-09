#include "Intc.h"
#include "Log.h"

#define LOG_NAME ("psx_intc")

using namespace Psx;

CIntc::CIntc()
{
	Reset();
}

CIntc::~CIntc()
{

}

void CIntc::Reset()
{
	m_mask = 0;
	m_status = 0;
}

void CIntc::AssertLine(unsigned int line)
{
	m_status |= (1 << line);
}

bool CIntc::IsInterruptPending() const
{
	return (m_mask & m_status) != 0;
}

uint32 CIntc::ReadRegister(uint32 address)
{
#ifdef _DEBUG
	DisassembleRead(address);
#endif
	switch(address)
	{
	case MASK:
		return m_mask;
		break;
	case STATUS:
		return m_status;
		break;
	}
	return 0;
}

void CIntc::WriteRegister(uint32 address, uint32 value)
{
#ifdef _DEBUG
	DisassembleWrite(address, value);
#endif
	switch(address)
	{
	case MASK:
		m_mask = value;
		break;
	case STATUS:
		m_status &= ~value;
		break;
	}
}

void CIntc::DisassembleRead(uint32 address)
{
	switch(address)
	{
	case STATUS:
		CLog::GetInstance().Print(LOG_NAME, "= STATUS\r\n");
		break;
	case MASK:
		CLog::GetInstance().Print(LOG_NAME, "= MASK\r\n");
		break;
	default:
		CLog::GetInstance().Print(LOG_NAME, "Read an unknown register (0x%0.8X).\r\n", address);
		break;
	}
}

void CIntc::DisassembleWrite(uint32 address, uint32 value)
{
	switch(address)
	{
	case STATUS:
		CLog::GetInstance().Print(LOG_NAME, "STATUS = 0x%0.8X\r\n", value);
		break;
	case MASK:
		CLog::GetInstance().Print(LOG_NAME, "MASK = 0x%0.8X\r\n", value);
		break;
	default:
		CLog::GetInstance().Print(LOG_NAME, "Wrote an unknown register (0x%0.8X, 0x%0.8X).\r\n",
			address, value);
		break;
	}
}
