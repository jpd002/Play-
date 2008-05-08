#ifndef _DMAC_H_
#define _DMAC_H_

#include "Types.h"

namespace Psx
{
	class CDmac
	{
	public:
					CDmac(uint8*);
		virtual		~CDmac();

		uint32		ReadRegister(uint32);
		void		WriteRegister(uint32, uint32);

		void		Reset();

		enum
		{
			ADDR_BEGIN	= 0x1F801080,
			ADDR_END	= 0x1F8010F7
		};

		enum
		{
			CH0_BASE	= 0x1F801080,
			CH1_BASE	= 0x1F801090,
			CH2_BASE	= 0x1F8010A0,
			CH3_BASE	= 0x1F8010B0,
			CH4_BASE	= 0x1F8010C0,
			CH5_BASE	= 0x1F8010D0,
			CH6_BASE	= 0x1F8010E0
		};

		enum
		{
			GENERAL_REGS_BASE = 0x1F8010F0
		};

		enum
		{
			CH_MADR		= 0x00,
			CH_BCR		= 0x04,
			CH_CHCR		= 0x08
		};

		enum
		{
			DPCR		= 0x1F8010F0,
			DICR		= 0x1F8010F4
		};

	private:
		void		DisassembleRead(uint32);
		void		DisassembleWrite(uint32, uint32);

		uint8*		m_ram;
		uint32		m_DPCR;
		uint32		m_DICR;
	};
}

#endif
