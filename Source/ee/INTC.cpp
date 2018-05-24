#include "INTC.h"
#include "../Log.h"
#include "../RegisterStateFile.h"

#define LOG_NAME ("intc")

#define STATE_REGS_XML ("intc/regs.xml")

CINTC::CINTC(CDMAC& dmac, CGSHandler*& gs)
    : m_INTC_STAT(0)
    , m_INTC_MASK(0)
    , m_dmac(dmac)
    , m_gs(gs)
{
}

CINTC::~CINTC()
{
}

void CINTC::Reset()
{
	m_INTC_STAT = 0;
	m_INTC_MASK = 0;
}

uint32 CINTC::GetStat() const
{
	uint32 tempStat = m_INTC_STAT;

	if((m_gs != nullptr) && m_gs->IsInterruptPending())
	{
		tempStat |= (1 << INTC_LINE_GS);
	}

	if(m_dmac.IsInterruptPending())
	{
		tempStat |= (1 << INTC_LINE_DMAC);
	}

	return tempStat;
}

bool CINTC::IsInterruptPending() const
{
	return (GetStat() & m_INTC_MASK) != 0;
}

uint32 CINTC::GetRegister(uint32 nAddress)
{
	switch(nAddress)
	{
	case INTC_STAT:
		return GetStat();
		break;
	case INTC_MASK:
		return m_INTC_MASK;
		break;
	default:
		CLog::GetInstance().Warn(LOG_NAME, "Read an unhandled register (0x%08X).\r\n", nAddress);
		break;
	}

	return 0;
}

void CINTC::SetRegister(uint32 nAddress, uint32 nValue)
{
	switch(nAddress)
	{
	case INTC_STAT:
		m_INTC_STAT &= ~nValue;
		break;
	case INTC_MASK:
		m_INTC_MASK ^= nValue;
		break;
	default:
		CLog::GetInstance().Warn(LOG_NAME, "Wrote to an unhandled register (0x%08X).\r\n", nAddress);
		break;
	}
}

void CINTC::AssertLine(uint32 nLine)
{
	m_INTC_STAT |= (1 << nLine);
}

void CINTC::LoadState(Framework::CZipArchiveReader& archive)
{
	CRegisterStateFile registerFile(*archive.BeginReadFile(STATE_REGS_XML));
	m_INTC_STAT = registerFile.GetRegister32("INTC_STAT");
	m_INTC_MASK = registerFile.GetRegister32("INTC_MASK");
}

void CINTC::SaveState(Framework::CZipArchiveWriter& archive)
{
	CRegisterStateFile* registerFile = new CRegisterStateFile(STATE_REGS_XML);
	registerFile->SetRegister32("INTC_STAT", m_INTC_STAT);
	registerFile->SetRegister32("INTC_MASK", m_INTC_MASK);
	archive.InsertFile(registerFile);
}
