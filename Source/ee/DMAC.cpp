#include <stdio.h>
#include "DMAC.h"
#include "../Ps2Const.h"
#include "../Log.h"
#include "../RegisterStateFile.h"
#include "../MIPS.h"
#include "../COP_SCU.h"
#include "placeholder_def.h"

#define LOG_NAME            ("dmac")
#define STATE_REGS_XML      ("dmac/regs.xml")
#define STATE_REGS_CTRL     ("D_CTRL")
#define STATE_REGS_STAT     ("D_STAT")
#define STATE_REGS_PCR		("D_PCR")
#define STATE_REGS_RBSR     ("D_RBSR")
#define STATE_REGS_RBOR     ("D_RBOR")
#define STATE_REGS_D8_SADR  ("D8_SADR")
#define STATE_REGS_D9_SADR  ("D9_SADR")

#define MADR_WRITE_MASK			(~0x0000000F)
#define SPR_MADR_WRITE_MASK		(~0x8000000F)

#define SADR_WRITE_MASK			((PS2::EE_SPR_SIZE - 1) & ~0x0F)

#define REGISTER_READ(addr, value)		\
	case (addr) + 0x0:					\
		return (value);					\
		break;							\
	case (addr) + 0x4:					\
	case (addr) + 0x8:					\
	case (addr) + 0xC:					\
		return 0;						\
		break;

using namespace Framework;
using namespace Dmac;

//DMA channels (EE side)
//0 - VIF0
//1 - VIF1
//2 - GIF
//3 - IPU (incoming)
//4 - IPU (outgoing)
//5 - SIF0 (from IOP)
//6 - SIF1 (to IOP?)
//7 - SIF2
//8 - SPR (incoming)
//9 - SPR (outgoing)

uint32 DummyTransfertFunction(uint32 address, uint32 size, uint32, bool)
{
//    return size;
	throw std::runtime_error("Not implemented.");
}

CDMAC::CDMAC(uint8* ram, uint8* spr, uint8* vuMem0, CMIPS& ee)
: m_ram(ram)
, m_spr(spr)
, m_vuMem0(vuMem0)
, m_ee(ee)
, m_D_STAT(0)
, m_D_ENABLE(0)
, m_D0(*this, 0, DummyTransfertFunction)
, m_D1(*this, 1, DummyTransfertFunction)
, m_D2(*this, 2, DummyTransfertFunction)
, m_D3_CHCR(0)
, m_D3_MADR(0)
, m_D3_QWC(0)
, m_D4(*this, 4, DummyTransfertFunction)
, m_D5_CHCR(0)
, m_D5_MADR(0)
, m_D5_QWC(0)
, m_D6_CHCR(0)
, m_D6_MADR(0)
, m_D6_QWC(0)
, m_D6_TADR(0)
, m_D8(*this, 8, std::bind(&CDMAC::ReceiveDMA8, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4))
, m_D8_SADR(0)
, m_D9(*this, 9, std::bind(&CDMAC::ReceiveDMA9, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4))
, m_D9_SADR(0)
{
	Reset();
}

CDMAC::~CDMAC()
{

}

void CDMAC::Reset()
{
	m_D_CTRL	<<= 0;
	m_D_STAT	= 0;
	m_D_ENABLE	= 0;
	m_D_PCR		= 0;
	m_D_RBSR	= 0;
	m_D_RBOR	= 0;

	//Reset Channel 0
	m_D0.Reset();

	//Reset Channel 1
	m_D1.Reset();

	//Reset Channel 2
	m_D2.Reset();

	//Reset Channel 3
	m_D3_CHCR = 0;
	m_D3_MADR = 0;
	m_D3_QWC = 0;

	//Reset Channel 4
	m_D4.Reset();

	//Reset Channel 5
	m_D5_CHCR = 0;
	m_D5_MADR = 0;
	m_D5_QWC = 0;

	//Reset Channel 6
	m_D6_CHCR = 0;
	m_D6_MADR = 0;
	m_D6_QWC = 0;
	m_D6_TADR = 0;

	//Reset Channel 8
	m_D8.Reset();
	m_D8_SADR = 0;

	//Reset Channel 9
	m_D9.Reset();
	m_D9_SADR = 0;
}

void CDMAC::SetChannelTransferFunction(unsigned int channel, const DmaReceiveHandler& handler)
{
	switch(channel)
	{
	case 0:
		m_D0.SetReceiveHandler(handler);
		break;
	case 1:
		m_D1.SetReceiveHandler(handler);
		break;
	case 2:
		m_D2.SetReceiveHandler(handler);
		break;
	case 4:
		m_D4.SetReceiveHandler(handler);
		break;
	case 5:
		m_receiveDma5 = handler;
		break;
	case 6:
		m_receiveDma6 = handler;
		break;
	default:
		throw std::runtime_error("Unsupported channel.");
		break;
	}
}

bool CDMAC::IsInterruptPending()
{
	uint16 mask		= static_cast<uint16>((m_D_STAT & 0x63FF0000) >> 16);
	uint16 status	= static_cast<uint16>((m_D_STAT & 0x0000E3FF) >>  0);

	return ((mask & status) != 0);
}

void CDMAC::ResumeDMA0()
{
	m_D0.Execute();
}

void CDMAC::ResumeDMA1()
{
	m_D1.Execute();
}

uint32 CDMAC::ResumeDMA3(const void* pBuffer, uint32 nSize)
{
	if(!(m_D3_CHCR & CHCR_STR)) return 0;

	nSize = std::min<uint32>(nSize, m_D3_QWC);

	void* pDst = nullptr;
	if(m_D3_MADR & 0x80000000)
	{
		pDst = m_spr + (m_D3_MADR & (PS2::EE_SPR_SIZE - 1));
	}
	else
	{
		pDst = m_ram + (m_D3_MADR & (PS2::EE_RAM_SIZE - 1));
	}

	memcpy(pDst, pBuffer, nSize * 0x10);

	m_D3_MADR	+= (nSize * 0x10);
	m_D3_QWC	-= nSize;

	if(m_D3_QWC == 0)
	{
		m_D3_CHCR &= ~CHCR_STR;
		m_D_STAT |= 0x08;
	}

	return nSize;
}

void CDMAC::ResumeDMA4()
{
	m_D4.Execute();
}

void CDMAC::ResumeDMA8()
{
	m_D8.Execute();
}

bool CDMAC::IsDMA4Started() const
{
	return (m_D4.m_CHCR.nSTR != 0) && (m_D_ENABLE == 0);
}

uint64 CDMAC::FetchDMATag(uint32 nAddress)
{
	if(nAddress & 0x80000000)
	{
		return *(uint64*)&m_spr[nAddress & 0x3FFF];
	}
	else
	{
		return *(uint64*)&m_ram[nAddress & 0x1FFFFFF];
	}
}

bool CDMAC::IsEndTagId(uint32 nTag)
{
	nTag = ((nTag >> 28) & 0x07);
	return ((nTag == CChannel::DMATAG_REFE) || (nTag == CChannel::DMATAG_END));
}

uint32 CDMAC::ReceiveDMA8(uint32 nDstAddress, uint32 nCount, uint32 unused, bool nTagIncluded)
{
	assert(nTagIncluded == false);

	assert(m_D8_SADR < PS2::EE_SPR_SIZE);
	assert((m_D8_SADR + (nCount * 0x10)) <= PS2::EE_SPR_SIZE);

	nDstAddress &= (PS2::EE_RAM_SIZE - 1);
	memcpy(m_ram + nDstAddress, m_spr + m_D8_SADR, nCount * 0x10);

	m_D8_SADR += (nCount * 0x10);
	m_D8_SADR &= SADR_WRITE_MASK;

	return nCount;
}

uint32 CDMAC::ReceiveDMA9(uint32 nSrcAddress, uint32 nCount, uint32 unused, bool nTagIncluded)
{
	assert(nTagIncluded == false);

	assert(m_D9_SADR < PS2::EE_SPR_SIZE);
	assert((m_D9_SADR + (nCount * 0x10)) <= PS2::EE_SPR_SIZE);

	if(nSrcAddress >= PS2::VUMEM0ADDR && nSrcAddress < (PS2::VUMEM0ADDR + PS2::VUMEM0SIZE))
	{
		nSrcAddress -= PS2::VUMEM0ADDR;
		nSrcAddress &= (PS2::VUMEM0SIZE - 1);
		memcpy(m_spr + m_D9_SADR, m_vuMem0 + nSrcAddress, nCount * 0x10);
	}
	else
	{
		nSrcAddress &= (PS2::EE_RAM_SIZE - 1);
		memcpy(m_spr + m_D9_SADR, m_ram + nSrcAddress, nCount * 0x10);
	}

	m_D9_SADR += (nCount * 0x10);
	m_D9_SADR &= SADR_WRITE_MASK;

	return nCount;
}

uint32 CDMAC::GetRegister(uint32 nAddress)
{
#ifdef _DEBUG
	DisassembleGet(nAddress);
#endif

	switch(nAddress)
	{
	case D0_CHCR + 0x0:
		return m_D0.ReadCHCR();
		break;
	case D0_CHCR + 0x4:
	case D0_CHCR + 0x8:
	case D0_CHCR + 0xC:
		return 0;
		break;

	case D0_TADR + 0x0:
		return m_D0.m_nTADR;
		break;
	case D0_TADR + 0x4:
	case D0_TADR + 0x8:
	case D0_TADR + 0xC:
		return 0;
		break;

	case D1_CHCR + 0x0:
		return m_D1.ReadCHCR();
		break;
	case D1_CHCR + 0x1:
		//This is done by FFXII
		return m_D1.ReadCHCR() >> 8;
		break;
	case D1_CHCR + 0x4:
	case D1_CHCR + 0x8:
	case D1_CHCR + 0xC:
		return 0;
		break;

	case D1_TADR + 0x0:
		return m_D1.m_nTADR;
		break;
	case D1_TADR + 0x4:
	case D1_TADR + 0x8:
	case D1_TADR + 0xC:
		return 0;
		break;

	case D2_CHCR + 0x0:
		return m_D2.ReadCHCR();
		break;

	case D2_CHCR + 0x4:
	case D2_CHCR + 0x8:
	case D2_CHCR + 0xC:
		return 0;
		break;

	case D2_MADR + 0x0:
		return m_D2.m_nMADR;
		break;
	case D2_MADR + 0x4:
	case D2_MADR + 0x8:
	case D2_MADR + 0xC:
		return 0;
		break;

	case D2_TADR + 0x0:
		return m_D2.m_nTADR;
		break;

	case D2_TADR + 0x4:
	case D2_TADR + 0x8:
	case D2_TADR + 0xC:
		return 0;
		break;

	case D3_CHCR + 0x0:
		return m_D3_CHCR;
		break;
	case D3_CHCR + 0x4:
	case D3_CHCR + 0x8:
	case D3_CHCR + 0xC:
		return 0;
		break;

	case D3_MADR + 0x0:
		return m_D3_MADR;
		break;
	case D3_MADR + 0x4:
	case D3_MADR + 0x8:
	case D3_MADR + 0xC:
		return 0;
		break;

	case D3_QWC + 0x0:
		return m_D3_QWC;
		break;
	case D3_QWC + 0x4:
	case D3_QWC + 0x8:
	case D3_QWC + 0xC:
		return 0;
		break;

	case D4_CHCR + 0x0:
		return m_D4.ReadCHCR();
		break;
	case D4_CHCR + 0x4:
	case D4_CHCR + 0x8:
	case D4_CHCR + 0xC:
		return 0;
		break;

	case D4_MADR + 0x0:
		return m_D4.m_nMADR;
		break;
	case D4_MADR + 0x4:
	case D4_MADR + 0x8:
	case D4_MADR + 0xC:
		return 0;
		break;

	case D4_QWC + 0x0:
		return m_D4.m_nQWC;
		break;
	case D4_QWC + 0x4:
	case D4_QWC + 0x8:
	case D4_QWC + 0xC:
		return 0;
		break;

	case D4_TADR + 0x0:
		return m_D4.m_nTADR;
		break;
	case D4_TADR + 0x4:
	case D4_TADR + 0x8:
	case D4_TADR + 0xC:
		return 0;
		break;

	//Channel 8
	REGISTER_READ(D8_CHCR, m_D8.ReadCHCR())
	REGISTER_READ(D8_MADR, m_D8.m_nMADR)
	REGISTER_READ(D8_QWC,  m_D8.m_nQWC)
	REGISTER_READ(D8_SADR, m_D8_SADR)

	//Channel 9
	REGISTER_READ(D9_CHCR, m_D9.ReadCHCR())
	REGISTER_READ(D9_MADR, m_D9.m_nMADR)
	REGISTER_READ(D9_QWC,  m_D9.m_nQWC)
	REGISTER_READ(D9_TADR, m_D9.m_nTADR)
	REGISTER_READ(D9_SADR, m_D9_SADR)

	//General Registers
	case D_CTRL:
		return m_D_CTRL;
		break;

	case D_STAT:
		return m_D_STAT;
		break;

	case D_PCR:
		return m_D_PCR;
		break;

	case D_ENABLER + 0x0:
		return m_D_ENABLE;
		break;
	case D_ENABLER + 0x4:
	case D_ENABLER + 0x8:
	case D_ENABLER + 0xC:
		break;

	default:
		CLog::GetInstance().Print(LOG_NAME, "Read an unhandled IO port (0x%0.8X).\r\n", nAddress);
		break;
	}

	return 0;
}

void CDMAC::SetRegister(uint32 nAddress, uint32 nData)
{
	switch(nAddress)
	{
	//Channel 0
	case D0_CHCR + 0x0:
		m_D0.WriteCHCR(nData);
		break;
	case D0_CHCR + 0x4:
	case D0_CHCR + 0x8:
	case D0_CHCR + 0xC:
		break;

	case D0_MADR + 0x0:
		m_D0.m_nMADR = nData & MADR_WRITE_MASK;
		break;
	case D0_MADR + 0x4:
	case D0_MADR + 0x8:
	case D0_MADR + 0xC:
		break;

	case D0_QWC + 0x0:
		m_D0.m_nQWC = nData;
		break;
	case D0_QWC + 0x4:
	case D0_QWC + 0x8:
	case D0_QWC + 0xC:
		break;

	case D0_TADR + 0x0:
		m_D0.m_nTADR = nData;
		break;
	case D0_TADR + 0x4:
	case D0_TADR + 0x8:
	case D0_TADR + 0xC:
		break;

	//Channel 1
	case D1_CHCR + 0x0:
		m_D1.WriteCHCR(nData);
		break;
	case D1_CHCR + 0x1:
		//This is done by FFXII
		m_D1.WriteCHCR((m_D1.ReadCHCR() & ~0xFF00) | ((nData & 0xFF) << 8));
		break;
	case D1_CHCR + 0x4:
	case D1_CHCR + 0x8:
	case D1_CHCR + 0xC:
		break;

	case D1_MADR + 0x0:
		assert(m_D1.m_CHCR.nSTR == 0);
		m_D1.m_nMADR = nData & MADR_WRITE_MASK;
		break;
	case D1_MADR + 0x4:
	case D1_MADR + 0x8:
	case D1_MADR + 0xC:
		break;

	case D1_QWC + 0x0:
		m_D1.m_nQWC = nData;
		break;
	case D1_QWC + 0x4:
	case D1_QWC + 0x8:
	case D1_QWC + 0xC:
		break;

	case D1_TADR + 0x0:
		m_D1.m_nTADR = nData;
		break;
	case D1_TADR + 0x4:
	case D1_TADR + 0x8:
	case D1_TADR + 0xC:
		break;

	//D2_CHCR
	case D2_CHCR + 0x0:
		m_D2.WriteCHCR(nData);
		break;
	case D2_CHCR + 0x4:
	case D2_CHCR + 0x8:
	case D2_CHCR + 0xC:
		break;

	//D2_MADR
	case D2_MADR + 0x0:
		m_D2.m_nMADR = nData & MADR_WRITE_MASK;
		break;
	case D2_MADR + 0x4:
	case D2_MADR + 0x8:
	case D2_MADR + 0xC:
		break;

	//D2_QWC
	case D2_QWC + 0x0:
		m_D2.m_nQWC = nData;
		break;
	case D2_QWC + 0x4:
	case D2_QWC + 0x8:
	case D2_QWC + 0xC:
		break;

	//D2_TADR
	case D2_TADR + 0x0:
		m_D2.m_nTADR = nData;
		break;
	case D2_TADR + 0x4:
	case D2_TADR + 0x8:
	case D2_TADR + 0xC:
		break;

	//D3_CHCR
	case D3_CHCR + 0x00:
		//We can't really start this DMA transfer at this moment since there might be no data in the IPU
		//The IPU will take responsibility to start the transfer
		m_D3_CHCR = nData;
		break;
	case D3_CHCR + 0x04:
	case D3_CHCR + 0x08:
	case D3_CHCR + 0x0C:
		break;

	//D3_MADR
	case D3_MADR + 0x00:
		m_D3_MADR = nData & MADR_WRITE_MASK;
		break;
	case D3_MADR + 0x04:
	case D3_MADR + 0x08:
	case D3_MADR + 0x0C:
		break;

	//D3_QWC
	case D3_QWC + 0x00:
		m_D3_QWC = nData;
		break;
	case D3_QWC + 0x04:
	case D3_QWC + 0x08:
	case D3_QWC + 0x0C:
		break;

	//D4_CHCR
	case D4_CHCR + 0x0:
		m_D4.WriteCHCR(nData);
		break;
	case D4_CHCR + 0x4:
	case D4_CHCR + 0x8:
	case D4_CHCR + 0xC:
		break;

	//D4_MADR
	case D4_MADR + 0x0:
		assert(m_D4.m_CHCR.nSTR == 0);
		m_D4.m_nMADR = nData & MADR_WRITE_MASK;
		break;
	case D4_MADR + 0x4:
	case D4_MADR + 0x8:
	case D4_MADR + 0xC:
		break;

	//D4_QWC
	case D4_QWC + 0x0:
		m_D4.m_nQWC = nData;
		break;
	case D4_QWC + 0x4:
	case D4_QWC + 0x8:
	case D4_QWC + 0xC:
		break;

	//D4_TADR
	case D4_TADR + 0x0:
		m_D4.m_nTADR = nData;
		break;
	case D4_TADR + 0x4:
	case D4_TADR + 0x8:
	case D4_TADR + 0xC:
		break;

	//D5_CHCR
	case D5_CHCR + 0x0:
		m_D5_CHCR = nData;
		if(m_D5_CHCR & 0x100)
		{
			m_receiveDma5(m_D5_MADR, m_D5_QWC * 0x10, 0, false);
			m_D5_CHCR	&= ~0x100;
			m_D_STAT	|= 0x20;
		}
		break;
	case D5_CHCR + 0x4:
	case D5_CHCR + 0x8:
	case D5_CHCR + 0xC:
		break;

	//D5_MADR
	case D5_MADR + 0x0:
		m_D5_MADR = nData & MADR_WRITE_MASK;
		break;
	case D5_MADR + 0x4:
	case D5_MADR + 0x8:
	case D5_MADR + 0xC:
		break;

	//D5_QWC
	case D5_QWC + 0x0:
		m_D5_QWC = nData;
		break;
	case D5_QWC + 0x4:
	case D5_QWC + 0x8:
	case D5_QWC + 0xC:
		break;

	//D6_CHCR
	case D6_CHCR + 0x0:
		m_D6_CHCR = nData;
		if(m_D6_CHCR & 0x100)
		{
			m_receiveDma6(m_D6_MADR, m_D6_QWC * 0x10, m_D6_TADR, false);
			m_D6_CHCR &= ~0x100;
		}
		break;
	case D6_CHCR + 0x4:
	case D6_CHCR + 0x8:
	case D6_CHCR + 0xC:
		break;

	//D6_MADR
	case D6_MADR + 0x0:
		m_D6_MADR = nData & MADR_WRITE_MASK;
		break;
	case D6_MADR + 0x4:
	case D6_MADR + 0x8:
	case D6_MADR + 0xC:
		break;

	//D6_QWC
	case D6_QWC + 0x0:
		m_D6_QWC = nData;
		break;
	case D6_QWC + 0x4:
	case D6_QWC + 0x8:
	case D6_QWC + 0xC:
		break;

	//D6_TADR
	case D6_TADR + 0x0:
		m_D6_TADR = nData;
		break;
	case D6_TADR + 0x4:
	case D6_TADR + 0x8:
	case D6_TADR + 0xC:
		break;

	//Channel 8
	case D8_CHCR + 0x0:
		m_D8.WriteCHCR(nData);
		break;
	case D8_CHCR + 0x4:
	case D8_CHCR + 0x8:
	case D8_CHCR + 0xC:
		break;

	case D8_MADR + 0x0:
		m_D8.m_nMADR = nData & SPR_MADR_WRITE_MASK;
		break;
	case D8_MADR + 0x4:
	case D8_MADR + 0x8:
	case D8_MADR + 0xC:
		break;

	case D8_QWC + 0x0:
		m_D8.m_nQWC = nData;
		break;
	case D8_QWC + 0x4:
	case D8_QWC + 0x8:
	case D8_QWC + 0xC:
		break;

	case D8_SADR + 0x0:
		m_D8_SADR = nData & SADR_WRITE_MASK;
		break;
	case D8_SADR + 0x4:
	case D8_SADR + 0x8:
	case D8_SADR + 0xC:
		break;

	//Channel 9
	case D9_CHCR + 0x0:
		m_D9.WriteCHCR(nData);
		break;
	case D9_CHCR + 0x4:
	case D9_CHCR + 0x8:
	case D9_CHCR + 0xC:
		break;

	case D9_MADR + 0x0:
		m_D9.m_nMADR = nData & SPR_MADR_WRITE_MASK;
		break;
	case D9_MADR + 0x4:
	case D9_MADR + 0x8:
	case D9_MADR + 0xC:
		break;

	case D9_QWC + 0x0:
		m_D9.m_nQWC = nData;
		break;
	case D9_QWC + 0x4:
	case D9_QWC + 0x8:
	case D9_QWC + 0xC:
		break;

	case D9_TADR + 0x0:
		m_D9.m_nTADR = nData;
		break;
	case D9_TADR + 0x4:
	case D9_TADR + 0x8:
	case D9_TADR + 0xC:
		break;

	case D9_SADR + 0x0:
		m_D9_SADR = nData & SADR_WRITE_MASK;
		break;
	case D9_SADR + 0x4:
	case D9_SADR + 0x8:
	case D9_SADR + 0xC:
		break;

	case D_CTRL + 0x0:
		m_D_CTRL <<= nData;
		break;
	case D_CTRL + 0x4:
	case D_CTRL + 0x8:
	case D_CTRL + 0xC:
		break;

	case D_STAT + 0x0:
		{
			uint32 nStat = nData & 0x0000FFFF;
			uint32 nMask = nData & 0xFFFF0000;

			//Set the masks
			m_D_STAT ^= nMask;

			//Clear the interrupt status
			m_D_STAT &= ~nStat;

			UpdateCpCond();
		}
		break;
	case D_STAT + 0x4:
	case D_STAT + 0x8:
	case D_STAT + 0xC:
		break;

	case D_PCR + 0x0:
		m_D_PCR = nData;
		break;
	case D_PCR + 0x4:
	case D_PCR + 0x8:
	case D_PCR + 0xC:
		break;

	case D_RBSR + 0x0:
		m_D_RBSR = nData;
		assert((m_D_RBSR & 0xF) == 0);
		break;
	case D_RBSR + 0x4:
	case D_RBSR + 0x8:
	case D_RBSR + 0xC:
		break;

	case D_RBOR + 0x0:
		m_D_RBOR = nData;
		assert((m_D_RBOR & m_D_RBSR) == 0);
		break;
	case D_RBOR + 0x4:
	case D_RBOR + 0x8:
	case D_RBOR + 0xC:
		break;

	case D_ENABLEW + 0x0:
		m_D_ENABLE = nData;
		break;
	case D_ENABLEW + 0x4:
	case D_ENABLEW + 0x8:
	case D_ENABLEW + 0xC:
		break;

	default:
		CLog::GetInstance().Print(LOG_NAME, "Wrote to an unhandled IO port (0x%0.8X, 0x%0.8X).\r\n", nAddress, nData);
		break;
	}

#ifdef _DEBUG
	DisassembleSet(nAddress, nData);
#endif

}

void CDMAC::LoadState(CZipArchiveReader& archive)
{
	CRegisterStateFile registerFile(*archive.BeginReadFile(STATE_REGS_XML));
	m_D_CTRL	<<= registerFile.GetRegister32(STATE_REGS_CTRL);
	m_D_STAT	= registerFile.GetRegister32(STATE_REGS_STAT);
	m_D_PCR		= registerFile.GetRegister32(STATE_REGS_PCR);
	m_D_RBSR	= registerFile.GetRegister32(STATE_REGS_RBSR);
	m_D_RBOR	= registerFile.GetRegister32(STATE_REGS_RBOR);
	m_D8_SADR	= registerFile.GetRegister32(STATE_REGS_D8_SADR);
	m_D9_SADR	= registerFile.GetRegister32(STATE_REGS_D9_SADR);

	m_D0.LoadState(archive);
	m_D1.LoadState(archive);
	m_D2.LoadState(archive);
	m_D4.LoadState(archive);
	m_D8.LoadState(archive);
	m_D9.LoadState(archive);
}

void CDMAC::SaveState(CZipArchiveWriter& archive)
{
	CRegisterStateFile* registerFile = new CRegisterStateFile(STATE_REGS_XML);
	registerFile->SetRegister32(STATE_REGS_CTRL,	m_D_CTRL);
	registerFile->SetRegister32(STATE_REGS_STAT,	m_D_STAT);
	registerFile->SetRegister32(STATE_REGS_PCR,		m_D_PCR);
	registerFile->SetRegister32(STATE_REGS_RBSR,	m_D_RBSR);
	registerFile->SetRegister32(STATE_REGS_RBOR,	m_D_RBOR);
	registerFile->SetRegister32(STATE_REGS_D8_SADR, m_D8_SADR);
	registerFile->SetRegister32(STATE_REGS_D9_SADR, m_D9_SADR);
	archive.InsertFile(registerFile);

	m_D0.SaveState(archive);
	m_D1.SaveState(archive);
	m_D2.SaveState(archive);
	m_D4.SaveState(archive);
	m_D8.SaveState(archive);
	m_D9.SaveState(archive);
}

void CDMAC::UpdateCpCond()
{
	bool condValue = true;
	for(unsigned int i = 0; i < 10; i++)
	{
		if(!(m_D_PCR & (1 << i))) continue;
		if(!(m_D_STAT & (1 << i)))
		{
			condValue = false;
		}
	}

	m_ee.m_State.nCOP0[CCOP_SCU::CPCOND0] = condValue;
}

void CDMAC::DisassembleGet(uint32 nAddress)
{
	switch(nAddress)
	{
	case D0_CHCR:
		CLog::GetInstance().Print(LOG_NAME, "= D0_CHCR.\r\n");
		break;
	case D0_TADR:
		CLog::GetInstance().Print(LOG_NAME, "= D0_TADR.\r\n");
		break;
	case D1_CHCR:
		CLog::GetInstance().Print(LOG_NAME, "= D1_CHCR.\r\n");
		break;
	case D1_TADR:
		CLog::GetInstance().Print(LOG_NAME, "= D1_TADR.\r\n");
		break;
	case D2_CHCR:
		CLog::GetInstance().Print(LOG_NAME, "= D2_CHCR.\r\n");
		break;
	case D2_TADR:
		CLog::GetInstance().Print(LOG_NAME, "= D2_TADR.\r\n");
		break;
	case D3_CHCR:
		CLog::GetInstance().Print(LOG_NAME, "= D3_CHCR.\r\n");
		break;
	case D3_MADR:
		CLog::GetInstance().Print(LOG_NAME, "= D3_MADR.\r\n");
		break;
	case D3_QWC:
		CLog::GetInstance().Print(LOG_NAME, "= D3_QWC.\r\n");
		break;
	case D4_CHCR:
		CLog::GetInstance().Print(LOG_NAME, "= D4_CHCR.\r\n");
		break;
	case D4_MADR:
		CLog::GetInstance().Print(LOG_NAME, "= D4_MADR.\r\n");
		break;
	case D4_QWC:
		CLog::GetInstance().Print(LOG_NAME, "= D4_QWC.\r\n");
		break;
	case D4_TADR:
		CLog::GetInstance().Print(LOG_NAME, "= D4_TADR.\r\n");
		break;
	case D8_CHCR:
		CLog::GetInstance().Print(LOG_NAME, "= D8_CHCR.\r\n");
		break;
	case D8_MADR:
		CLog::GetInstance().Print(LOG_NAME, "= D8_MADR.\r\n");
		break;
	case D8_QWC:
		CLog::GetInstance().Print(LOG_NAME, "= D8_QWC.\r\n");
		break;
	case D8_SADR:
		CLog::GetInstance().Print(LOG_NAME, "= D8_SADR.\r\n");
		break;
	case D9_CHCR:
		CLog::GetInstance().Print(LOG_NAME, "= D9_CHCR.\r\n");
		break;
	case D9_MADR:
		CLog::GetInstance().Print(LOG_NAME, "= D9_MADR.\r\n");
		break;
	case D9_TADR:
		CLog::GetInstance().Print(LOG_NAME, "= D9_TADR.\r\n");
		break;
	case D9_SADR:
		CLog::GetInstance().Print(LOG_NAME, "= D9_SADR.\r\n");
		break;
	case D_CTRL:
		CLog::GetInstance().Print(LOG_NAME, "= D_CTRL.\r\n");
		break;
	case D_STAT:
		CLog::GetInstance().Print(LOG_NAME, "= D_STAT.\r\n");
		break;
	case D_PCR:
		CLog::GetInstance().Print(LOG_NAME, "= D_PCR.\r\n");
		break;
	case D_ENABLER:
		CLog::GetInstance().Print(LOG_NAME, "= D_ENABLER.\r\n");
		break;
	default:
		CLog::GetInstance().Print(LOG_NAME, "Reading unknown register 0x%0.8X.\r\n", nAddress);
		break;
	}
}

void CDMAC::DisassembleSet(uint32 nAddress, uint32 nData)
{
	switch(nAddress)
	{
	case D1_CHCR:
		CLog::GetInstance().Print(LOG_NAME, "D1_CHCR = 0x%0.8X.\r\n", nData);
		break;
	case D1_MADR:
		CLog::GetInstance().Print(LOG_NAME, "D1_MADR = 0x%0.8X.\r\n", nData);
		break;
	case D1_QWC:
		CLog::GetInstance().Print(LOG_NAME, "D1_SIZE = 0x%0.8X.\r\n", nData);
		break;
	case D1_TADR:
		CLog::GetInstance().Print(LOG_NAME, "D1_TADR = 0x%0.8X.\r\n", nData);
		break;
	case D2_CHCR:
		CLog::GetInstance().Print(LOG_NAME, "D2_CHCR = 0x%0.8X.\r\n", nData);
		break;
	case D2_MADR:
		CLog::GetInstance().Print(LOG_NAME, "D2_MADR = 0x%0.8X.\r\n", nData);
		break;
	case D2_QWC:
		CLog::GetInstance().Print(LOG_NAME, "D2_SIZE = 0x%0.8X.\r\n", nData);
		break;
	case D2_TADR:
		CLog::GetInstance().Print(LOG_NAME, "D2_TADR = 0x%0.8X.\r\n", nData);
		break;
	case D3_CHCR:
		CLog::GetInstance().Print(LOG_NAME, "D3_CHCR = 0x%0.8X.\r\n", nData);
		break;
	case D3_MADR:
		CLog::GetInstance().Print(LOG_NAME, "D3_MADR = 0x%0.8X.\r\n", nData);
		break;
	case D3_QWC:
		CLog::GetInstance().Print(LOG_NAME, "D3_QWC = 0x%0.8X.\r\n", nData);
		break;
	case D4_CHCR:
		CLog::GetInstance().Print(LOG_NAME, "D4_CHCR = 0x%0.8X.\r\n", nData);
		break;
	case D4_MADR:
		CLog::GetInstance().Print(LOG_NAME, "D4_MADR = 0x%0.8X.\r\n", nData);
		break;
	case D4_QWC:
		CLog::GetInstance().Print(LOG_NAME, "D4_QWC = 0x%0.8X.\r\n", nData);
		break;
	case D4_TADR:
		CLog::GetInstance().Print(LOG_NAME, "D4_TADR = 0x%0.8X.\r\n", nData);
		break;
	case D5_CHCR:
		CLog::GetInstance().Print(LOG_NAME, "D5_CHCR = 0x%0.8X.\r\n", nData);
		break;
	case D5_MADR:
		CLog::GetInstance().Print(LOG_NAME, "D5_MADR = 0x%0.8X.\r\n", nData);
		break;
	case D5_QWC:
		CLog::GetInstance().Print(LOG_NAME, "D5_QWC = 0x%0.8X.\r\n", nData);
		break;
	case D6_CHCR:
		CLog::GetInstance().Print(LOG_NAME, "D6_CHCR = 0x%0.8X.\r\n", nData);
		break;
	case D6_MADR:
		CLog::GetInstance().Print(LOG_NAME, "D6_MADR = 0x%0.8X.\r\n", nData);
		break;
	case D6_QWC:
		CLog::GetInstance().Print(LOG_NAME, "D6_QWC = 0x%0.8X.\r\n", nData);
		break;
	case D6_TADR:
		CLog::GetInstance().Print(LOG_NAME, "D6_TADR = 0x%0.8X.\r\n", nData);
		break;
	case D8_CHCR:
		CLog::GetInstance().Print(LOG_NAME, "D8_CHCR = 0x%0.8X.\r\n", nData);
		break;
	case D8_MADR:
		CLog::GetInstance().Print(LOG_NAME, "D8_MADR = 0x%0.8X.\r\n", nData);
		break;
	case D8_QWC:
		CLog::GetInstance().Print(LOG_NAME, "D8_QWC = 0x%0.8X.\r\n", nData);
		break;
	case D8_SADR:
		CLog::GetInstance().Print(LOG_NAME, "D8_SADR = 0x%0.8X.\r\n", nData);
		break;
	case D9_CHCR:
		CLog::GetInstance().Print(LOG_NAME, "D9_CHCR = 0x%0.8X.\r\n", nData);
		break;
	case D9_MADR:
		CLog::GetInstance().Print(LOG_NAME, "D9_MADR = 0x%0.8X.\r\n", nData);
		break;
	case D9_QWC:
		CLog::GetInstance().Print(LOG_NAME, "D9_QWC = 0x%0.8X.\r\n", nData);
		break;
	case D9_TADR:
		CLog::GetInstance().Print(LOG_NAME, "D9_TADR = 0x%0.8X.\r\n", nData);
		break;
	case D9_SADR:
		CLog::GetInstance().Print(LOG_NAME, "D9_SADR = 0x%0.8X.\r\n", nData);
		break;
	case D_CTRL:
		CLog::GetInstance().Print(LOG_NAME, "D_CTRL = 0x%0.8X.\r\n", nData);
		break;
	case D_STAT:
		CLog::GetInstance().Print(LOG_NAME, "D_STAT = 0x%0.8X.\r\n", nData);
		break;
	case D_PCR:
		CLog::GetInstance().Print(LOG_NAME, "D_PCR = 0x%0.8X.\r\n", nData);
		break;
	case D_RBSR:
		CLog::GetInstance().Print(LOG_NAME, "D_RBSR = 0x%0.8X.\r\n", nData);
		break;
	case D_RBOR:
		CLog::GetInstance().Print(LOG_NAME, "D_RBOR = 0x%0.8X.\r\n", nData);
		break;
	case D_ENABLEW:
		CLog::GetInstance().Print(LOG_NAME, "D_ENABLEW = 0x%0.8X.\r\n", nData);
		break;
	default:
		CLog::GetInstance().Print(LOG_NAME, "Writing unknown register 0x%0.8X, 0x%0.8X.\r\n", nAddress, nData);
		break;
	}
}
