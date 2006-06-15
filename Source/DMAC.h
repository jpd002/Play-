#ifndef _DMAC_H_
#define _DMAC_H_

#include "Types.h"
#include "Stream.h"

class CDMAC
{
public:
	enum REGISTER
	{
		D1_CHCR		= 0x10009000,
		D1_MADR		= 0x10009010,
		D1_QWC		= 0x10009020,
		D1_TADR		= 0x10009030,

		D2_CHCR		= 0x1000A000,
		D2_MADR		= 0x1000A010,
		D2_SIZE		= 0x1000A020,
		D2_TADR		= 0x1000A030,
		
		D3_CHCR		= 0x1000B000,
		D3_MADR		= 0x1000B010,
		D3_QWC		= 0x1000B020,

		D4_CHCR		= 0x1000B400,
		D4_MADR		= 0x1000B410,
		D4_QWC		= 0x1000B420,
		D4_TADR		= 0x1000B430,

		D5_CHCR		= 0x1000C000,
		D5_MADR		= 0x1000C010,
		D5_QWC		= 0x1000C020,

		D6_CHCR		= 0x1000C400,
		D6_MADR		= 0x1000C410,
		D6_QWC		= 0x1000C420,
		D6_TADR		= 0x1000C430,

		D9_CHCR		= 0x1000D400,
		D9_MADR		= 0x1000D410,
		D9_QWC		= 0x1000D420,
		D9_TADR		= 0x1000D430,
		D9_SADR		= 0x1000D480,

		D_STAT		= 0x1000E010,

		D_ENABLER	= 0x1000F520,
		D_ENABLEW	= 0x1000F590,
	};

	enum CHCR_BIT
	{
		CHCR_STR		= 0x100,
	};

	enum ENABLE_BIT
	{
		ENABLE_CPND		= 0x10000,
	};

	static void					Reset();

	static uint32				GetRegister(uint32);
	static void					SetRegister(uint32, uint32);

	static void					LoadState(Framework::CStream*);
	static void					SaveState(Framework::CStream*);

	static void					DisassembleGet(uint32);
	static void					DisassembleSet(uint32, uint32);

	static bool					IsInterruptPending();
	static uint32				ResumeDMA3(void*, uint32);
	static void					ResumeDMA4();
	static bool					IsEndTagId(uint32);

private:
	enum SCCTRL_BIT
	{
		SCCTRL_SUSPENDED	= 0x001,
		SCCTRL_INITXFER		= 0x200,
	};

	typedef uint32				(*DMARECEIVEMETHOD)(uint32, uint32, bool);
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


								CChannel(unsigned int, DMARECEIVEMETHOD, DMASLICEDONECALLBACK);
		void					Reset();
		uint32					ReadCHCR();
		void					WriteCHCR(uint32);
		void					ExecuteNormal();
		void					ExecuteSourceChain();
		void					LoadState(Framework::CStream*);
		void					SaveState(Framework::CStream*);

		CHCR					m_CHCR;
		uint32					m_nMADR;
		uint32					m_nQWC;
		uint32					m_nTADR;
		uint32					m_nASR[2];

	private:
		void					ClearSTR();

		unsigned int			m_nNumber;
		uint32					m_nSCCTRL;
		DMARECEIVEMETHOD		m_pReceive;
		DMASLICEDONECALLBACK	m_pSliceDone;
	};

	static uint64				FetchDMATag(uint32);

	static uint32				ReceiveSPRDMA(uint32, uint32, bool);

	static uint32				m_D_STAT;
	static uint32				m_D_ENABLE;

	static CChannel				m_D1;

	static CChannel				m_D2;

	static uint32				m_D3_CHCR;
	static uint32				m_D3_MADR;
	static uint32				m_D3_QWC;

	static CChannel				m_D4;

	static uint32				m_D5_CHCR;
	static uint32				m_D5_MADR;
	static uint32				m_D5_QWC;

	static uint32				m_D6_CHCR;
	static uint32				m_D6_MADR;
	static uint32				m_D6_QWC;
	static uint32				m_D6_TADR;

	static CChannel				m_D9;
	static uint32				m_D9_SADR;
};

#endif
