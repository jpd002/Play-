#include "Iop_Ilink.h"
#include "Iop_Intc.h"
#include "Log.h"
#include "../states/RegisterStateFile.h"

#define LOG_NAME ("iop_ilink")

#define STATE_REGS_XML ("iop_ilink/regs.xml")
#define STATE_REGS_CTRL2 ("CTRL2")
#define STATE_REGS_PHYRESULT ("PHYRESULT")
#define STATE_REGS_INTR0 ("INTR0")
#define STATE_REGS_INTR0MASK ("INTR0MASK")
#define STATE_REGS_INTR1 ("INTR1")
#define STATE_REGS_INTR1MASK ("INTR1MASK")
#define STATE_REGS_INTR2 ("INTR2")
#define STATE_REGS_INTR2MASK ("INTR2MASK")

using namespace Iop;

CIlink::CIlink(CIntc& intc)
    : m_intc(intc)
{
}

void CIlink::Reset()
{
	m_ctrl2 = 0;
	m_phyResult = 0;
	m_intr0 = 0;
	m_intr0Mask = 0;
	m_intr1 = 0;
	m_intr1Mask = 0;
	m_intr2 = 0;
	m_intr2Mask = 0;
}

void CIlink::LoadState(Framework::CZipArchiveReader& archive)
{
	CRegisterStateFile registerFile(*archive.BeginReadFile(STATE_REGS_XML));
	m_ctrl2 = registerFile.GetRegister32(STATE_REGS_CTRL2);
	m_phyResult = registerFile.GetRegister32(STATE_REGS_PHYRESULT);
	m_intr0 = registerFile.GetRegister32(STATE_REGS_INTR0);
	m_intr0Mask = registerFile.GetRegister32(STATE_REGS_INTR0MASK);
	m_intr1 = registerFile.GetRegister32(STATE_REGS_INTR1);
	m_intr1Mask = registerFile.GetRegister32(STATE_REGS_INTR1MASK);
	m_intr2 = registerFile.GetRegister32(STATE_REGS_INTR2);
	m_intr2Mask = registerFile.GetRegister32(STATE_REGS_INTR2MASK);
}

void CIlink::SaveState(Framework::CZipArchiveWriter& archive)
{
	CRegisterStateFile* registerFile = new CRegisterStateFile(STATE_REGS_XML);
	registerFile->SetRegister32(STATE_REGS_CTRL2, m_ctrl2);
	registerFile->SetRegister32(STATE_REGS_PHYRESULT, m_phyResult);
	registerFile->SetRegister32(STATE_REGS_INTR0, m_intr0);
	registerFile->SetRegister32(STATE_REGS_INTR0MASK, m_intr0Mask);
	registerFile->SetRegister32(STATE_REGS_INTR1, m_intr1);
	registerFile->SetRegister32(STATE_REGS_INTR1MASK, m_intr1Mask);
	registerFile->SetRegister32(STATE_REGS_INTR2, m_intr2);
	registerFile->SetRegister32(STATE_REGS_INTR2MASK, m_intr2Mask);
	archive.InsertFile(registerFile);
}

uint32 CIlink::ReadRegister(uint32 address)
{
	uint32 result = 0;
	switch(address)
	{
	case REG_NODEID:
		result = 1;
		break;
	case REG_PHY_ACCESS:
		result = m_phyResult;
		break;
	case REG_CTRL2:
		result = m_ctrl2;

		// If the device is powered (0x2), add
		// 0x80 to indicate that the device is ready.
		if(result & 0x2)
		{
			result |= REG_CTRL2_SOK;
		}
		break;
	case REG_INTR0:
		result = m_intr0;
		m_intr0 = 0;
		break;
	case REG_INTR0_MASK:
		result = m_intr0Mask;
		break;
	case REG_INTR1:
		result = m_intr1;
		m_intr1 = 0;
		break;
	case REG_INTR1_MASK:
		result = m_intr1Mask;
		break;
	case REG_INTR2:
		result = m_intr2;
		m_intr2 = 0;
		break;
	case REG_INTR2_MASK:
		result = m_intr2Mask;
		break;
	}
	LogRead(address);
	return result;
}

void CIlink::WriteRegister(uint32 address, uint32 value)
{
	switch(address)
	{
	case REG_CTRL2:
		m_ctrl2 = value;
		break;
	case REG_PHY_ACCESS:
	{
		uint32 phyReg = (value >> 24) & 0x3F;
		uint32 phyData = (value >> 16) & 0xFF;
		m_phyResult = (phyReg << 8);

		m_intr0 |= INTR0_PHYRRX;
		if(m_intr0 & m_intr0Mask)
		{
			m_intc.AssertLine(CIntc::LINE_ILINK);
		}
	}
	break;
	case REG_INTR0:
		m_intr0 &= ~value;
		break;
	case REG_INTR0_MASK:
		m_intr0Mask = value;
		break;
	case REG_INTR1:
		m_intr1 &= ~value;
		break;
	case REG_INTR1_MASK:
		m_intr1Mask = value;
		break;
	case REG_INTR2:
		m_intr2 = value;
		break;
	case REG_INTR2_MASK:
		m_intr2Mask = value;
		break;
	}
	LogWrite(address, value);
}

void CIlink::LogRead(uint32 address)
{
#define LOG_GET(registerId)                                           \
	case registerId:                                                  \
		CLog::GetInstance().Print(LOG_NAME, "= " #registerId "\r\n"); \
		break;

	switch(address)
	{
		LOG_GET(REG_NODEID)
		LOG_GET(REG_CTRL2)
		LOG_GET(REG_INTR0)
		LOG_GET(REG_INTR0_MASK)
		LOG_GET(REG_INTR1)
		LOG_GET(REG_INTR1_MASK)

	default:
		CLog::GetInstance().Warn(LOG_NAME, "Read an unknown register 0x%08X.\r\n", address);
		break;
	}
#undef LOG_GET
}

void CIlink::LogWrite(uint32 address, uint32 value)
{
#define LOG_SET(registerId)                                                      \
	case registerId:                                                             \
		CLog::GetInstance().Print(LOG_NAME, #registerId " = 0x%08X\r\n", value); \
		break;

	switch(address)
	{
		LOG_SET(REG_PHY_ACCESS)
		LOG_SET(REG_INTR0)
		LOG_SET(REG_INTR0_MASK)
		LOG_SET(REG_INTR1)
		LOG_SET(REG_INTR1_MASK)

	default:
		CLog::GetInstance().Warn(LOG_NAME, "Wrote 0x%08X to an unknown register 0x%08X.\r\n", value, address);
		break;
	}
#undef LOG_SET
}
