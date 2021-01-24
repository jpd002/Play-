#include "Iop_Speed.h"
#include "Log.h"

#define LOG_NAME ("iop_speed")

using namespace Iop;

const uint16 CSpeed::m_eepromData[] =
{
	//MAC address
	0x1122, 0x1122, 0x4466,
	//Checksum (just the sum of words above)
	0x66AA
};

void CSpeed::Reset()
{
	m_smapEmac3StaCtrl.f = 0;
}

void CSpeed::ProcessEmac3StaCtrl()
{
	auto staCtrl = make_convertible<SMAP_EMAC3_STA_CTRL>(m_smapEmac3StaCtrl.f);
	//phyAddr:
	//1: SMAP_DsPHYTER_ADDRESS
	//phyRegAddr:
	//0: SMAP_DsPHYTER_BMCR
	//1: SMAP_DsPHYTER_BMSR
	//4: SMAP_DsPHYTER_ANAR
	//BMCR bits
	//15: SMAP_PHY_BMCR_RST
	//12: SMAP_PHY_BMCR_ANEN //Enable auto-nego
	//9 : SMAP_PHY_BMCR_RSAN //Restart auto-nego
	//BSMR bits
	//2: SMAP_PHY_BMSR_LINK
	//5: SMAP_PHY_BMSR_ANCP (Auto-Nego Complete)
	switch(staCtrl.phyStaCmd)
	{
	case 0:
		break;
	case SMAP_EMAC3_STA_CMD_READ:
		CLog::GetInstance().Print(LOG_NAME, "SMAP_PHY: Reading reg 0x%02X.\r\n", staCtrl.phyRegAddr);
		assert(staCtrl.phyAddr == 1);
		switch(staCtrl.phyRegAddr)
		{
		case 0:
			staCtrl.phyData = 0;
			break;
		case 1:
			staCtrl.phyData = (1 << 2) | (1 << 5);
			break;
		case 4:
			staCtrl.phyData = 0;
			break;
		default:
			//assert(false);
			break;
		}
		staCtrl.phyOpComp = 1;
		break;
	case SMAP_EMAC3_STA_CMD_WRITE:
		CLog::GetInstance().Print(LOG_NAME, "SMAP_PHY: Writing 0x%04X to reg 0x%02X.\r\n", staCtrl.phyData, staCtrl.phyRegAddr);
		assert(staCtrl.phyAddr == 1);
		//assert(staCtrl.phyRegAddr == 0);
		staCtrl.phyOpComp = 1;
		break;
	default:
		assert(false);
		break;
	}
	m_smapEmac3StaCtrl.f = staCtrl;
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
	case REG_INTR_MASK:
		result = m_intrMask;
		break;
	case REG_PIO_DATA:
		{
			assert(m_eepRomReadIndex < ((m_eepRomDataSize * 16) + 1));
			if(m_eepRomReadIndex == 0)
			{
				result = 0;
			}
			else
			{
				assert(m_eepRomReadIndex >= 1);
				uint32 wordIndex = (m_eepRomReadIndex - 1) / 16;
				uint32 wordBit = 15 - ((m_eepRomReadIndex - 1) % 16);
				result = (m_eepromData[wordIndex] & (1 << wordBit)) ? 0x10 : 0;
			}
			m_eepRomReadIndex++;
		}
		break;
	case REG_SMAP_EMAC3_STA_CTRL_HI:
		result = m_smapEmac3StaCtrl.h1 | (m_smapEmac3StaCtrl.h0 << 16);
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
	case REG_INTR_MASK:
		m_intrMask = value;
		break;
	case REG_PIO_DIR:
		if(value == 0xE1)
		{
			//Reset reading process
			m_eepRomReadIndex = 0;
		}
		break;
	case REG_SMAP_EMAC3_STA_CTRL_HI:
		m_smapEmac3StaCtrl.h1 = static_cast<uint16>(value);
		m_smapEmac3StaCtrl.h0 = static_cast<uint16>(value >> 16);
		ProcessEmac3StaCtrl();
		break;
	case REG_SMAP_EMAC3_STA_CTRL_LO:
		m_smapEmac3StaCtrl.h0 = static_cast<uint16>(value);
		ProcessEmac3StaCtrl();
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

	if((address >= REG_SMAP_BD_TX_BASE) && (address < (REG_SMAP_BD_TX_BASE + SMAP_BD_SIZE)))
	{
		LogBdRead("REG_SMAP_BD_TX", REG_SMAP_BD_TX_BASE, address);
		return;
	}
	
	if((address >= REG_SMAP_BD_RX_BASE) && (address < (REG_SMAP_BD_RX_BASE + SMAP_BD_SIZE)))
	{
		LogBdRead("REG_SMAP_BD_RX", REG_SMAP_BD_RX_BASE, address);
		return;
	}

	switch(address)
	{
		LOG_GET(REG_REV1)
		LOG_GET(REG_REV3)
		LOG_GET(REG_INTR_STAT)
		LOG_GET(REG_INTR_MASK)
		LOG_GET(REG_PIO_DATA)
		LOG_GET(REG_SMAP_EMAC3_TXMODE0_HI)
		LOG_GET(REG_SMAP_EMAC3_TXMODE0_LO)
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

	if((address >= REG_SMAP_BD_TX_BASE) && (address < (REG_SMAP_BD_TX_BASE + SMAP_BD_SIZE)))
	{
		LogBdWrite("REG_SMAP_BD_TX", REG_SMAP_BD_TX_BASE, address, value);
		return;
	}
	
	if((address >= REG_SMAP_BD_RX_BASE) && (address < (REG_SMAP_BD_RX_BASE + SMAP_BD_SIZE)))
	{
		LogBdWrite("REG_SMAP_BD_RX", REG_SMAP_BD_RX_BASE, address, value);
		return;
	}
	
	switch(address)
	{
		LOG_SET(REG_INTR_STAT)
		LOG_SET(REG_INTR_MASK)
		LOG_SET(REG_PIO_DIR)
		LOG_SET(REG_PIO_DATA)
		LOG_SET(REG_SMAP_TXFIFO_FRAME_INC)
		LOG_SET(REG_SMAP_TXFIFO_DATA)
		LOG_SET(REG_SMAP_EMAC3_TXMODE0_HI)
		LOG_SET(REG_SMAP_EMAC3_TXMODE0_LO)
		LOG_SET(REG_SMAP_EMAC3_STA_CTRL_HI)
		LOG_SET(REG_SMAP_EMAC3_STA_CTRL_LO)

	default:
		CLog::GetInstance().Warn(LOG_NAME, "Wrote 0x%08X to an unknown register 0x%08X.\r\n", value, address);
		break;
	}
}

void CSpeed::LogBdRead(const char* name, uint32 base, uint32 address)
{
	uint32 regIndex = address & 0x7;
	uint32 structIndex = (address - base) / 8;
	switch(regIndex)
	{
	case 0:
		CLog::GetInstance().Print(LOG_NAME, "= %s[%d].CTRLSTAT\r\n", name, structIndex);
		break;
	case 2:
		CLog::GetInstance().Print(LOG_NAME, "= %s[%d].RESERVED\r\n", name, structIndex);
		break;
	case 4:
		CLog::GetInstance().Print(LOG_NAME, "= %s[%d].LENGTH\r\n", name, structIndex);
		break;
	case 6:
		CLog::GetInstance().Print(LOG_NAME, "= %s[%d].POINTER\r\n", name, structIndex);
		break;
	}
}

void CSpeed::LogBdWrite(const char* name, uint32 base, uint32 address, uint32 value)
{
	uint32 regIndex = address & 0x7;
	uint32 structIndex = (address - base) / 8;
	switch(regIndex)
	{
	case 0:
		CLog::GetInstance().Print(LOG_NAME, "%s[%d].CTRLSTAT = 0x%08X\r\n", name, structIndex, value);
		break;
	case 2:
		CLog::GetInstance().Print(LOG_NAME, "%s[%d].RESERVED = 0x%08X\r\n", name, structIndex, value);
		break;
	case 4:
		CLog::GetInstance().Print(LOG_NAME, "%s[%d].LENGTH = 0x%08X\r\n", name, structIndex, value);
		break;
	case 6:
		CLog::GetInstance().Print(LOG_NAME, "%s[%d].POINTER = 0x%08X\r\n", name, structIndex, value);
		break;
	}
}
