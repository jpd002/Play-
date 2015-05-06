#ifndef _DMAC_H_
#define _DMAC_H_

#include "Types.h"
#include "zip/ZipArchiveWriter.h"
#include "zip/ZipArchiveReader.h"
#include "Dmac_Channel.h"

class CMIPS;

class CDMAC
{
public:
	friend class Dmac::CChannel;

	enum REGISTER
	{
		D0_CHCR		= 0x10008000,
		D0_MADR		= 0x10008010,
		D0_QWC		= 0x10008020,
		D0_TADR		= 0x10008030,

		D1_CHCR		= 0x10009000,
		D1_MADR		= 0x10009010,
		D1_QWC		= 0x10009020,
		D1_TADR		= 0x10009030,

		D2_CHCR		= 0x1000A000,
		D2_MADR		= 0x1000A010,
		D2_QWC		= 0x1000A020,
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

		D8_CHCR		= 0x1000D000,
		D8_MADR		= 0x1000D010,
		D8_QWC		= 0x1000D020,
		D8_SADR		= 0x1000D080,

		D9_CHCR		= 0x1000D400,
		D9_MADR		= 0x1000D410,
		D9_QWC		= 0x1000D420,
		D9_TADR		= 0x1000D430,
		D9_SADR		= 0x1000D480,

		D_CTRL		= 0x1000E000,
		D_STAT		= 0x1000E010,
		D_PCR		= 0x1000E020,
		D_RBSR		= 0x1000E040,
		D_RBOR		= 0x1000E050,

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

						CDMAC(uint8*, uint8*, uint8*, CMIPS&);
	virtual				~CDMAC();

	void				Reset();

	void				SetChannelTransferFunction(unsigned int, const Dmac::DmaReceiveHandler&);

	uint32				GetRegister(uint32);
	void				SetRegister(uint32, uint32);

	void				LoadState(Framework::CZipArchiveReader&);
	void				SaveState(Framework::CZipArchiveWriter&);

	void				DisassembleGet(uint32);
	void				DisassembleSet(uint32, uint32);

	bool				IsInterruptPending();
	void				ResumeDMA0();
	void				ResumeDMA1();
	uint32				ResumeDMA3(const void*, uint32);
	void				ResumeDMA4();
	void				ResumeDMA8();
	bool				IsDMA4Started() const;
	static bool			IsEndTagId(uint32);

private:
	struct D_CTRL_REG : public convertible<uint32>
	{
		unsigned int	dmae		: 1;
		unsigned int	rele		: 1;
		unsigned int	mfd			: 2;
		unsigned int	sts			: 2;
		unsigned int	std			: 2;
		unsigned int	rcyc		: 3;
		unsigned int	reserved	: 21;
	};
	static_assert(sizeof(D_CTRL_REG) == sizeof(uint32), "Size of D_CTRL_REG struct must be 4 bytes.");

	uint64				FetchDMATag(uint32);

	uint32				ReceiveDMA8(uint32, uint32, uint32, bool);
	uint32				ReceiveDMA9(uint32, uint32, uint32, bool);

	void				UpdateCpCond();

	D_CTRL_REG			m_D_CTRL;
	uint32				m_D_STAT;
	uint32				m_D_ENABLE;
	uint32				m_D_PCR;
	uint32				m_D_RBSR;
	uint32				m_D_RBOR;

	Dmac::CChannel		m_D0;
	Dmac::CChannel		m_D1;

	Dmac::CChannel		m_D2;

	uint32				m_D3_CHCR;
	uint32				m_D3_MADR;
	uint32				m_D3_QWC;

	Dmac::CChannel		m_D4;

	uint32				m_D5_CHCR;
	uint32				m_D5_MADR;
	uint32				m_D5_QWC;

	uint32				m_D6_CHCR;
	uint32				m_D6_MADR;
	uint32				m_D6_QWC;
	uint32				m_D6_TADR;

	Dmac::CChannel		m_D8;
	uint32				m_D8_SADR;

	Dmac::CChannel		m_D9;
	uint32				m_D9_SADR;

	uint8*				m_ram;
	uint8*				m_spr;
	uint8*				m_vuMem0;

	CMIPS&				m_ee;

	Dmac::DmaReceiveHandler m_receiveDma5;
	Dmac::DmaReceiveHandler m_receiveDma6;
};

#endif
