#pragma once

#include "Types.h"
#include "zip/ZipArchiveWriter.h"
#include "zip/ZipArchiveReader.h"
#include "Iop_DmacChannel.h"

namespace Iop
{
	class CIntc;

	class CDmac
	{
	public:
		enum
		{
			CHANNEL_SPU0 = 4,
			CHANNEL_SPU1 = 8,
			CHANNEL_DEV9 = 9,
			CHANNEL_SIO2in = 11,
			CHANNEL_SIO2out = 12,
			MAX_CHANNEL = 14,
		};

		enum
		{
			CH0_BASE = 0x1F801080,
			CH1_BASE = 0x1F801090,
			CH2_BASE = 0x1F8010A0,
			CH3_BASE = 0x1F8010B0,
			CH4_BASE = 0x1F8010C0,
			CH5_BASE = 0x1F8010D0,
			CH6_BASE = 0x1F8010E0,
			CH8_BASE = 0x1F801500,
			CH9_BASE = 0x1F801510,
			CH11_BASE = 0x1F801530, //Unsure about that
			CH12_BASE = 0x1F801540, //Unsure about that
		};

		enum DMAC_ZONE1
		{
			DMAC_ZONE1_START = 0x1F801080,
			DMAC_ZONE1_END = 0x1F8010FF,
			DMAC_ZONE2_START = 0x1F801500,
			DMAC_ZONE2_END = 0x1F80155F,
			DMAC_ZONE3_START = 0x1F801570,
			DMAC_ZONE3_END = 0x1F801578,
		};

		CDmac(uint8*, CIntc&);
		virtual ~CDmac() = default;

		void Reset();
		void SetReceiveFunction(unsigned int, const Dmac::CChannel::ReceiveFunctionType&);
		uint32 ReadRegister(uint32);
		uint32 WriteRegister(uint32, uint32);

		void LoadState(Framework::CZipArchiveReader&);
		void SaveState(Framework::CZipArchiveWriter&);

		void ResumeDma(unsigned int);

		void AssertLine(unsigned int);
		uint8* GetRam();

		enum
		{
			DPCR = 0x1F8010F0,
			DPCR2 = 0x1F801570,
			DICR = 0x1F8010F4
		};

	private:
		unsigned int GetChannelIdFromAddress(uint32);
		Dmac::CChannel* GetChannelFromAddress(uint32);
		void LogRead(uint32);
		void LogWrite(uint32, uint32);

		Dmac::CChannel m_channelSpu0;
		Dmac::CChannel m_channelSpu1;
		Dmac::CChannel m_channelDev9;
		Dmac::CChannel m_channelSio2In;
		Dmac::CChannel m_channelSio2Out;
		Dmac::CChannel* m_channel[MAX_CHANNEL];

		uint32 m_DPCR;
		uint32 m_DPCR2;

		uint32 m_DICR;
		uint8* m_ram;
		CIntc& m_intc;
	};
}
