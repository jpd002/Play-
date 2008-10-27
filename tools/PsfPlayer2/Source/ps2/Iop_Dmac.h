#ifndef _IOP_DMAC_H_
#define _IOP_DMAC_H_

#include "Types.h"
#include "Iop_DmacChannel.h"

namespace Iop
{
    class CDmac
    {
    public:
        enum
        {
            MAX_CHANNEL = 7,
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

        enum DMAC_ZONE1
        {
            DMAC_ZONE1_START    = 0x1F801080,
            DMAC_ZONE1_END      = 0x1F8010FF,
        };

                        CDmac(uint8*);
        virtual         ~CDmac();

        void            Reset();
        void			SetReceiveFunction(unsigned int, const Dmac::CChannel::ReceiveFunctionType&);
        uint32          ReadRegister(uint32);
        uint32          WriteRegister(uint32, uint32);

		void			AssertLine(unsigned int);
        uint8*          GetRam();

        enum
		{
			DPCR		= 0x1F8010F0,
			DICR		= 0x1F8010F4
		};

    private:
        Dmac::CChannel* GetChannelFromAddress(uint32);
        void            LogRead(uint32);
        void            LogWrite(uint32, uint32);

        Dmac::CChannel  m_channelSpu;
        Dmac::CChannel* m_channel[MAX_CHANNEL];

        uint32			m_DPCR;
		uint32			m_DICR;
        uint8*          m_ram;
    };
}

#endif
