#include <cstring>
#include <stdio.h>
#include "DMAC.h"
#include "../Ps2Const.h"
#include "../Log.h"
#include "../states/RegisterStateFile.h"
#include "../MIPS.h"
#include "../COP_SCU.h"
#include "placeholder_def.h"

#define LOG_NAME ("ee_dmac")
#define STATE_REGS_XML ("dmac/regs.xml")
#define STATE_REGS_CTRL ("D_CTRL")
#define STATE_REGS_STAT ("D_STAT")
#define STATE_REGS_ENABLE ("D_ENABLE")
#define STATE_REGS_PCR ("D_PCR")
#define STATE_REGS_SQWC ("D_SQWC")
#define STATE_REGS_RBSR ("D_RBSR")
#define STATE_REGS_RBOR ("D_RBOR")
#define STATE_REGS_STADR ("D_STADR")
#define STATE_REGS_D3_CHCR ("D3_CHCR")
#define STATE_REGS_D3_MADR ("D3_MADR")
#define STATE_REGS_D3_QWC ("D3_QWC")
#define STATE_REGS_D5_CHCR ("D5_CHCR")
#define STATE_REGS_D5_MADR ("D5_MADR")
#define STATE_REGS_D5_QWC ("D5_QWC")
#define STATE_REGS_D6_CHCR ("D6_CHCR")
#define STATE_REGS_D6_MADR ("D6_MADR")
#define STATE_REGS_D6_QWC ("D6_QWC")
#define STATE_REGS_D6_TADR ("D6_TADR")
#define STATE_REGS_D8_SADR ("D8_SADR")
#define STATE_REGS_D9_SADR ("D9_SADR")

#define SADR_WRITE_MASK ((PS2::EE_SPR_SIZE - 1) & ~0x0F)

#define REGISTER_READ(addr, value) \
	case(addr) + 0x0:              \
		return (value);            \
		break;                     \
	case(addr) + 0x4:              \
	case(addr) + 0x8:              \
	case(addr) + 0xC:              \
		return 0;                  \
		break;

#define REGISTER_WRITE(addr, reg, value) \
	case(addr) + 0x0:                    \
		(reg) = (value);                 \
		break;                           \
	case(addr) + 0x4:                    \
	case(addr) + 0x8:                    \
	case(addr) + 0xC:                    \
		break;

using namespace Dmac;

static uint32 DummyTransferFunction(uint32 address, uint32 size, uint32, bool)
{
	throw std::runtime_error("Not implemented.");
}

CDMAC::CDMAC(uint8* ram, uint8* spr, uint8* vuMem0, CMIPS& ee)
    : m_ram(ram)
    , m_spr(spr)
    , m_vuMem0(vuMem0)
    , m_ee(ee)
    , m_D_STAT(0)
    , m_D_ENABLE(0)
    , m_D0(*this, 0, DummyTransferFunction)
    , m_D1(*this, 1, DummyTransferFunction)
    , m_D2(*this, 2, DummyTransferFunction)
    , m_D3_CHCR(0)
    , m_D3_MADR(0)
    , m_D3_QWC(0)
    , m_D4(*this, 4, DummyTransferFunction)
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

void CDMAC::Reset()
{
	m_D_CTRL <<= 0;
	m_D_STAT = 0;
	m_D_ENABLE = 0;
	m_D_PCR = 0;
	m_D_SQWC <<= 0;
	m_D_RBSR = 0;
	m_D_RBOR = 0;
	m_D_STADR = 0;

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

bool CDMAC::IsInterruptPending() const
{
	uint16 mask = static_cast<uint16>((m_D_STAT & 0x63FF0000) >> 16);
	uint16 status = static_cast<uint16>((m_D_STAT & 0x0000E3FF) >> 0);

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

void CDMAC::ResumeDMA2()
{
	m_D2.Execute();
}

uint32 CDMAC::ResumeDMA3(const void* pBuffer, uint32 nSize)
{
	if(!(m_D3_CHCR & CHCR_STR)) return 0;

	if((m_D_CTRL.sts == D_CTRL_STS_FROM_IPU) && (m_D_CTRL.std != D_CTRL_STD_NONE))
	{
		assert(m_D3_MADR >= m_D_STADR);
	}

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

	m_D3_MADR += (nSize * 0x10);
	m_D3_QWC -= nSize;

	if(m_D_CTRL.sts == D_CTRL_STS_FROM_IPU)
	{
		m_D_STADR = m_D3_MADR;
	}

	if(m_D3_QWC == 0)
	{
		m_D3_CHCR &= ~CHCR_STR;
		m_D_STAT |= (1 << CHANNEL_ID_FROM_IPU);
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
	return (m_D4.m_CHCR.nSTR != 0) && ((m_D_ENABLE & CDMAC::ENABLE_CPND) == 0);
}

uint64 CDMAC::FetchDMATag(uint32 address)
{
	if(address & 0x80000000)
	{
		return *reinterpret_cast<uint64*>(&m_spr[address & (PS2::EE_SPR_SIZE - 1)]);
	}
	else
	{
		return *reinterpret_cast<uint64*>(&m_ram[address & (PS2::EE_RAM_SIZE - 1)]);
	}
}

bool CDMAC::IsEndSrcTagId(uint32 tag)
{
	tag = ((tag >> 12) & 0x07);
	return ((tag == CChannel::DMATAG_SRC_REFE) || (tag == CChannel::DMATAG_SRC_END));
}

bool CDMAC::IsEndDstTagId(uint32 tag)
{
	tag = ((tag >> 12) & 0x07);
	return (tag == CChannel::DMATAG_DST_END);
}

uint32 CDMAC::ReceiveDMA8(uint32 nDstAddress, uint32 nCount, uint32 unused, bool nTagIncluded)
{
	assert(nTagIncluded == false);
	assert(m_D8_SADR < PS2::EE_SPR_SIZE);

	uint8* dstPtr = nullptr;
	if((nDstAddress >= PS2::VUMEM0ADDR) && (nDstAddress < (PS2::VUMEM0ADDR + PS2::VUMEM0SIZE)))
	{
		nDstAddress -= PS2::VUMEM0ADDR;
		nDstAddress &= (PS2::VUMEM0SIZE - 1);
		dstPtr = m_vuMem0;
	}
	else
	{
		nDstAddress &= (PS2::EE_RAM_SIZE - 1);
		dstPtr = m_ram;
	}

	uint32 remainTransfer = nCount;
	while(remainTransfer != 0)
	{
		uint32 remainSpr = (PS2::EE_SPR_SIZE - m_D8_SADR) / 0x10;
		uint32 copySize = std::min<uint32>(remainSpr, remainTransfer);
		memcpy(dstPtr + nDstAddress, m_spr + m_D8_SADR, copySize * 0x10);

		remainTransfer -= copySize;
		nDstAddress += (copySize * 0x10);
		m_D8_SADR += (copySize * 0x10);
		m_D8_SADR &= SADR_WRITE_MASK;
	}

	return nCount;
}

uint32 CDMAC::ReceiveDMA9(uint32 nSrcAddress, uint32 nCount, uint32 unused, bool nTagIncluded)
{
	assert(nTagIncluded == false);
	assert(m_D9_SADR < PS2::EE_SPR_SIZE);

	const uint8* srcPtr = nullptr;
	if((nSrcAddress >= PS2::VUMEM0ADDR) && (nSrcAddress < (PS2::VUMEM0ADDR + PS2::VUMEM0SIZE)))
	{
		nSrcAddress -= PS2::VUMEM0ADDR;
		nSrcAddress &= (PS2::VUMEM0SIZE - 1);
		srcPtr = m_vuMem0;
	}
	else
	{
		nSrcAddress &= (PS2::EE_RAM_SIZE - 1);
		srcPtr = m_ram;
	}
	assert(srcPtr);

	uint32 remainTransfer = nCount;
	while(remainTransfer != 0)
	{
		uint32 remainSpr = (PS2::EE_SPR_SIZE - m_D9_SADR) / 0x10;
		uint32 copySize = std::min<uint32>(remainSpr, remainTransfer);
		memcpy(m_spr + m_D9_SADR, srcPtr + nSrcAddress, copySize * 0x10);

		remainTransfer -= copySize;
		nSrcAddress += (copySize * 0x10);
		m_D9_SADR += (copySize * 0x10);
		m_D9_SADR &= SADR_WRITE_MASK;
	}

	return nCount;
}

uint32 CDMAC::GetRegister(uint32 nAddress)
{
#ifdef _DEBUG
	DisassembleGet(nAddress);
#endif

	switch(nAddress)
	{
		//Channel 0
		REGISTER_READ(D0_CHCR, m_D0.ReadCHCR())
		REGISTER_READ(D0_MADR, m_D0.m_nMADR)
		REGISTER_READ(D0_QWC, m_D0.m_nQWC)
		REGISTER_READ(D0_TADR, m_D0.m_nTADR)
		REGISTER_READ(D0_ASR0, m_D0.m_nASR[0])
		REGISTER_READ(D0_ASR1, m_D0.m_nASR[1])

		//Channel 1
		REGISTER_READ(D1_CHCR, m_D1.ReadCHCR())
		REGISTER_READ(D1_MADR, m_D1.m_nMADR)
		REGISTER_READ(D1_QWC, m_D1.m_nQWC)
		REGISTER_READ(D1_TADR, m_D1.m_nTADR)
		REGISTER_READ(D1_ASR0, m_D1.m_nASR[0])
		REGISTER_READ(D1_ASR1, m_D1.m_nASR[1])

	case D1_CHCR + 0x1:
		//This is done by FFXII
		return m_D1.ReadCHCR() >> 8;
		break;

	case D1_CHCR + 0x2:
		//This is done by Shin Megami Tensei
		return m_D1.ReadCHCR() >> 16;
		break;

		//Channel 2
		REGISTER_READ(D2_CHCR, m_D2.ReadCHCR())
		REGISTER_READ(D2_MADR, m_D2.m_nMADR)
		REGISTER_READ(D2_QWC, m_D2.m_nQWC)
		REGISTER_READ(D2_TADR, m_D2.m_nTADR)
		REGISTER_READ(D2_ASR0, m_D2.m_nASR[0])
		REGISTER_READ(D2_ASR1, m_D2.m_nASR[1])

	case D2_CHCR + 0x1:
		//This is done by Parappa The Rapper 2
		return m_D2.ReadCHCR() >> 8;
		break;

	case D2_CHCR + 0x2:
		//This is done by Hot Shots Golf Fore
		return m_D2.ReadCHCR() >> 16;
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

	case D5_CHCR + 0x0:
		return m_D5_CHCR;
		break;
	case D5_CHCR + 0x4:
	case D5_CHCR + 0x8:
	case D5_CHCR + 0xC:
		return 0;
		break;

		//Channel 8
		REGISTER_READ(D8_CHCR, m_D8.ReadCHCR())
		REGISTER_READ(D8_MADR, m_D8.m_nMADR)
		REGISTER_READ(D8_QWC, m_D8.m_nQWC)
		REGISTER_READ(D8_SADR, m_D8_SADR)

	case D8_CHCR + 0x1:
		//This is done by Front Mission 4
		return m_D8.ReadCHCR() >> 8;
		break;

		//Channel 9
		REGISTER_READ(D9_CHCR, m_D9.ReadCHCR())
		REGISTER_READ(D9_MADR, m_D9.m_nMADR)
		REGISTER_READ(D9_QWC, m_D9.m_nQWC)
		REGISTER_READ(D9_TADR, m_D9.m_nTADR)
		REGISTER_READ(D9_SADR, m_D9_SADR)

	case D9_CHCR + 0x1:
		//This is done by AirBlade
		return m_D9.ReadCHCR() >> 8;
		break;

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

	case D_SQWC:
		return m_D_SQWC;
		break;

	case D_RBSR:
		return m_D_RBSR;
		break;

	case D_RBOR:
		return m_D_RBOR;
		break;

	case D_ENABLER + 0x0:
		return m_D_ENABLE;
		break;
	case D_ENABLER + 0x4:
	case D_ENABLER + 0x8:
	case D_ENABLER + 0xC:
		break;

	default:
		CLog::GetInstance().Warn(LOG_NAME, "Read an unhandled IO port (0x%08X).\r\n", nAddress);
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

	case D0_ASR0 + 0x0:
		m_D0.m_nASR[0] = nData;
		break;
	case D0_ASR0 + 0x4:
	case D0_ASR0 + 0x8:
	case D0_ASR0 + 0xC:
		break;

	case D0_ASR1 + 0x0:
		m_D0.m_nASR[1] = nData;
		break;
	case D0_ASR1 + 0x4:
	case D0_ASR1 + 0x8:
	case D0_ASR1 + 0xC:
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

		REGISTER_WRITE(D1_ASR0, m_D1.m_nASR[0], nData & MADR_WRITE_MASK)
		REGISTER_WRITE(D1_ASR1, m_D1.m_nASR[1], nData & MADR_WRITE_MASK)

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
		m_D2.m_nQWC = nData & QWC_WRITE_MASK;
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

	case D2_ASR0 + 0x0:
		m_D2.m_nASR[0] = nData;
		break;
	case D2_ASR0 + 0x4:
	case D2_ASR0 + 0x8:
	case D2_ASR0 + 0xC:
		break;

	case D2_ASR1 + 0x0:
		m_D2.m_nASR[1] = nData;
		break;
	case D2_ASR1 + 0x4:
	case D2_ASR1 + 0x8:
	case D2_ASR1 + 0xC:
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
		if(m_D5_CHCR & CHCR_STR)
		{
			m_receiveDma5(m_D5_MADR, m_D5_QWC * 0x10, 0, false);
			m_D5_CHCR &= ~CHCR_STR;
			m_D_STAT |= (1 << CHANNEL_ID_SIF0);
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
	case D8_CHCR + 0x1:
		//This is done by Front Mission 4
		m_D8.WriteCHCR((m_D8.ReadCHCR() & ~0xFF00) | ((nData & 0xFF) << 8));
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
	case D9_CHCR + 0x1:
		//This is done by AirBlade
		m_D9.WriteCHCR((m_D9.ReadCHCR() & ~0xFF00) | ((nData & 0xFF) << 8));
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
		UpdateCpCond();
		break;
	case D_PCR + 0x4:
	case D_PCR + 0x8:
	case D_PCR + 0xC:
		break;

	case D_SQWC + 0x0:
		m_D_SQWC <<= nData;
		break;
	case D_SQWC + 0x4:
	case D_SQWC + 0x8:
	case D_SQWC + 0xC:
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

	case D_STADR + 0x0:
		m_D_STADR = nData;
		break;
	case D_STADR + 0x4:
	case D_STADR + 0x8:
	case D_STADR + 0xC:
		break;

	case D_ENABLEW + 0x0:
		m_D_ENABLE = nData;
		break;
	case D_ENABLEW + 0x4:
	case D_ENABLEW + 0x8:
	case D_ENABLEW + 0xC:
		break;

	default:
		CLog::GetInstance().Warn(LOG_NAME, "Wrote to an unhandled IO port (0x%08X, 0x%08X).\r\n", nAddress, nData);
		break;
	}

#ifdef _DEBUG
	DisassembleSet(nAddress, nData);
#endif
}

void CDMAC::LoadState(Framework::CZipArchiveReader& archive)
{
	CRegisterStateFile registerFile(*archive.BeginReadFile(STATE_REGS_XML));
	m_D_CTRL <<= registerFile.GetRegister32(STATE_REGS_CTRL);
	m_D_STAT = registerFile.GetRegister32(STATE_REGS_STAT);
	m_D_ENABLE = registerFile.GetRegister32(STATE_REGS_ENABLE);
	m_D_PCR = registerFile.GetRegister32(STATE_REGS_PCR);
	m_D_SQWC <<= registerFile.GetRegister32(STATE_REGS_SQWC);
	m_D_RBSR = registerFile.GetRegister32(STATE_REGS_RBSR);
	m_D_RBOR = registerFile.GetRegister32(STATE_REGS_RBOR);
	m_D_STADR = registerFile.GetRegister32(STATE_REGS_STADR);
	m_D3_CHCR = registerFile.GetRegister32(STATE_REGS_D3_CHCR);
	m_D3_MADR = registerFile.GetRegister32(STATE_REGS_D3_MADR);
	m_D3_QWC = registerFile.GetRegister32(STATE_REGS_D3_QWC);
	m_D5_CHCR = registerFile.GetRegister32(STATE_REGS_D5_CHCR);
	m_D5_MADR = registerFile.GetRegister32(STATE_REGS_D5_MADR);
	m_D5_QWC = registerFile.GetRegister32(STATE_REGS_D5_QWC);
	m_D6_CHCR = registerFile.GetRegister32(STATE_REGS_D6_CHCR);
	m_D6_MADR = registerFile.GetRegister32(STATE_REGS_D6_MADR);
	m_D6_QWC = registerFile.GetRegister32(STATE_REGS_D6_QWC);
	m_D6_TADR = registerFile.GetRegister32(STATE_REGS_D6_TADR);
	m_D8_SADR = registerFile.GetRegister32(STATE_REGS_D8_SADR);
	m_D9_SADR = registerFile.GetRegister32(STATE_REGS_D9_SADR);

	m_D0.LoadState(archive);
	m_D1.LoadState(archive);
	m_D2.LoadState(archive);
	m_D4.LoadState(archive);
	m_D8.LoadState(archive);
	m_D9.LoadState(archive);
}

void CDMAC::SaveState(Framework::CZipArchiveWriter& archive)
{
	CRegisterStateFile* registerFile = new CRegisterStateFile(STATE_REGS_XML);
	registerFile->SetRegister32(STATE_REGS_CTRL, m_D_CTRL);
	registerFile->SetRegister32(STATE_REGS_STAT, m_D_STAT);
	registerFile->SetRegister32(STATE_REGS_ENABLE, m_D_ENABLE);
	registerFile->SetRegister32(STATE_REGS_PCR, m_D_PCR);
	registerFile->SetRegister32(STATE_REGS_SQWC, m_D_SQWC);
	registerFile->SetRegister32(STATE_REGS_RBSR, m_D_RBSR);
	registerFile->SetRegister32(STATE_REGS_RBOR, m_D_RBOR);
	registerFile->SetRegister32(STATE_REGS_STADR, m_D_STADR);
	registerFile->SetRegister32(STATE_REGS_D3_CHCR, m_D3_CHCR);
	registerFile->SetRegister32(STATE_REGS_D3_MADR, m_D3_MADR);
	registerFile->SetRegister32(STATE_REGS_D3_QWC, m_D3_QWC);
	registerFile->SetRegister32(STATE_REGS_D5_CHCR, m_D5_CHCR);
	registerFile->SetRegister32(STATE_REGS_D5_MADR, m_D5_MADR);
	registerFile->SetRegister32(STATE_REGS_D5_QWC, m_D5_QWC);
	registerFile->SetRegister32(STATE_REGS_D6_CHCR, m_D6_CHCR);
	registerFile->SetRegister32(STATE_REGS_D6_MADR, m_D6_MADR);
	registerFile->SetRegister32(STATE_REGS_D6_QWC, m_D6_QWC);
	registerFile->SetRegister32(STATE_REGS_D6_TADR, m_D6_TADR);
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
#define LOG_GET(registerId)                                            \
	case registerId:                                                   \
		CLog::GetInstance().Print(LOG_NAME, "= " #registerId ".\r\n"); \
		break;

	switch(nAddress)
	{
		//Channel 0
		LOG_GET(D0_CHCR)
		LOG_GET(D0_MADR)
		LOG_GET(D0_QWC)
		LOG_GET(D0_TADR)
		LOG_GET(D0_ASR0)
		LOG_GET(D0_ASR1)

		//Channel 1
		LOG_GET(D1_CHCR)
		LOG_GET(D1_MADR)
		LOG_GET(D1_QWC)
		LOG_GET(D1_TADR)
		LOG_GET(D1_ASR0)
		LOG_GET(D1_ASR1)

		//Channel 2
		LOG_GET(D2_CHCR)
		LOG_GET(D2_MADR)
		LOG_GET(D2_QWC)
		LOG_GET(D2_TADR)
		LOG_GET(D2_ASR0)
		LOG_GET(D2_ASR1)

		//Channel 3
		LOG_GET(D3_CHCR)
		LOG_GET(D3_MADR)
		LOG_GET(D3_QWC)

		//Channel 4
		LOG_GET(D4_CHCR)
		LOG_GET(D4_MADR)
		LOG_GET(D4_QWC)
		LOG_GET(D4_TADR)

		//Channel 5
		LOG_GET(D5_CHCR)

		//Channel 8
		LOG_GET(D8_CHCR)
		LOG_GET(D8_MADR)
		LOG_GET(D8_QWC)
		LOG_GET(D8_SADR)

		//Channel 9
		LOG_GET(D9_CHCR)
		LOG_GET(D9_MADR)
		LOG_GET(D9_TADR)
		LOG_GET(D9_SADR)

		//General
		LOG_GET(D_CTRL)
		LOG_GET(D_STAT)
		LOG_GET(D_PCR)
		LOG_GET(D_SQWC)
		LOG_GET(D_RBSR)
		LOG_GET(D_RBOR)
		LOG_GET(D_ENABLER)

	default:
		CLog::GetInstance().Warn(LOG_NAME, "Reading unknown register 0x%08X.\r\n", nAddress);
		break;
	}

#undef LOG_GET
}

void CDMAC::DisassembleSet(uint32 nAddress, uint32 nData)
{
#define LOG_SET(registerId)                                                       \
	case registerId:                                                              \
		CLog::GetInstance().Print(LOG_NAME, #registerId " = 0x%08X.\r\n", nData); \
		break;

	switch(nAddress)
	{
		//Channel 0
		LOG_SET(D0_CHCR)
		LOG_SET(D0_MADR)
		LOG_SET(D0_QWC)
		LOG_SET(D0_TADR)
		LOG_SET(D0_ASR0)
		LOG_SET(D0_ASR1)

		//Channel 1
		LOG_SET(D1_CHCR)
		LOG_SET(D1_MADR)
		LOG_SET(D1_QWC)
		LOG_SET(D1_TADR)
		LOG_SET(D1_ASR0)
		LOG_SET(D1_ASR1)

		//Channel 2
		LOG_SET(D2_CHCR)
		LOG_SET(D2_MADR)
		LOG_SET(D2_QWC)
		LOG_SET(D2_TADR)
		LOG_SET(D2_ASR0)
		LOG_SET(D2_ASR1)

		//Channel 3
		LOG_SET(D3_CHCR)
		LOG_SET(D3_MADR)
		LOG_SET(D3_QWC)

		//Channel 4
		LOG_SET(D4_CHCR)
		LOG_SET(D4_MADR)
		LOG_SET(D4_QWC)
		LOG_SET(D4_TADR)

		//Channel 5
		LOG_SET(D5_CHCR)
		LOG_SET(D5_MADR)
		LOG_SET(D5_QWC)

		//Channel 6
		LOG_SET(D6_CHCR)
		LOG_SET(D6_MADR)
		LOG_SET(D6_QWC)
		LOG_SET(D6_TADR)

		//Channel 8
		LOG_SET(D8_CHCR)
		LOG_SET(D8_MADR)
		LOG_SET(D8_QWC)
		LOG_SET(D8_SADR)

		//Channel 9
		LOG_SET(D9_CHCR)
		LOG_SET(D9_MADR)
		LOG_SET(D9_QWC)
		LOG_SET(D9_TADR)
		LOG_SET(D9_SADR)

		//General
		LOG_SET(D_CTRL)
		LOG_SET(D_STAT)
		LOG_SET(D_PCR)
		LOG_SET(D_SQWC)
		LOG_SET(D_RBSR)
		LOG_SET(D_RBOR)
		LOG_SET(D_STADR)
		LOG_SET(D_ENABLEW)

	default:
		CLog::GetInstance().Warn(LOG_NAME, "Writing unknown register 0x%08X, 0x%08X.\r\n", nAddress, nData);
		break;
	}

#undef LOG_SET
}
