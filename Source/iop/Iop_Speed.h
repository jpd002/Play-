#pragma once

#include "Types.h"
#include "BasicUnion.h"
#include "Convertible.h"

namespace Iop
{
	class CSpeed
	{
	public:
		void Reset();
		
		uint32 ReadRegister(uint32);
		void WriteRegister(uint32, uint32);
		
	private:
		enum
		{
			INTR_ATA0 = 0,
			INTR_ATA1 = 1,
			INTR_SMAP_TXDNV = 2, //DNV = Descriptor not valid
			INTR_SMAP_RXDNV = 3,
			INTR_SMAP_TXEND = 4,
			INTR_SMAP_RXEND = 5,
			INTR_SMAP_EMAC3 = 6,
			INTR_DVR = 9,
			INTR_UART = 12
		};
		
		enum
		{
			REG_REV1 = 0x10000002,
			REG_REV3 = 0x10000004,
			REG_INTR_STAT = 0x10000028,
			REG_INTR_MASK = 0x1000002A,
			REG_PIO_DIR = 0x1000002C,
			REG_PIO_DATA = 0x1000002E,
			REG_SMAP_TXFIFO_FRAME_INC = 0x10001010,
			REG_SMAP_TXFIFO_DATA = 0x10001100,
			REG_SMAP_EMAC3_TXMODE0_HI = 0x10002008,
			REG_SMAP_EMAC3_TXMODE0_LO = 0x1000200A,
			REG_SMAP_EMAC3_STA_CTRL_HI = 0x1000205C,
			REG_SMAP_EMAC3_STA_CTRL_LO = 0x1000205E,
			SMAP_BD_SIZE = 0x00000200,
			REG_SMAP_BD_TX_BASE = 0x10003000,
			REG_SMAP_BD_RX_BASE = 0x10003200,
		};

		enum REV3_FLAGS
		{
			SPEED_CAPS_SMAP = 0x01,
			SPEED_CAPS_ATA = 0x02,
		};
		
		enum SMAP_EMAC3_STA_CMD
		{
			SMAP_EMAC3_STA_CMD_READ = 0x01,
			SMAP_EMAC3_STA_CMD_WRITE = 0x02,
		};
		
		struct SMAP_EMAC3_STA_CTRL : public convertible<uint32>
		{
			uint32 phyRegAddr : 5;
			uint32 phyAddr : 5;
			uint32 phyOpbClck : 2;
			uint32 phyStaCmd : 2;
			uint32 phyErrRead : 1;
			uint32 phyOpComp : 1;
			uint32 phyData : 16;
		};

		void ProcessEmac3StaCtrl();
		
		void LogRead(uint32);
		void LogWrite(uint32, uint32);
		void LogBdRead(const char*, uint32, uint32);
		void LogBdWrite(const char*, uint32, uint32, uint32);

		uint32 m_intrMask = 0;
		uint32 m_eepRomReadIndex = 0;
		static const uint32 m_eepRomDataSize = 4;
		static const uint16 m_eepromData[];
		UNION32_16 m_smapEmac3StaCtrl;
	};
}
