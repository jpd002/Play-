#include "Iop_Speed.h"
#include "Log.h"

#define LOG_NAME ("iop_speed")

using namespace Iop;

void CSpeed::Reset()
{
	m_smapEmac3StaCtrl.f = 0;
}

uint32 CSpeed::ReadRegister(uint32 address)
{
	uint32 result = 0;
	switch(address)
	{
	case REG_REV1:
		//SMAP driver checks this
		result = 17;
		break;
	case REG_REV3:
		result = SPEED_CAPS_SMAP;
		break;
	case REG_INTR_STAT:
		result = (1 << INTR_SMAP_TXDNV) | (1 << INTR_SMAP_TXEND) | (1 << INTR_SMAP_RXEND);
		break;
	case REG_SMAP_EMAC3_STA_CTRL_HI:
		result = m_smapEmac3StaCtrl.h1;
		break;
	case REG_SMAP_EMAC3_STA_CTRL_LO:
		result = m_smapEmac3StaCtrl.h0;
		break;
	}
	LogRead(address);
	return result;
}

void CSpeed::WriteRegister(uint32 address, uint32 value)
{
	switch(address)
	{
	case REG_SMAP_EMAC3_STA_CTRL_HI:
		m_smapEmac3StaCtrl.h1 = static_cast<uint16>(value);
		break;
	case REG_SMAP_EMAC3_STA_CTRL_LO:
		m_smapEmac3StaCtrl.h0 = static_cast<uint16>(value);
		{
			auto staCtrl = make_convertible<SMAP_EMAC3_STA_CTRL>(m_smapEmac3StaCtrl.f);
			//SMAP_DsPHYTER_ADDRESS is 1
			assert(staCtrl.phyAddr == 1);
			//phyRegAddr
			//SMAP_DsPHYTER_BMCR is 0
			switch(staCtrl.phyStaCmd)
			{
			case SMAP_EMAC3_STA_CMD_READ:
				CLog::GetInstance().Print(LOG_NAME, "SMAP_EMAC3: Reading reg %02X.\r\n", staCtrl.phyRegAddr);
				assert(staCtrl.phyRegAddr == 0);
				break;
			case SMAP_EMAC3_STA_CMD_WRITE:
				CLog::GetInstance().Print(LOG_NAME, "SMAP_EMAC3: Writing %04X to reg %02X.\r\n", staCtrl.phyData, staCtrl.phyRegAddr);
				break;
			default:
				assert(false);
				break;
			}
			staCtrl.phyOpComp = 1;
			m_smapEmac3StaCtrl.f = staCtrl;
		}
		break;
	}
	LogWrite(address, value);
}

void CSpeed::LogRead(uint32 address)
{
#define LOG_GET(registerId)                                           \
	case registerId:                                                  \
		CLog::GetInstance().Print(LOG_NAME, "= " #registerId "\r\n"); \
		break;

	switch(address)
	{
		LOG_GET(REG_REV1)
		LOG_GET(REG_REV3)
		LOG_GET(REG_INTR_STAT)
		LOG_GET(REG_SMAP_EMAC3_STA_CTRL_HI)
		LOG_GET(REG_SMAP_EMAC3_STA_CTRL_LO)

	default:
		CLog::GetInstance().Warn(LOG_NAME, "Read an unknown register 0x%08X.\r\n", address);
		break;
	}
}

void CSpeed::LogWrite(uint32 address, uint32 value)
{
#define LOG_SET(registerId)                                                      \
	case registerId:                                                             \
		CLog::GetInstance().Print(LOG_NAME, #registerId " = 0x%08X\r\n", value); \
		break;

	switch(address)
	{
		LOG_SET(REG_INTR_STAT)
		LOG_SET(REG_SMAP_TXFIFO_DATA)
		LOG_SET(REG_SMAP_EMAC3_STA_CTRL_LO)
		LOG_SET(REG_SMAP_EMAC3_STA_CTRL_HI)

	default:
		CLog::GetInstance().Warn(LOG_NAME, "Wrote 0x%08X to an unknown register 0x%08X.\r\n", value, address);
		break;
	}
}
