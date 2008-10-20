#ifndef _PSX_DMAC_H_
#define _PSX_DMAC_H_

#include "Types.h"
#include "DmaChannel.h"

namespace Psx
{
	class CIntc;

	class CDmac
	{
	public:
						CDmac(uint8*, CIntc&);
		virtual			~CDmac();

		void			Reset();
		void			SetReceiveFunction(unsigned int, const CDmaChannel::ReceiveFunctionType&);

		uint8*			GetRam() const;
		void			AssertLine(unsigned int);

		uint32			ReadRegister(uint32);
		void			WriteRegister(uint32, uint32);

		enum
		{
			ADDR_BEGIN	= 0x1F801080,
			ADDR_END	= 0x1F8010F7
		};

		enum
		{
			MAX_CHANNEL = 7
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
			DPCR		= 0x1F8010F0,
			DICR		= 0x1F8010F4
		};

	private:
		void			DisassembleRead(uint32);
		void			DisassembleWrite(uint32, uint32);

		CDmaChannel*	m_channel[MAX_CHANNEL];
		CDmaChannel		m_channelSpu;

		uint8*			m_ram;
		CIntc&			m_intc;
		uint32			m_DPCR;
		uint32			m_DICR;
	};
}

#endif
