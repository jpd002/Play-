#ifndef _DMAC_CHANNEL_H_
#define _DMAC_CHANNEL_H_

#include "Types.h"
#include <functional>

class CDMAC;

namespace Dmac
{
    typedef std::tr1::function<uint32 (uint32, uint32, bool)> DmaReceiveHandler;

//	typedef uint32				(*DMARECEIVEMETHOD)(uint32, uint32, bool);
	typedef	void				(*DMASLICEDONECALLBACK)();

	class CChannel
	{
	public:
		struct CHCR
		{
			unsigned int		nDIR		: 1;
			unsigned int		nReserved0	: 1;
			unsigned int		nMOD		: 2;
			unsigned int		nASP		: 2;
			unsigned int		nTTE		: 1;
			unsigned int		nTIE		: 1;
			unsigned int		nSTR		: 1;
			unsigned int		nReserved1	: 7;
			unsigned int		nTAG		: 16;
		};


								CChannel(CDMAC&, unsigned int, const DmaReceiveHandler&, DMASLICEDONECALLBACK);
        virtual                 ~CChannel();
		void					Reset();
		uint32					ReadCHCR();
		void					WriteCHCR(uint32);
        void                    Execute();
		void					ExecuteNormal();
		void					ExecuteSourceChain();
        void                    SetReceiveHandler(const DmaReceiveHandler&);

		CHCR					m_CHCR;
		uint32					m_nMADR;
		uint32					m_nQWC;
		uint32					m_nTADR;
		uint32					m_nASR[2];

	private:
	    enum SCCTRL_BIT
	    {
		    SCCTRL_SUSPENDED	= 0x001,
		    SCCTRL_INITXFER		= 0x200,
	    };

        void					ClearSTR();

		unsigned int			m_nNumber;
		uint32					m_nSCCTRL;
		DmaReceiveHandler		m_pReceive;
		DMASLICEDONECALLBACK	m_pSliceDone;
        CDMAC&                  m_dmac;
    };
};

#endif
