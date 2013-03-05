#include "Iop_Intc.h"
#include "../RegisterStateFile.h"

#define STATE_REGS_XML		("iop_intc/regs.xml")
#define STATE_REGS_STATUS	("STATUS")
#define STATE_REGS_MASK		("MASK")

using namespace Iop;

CIntc::CIntc() 
: m_mask(0)
, m_status(0)
{

}

CIntc::~CIntc()
{

}

void CIntc::Reset()
{
	m_status.f = 0;
	m_mask.f = 0;
}

void CIntc::LoadState(Framework::CZipArchiveReader& archive)
{
	CRegisterStateFile registerFile(*archive.BeginReadFile(STATE_REGS_XML));
	m_status.f	= registerFile.GetRegister64(STATE_REGS_STATUS);
	m_mask.f	= registerFile.GetRegister64(STATE_REGS_MASK);
}

void CIntc::SaveState(Framework::CZipArchiveWriter& archive)
{
	CRegisterStateFile* registerFile = new CRegisterStateFile(STATE_REGS_XML);
	registerFile->SetRegister64(STATE_REGS_STATUS, m_status.f);
	registerFile->SetRegister64(STATE_REGS_MASK, m_mask.f);
	archive.InsertFile(registerFile);
}

uint32 CIntc::ReadRegister(uint32 address)
{
	switch(address)
	{
	case MASK0:
		return m_mask.h0;
		break;
	case MASK1:
		return m_mask.h1;
		break;
	case STATUS0:
		return m_status.h0;
		break;
	case STATUS1:
		return m_status.h1;
		break;
	}
	return 0;
}

uint32 CIntc::WriteRegister(uint32 address, uint32 value)
{
	switch(address)
	{
	case MASK0:
		m_mask.h0 = value;
		break;
	case MASK1:
		m_mask.h1 = value;
		break;
	case STATUS0:
		m_status.h0 &= value;
		break;
	case STATUS1:
		m_status.h1 &= value;
		break;
	}
	return 0;
}

void CIntc::AssertLine(unsigned int line)
{
	m_status.f |= 1LL << line;
}

void CIntc::ClearLine(unsigned int line)
{
	m_status.f &= ~(1LL << line);
}

void CIntc::SetMask(uint64 mask)
{
	m_mask.f = mask;
}

void CIntc::SetStatus(uint64 status)
{
	m_status.f = status;
}

bool CIntc::HasPendingInterrupt()
{
	return (m_mask.f & m_status.f) != 0;
}
