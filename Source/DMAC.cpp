#include <stdio.h>
#include "DMAC.h"
#include "Ps2Const.h"
#include "Profiler.h"
#include "Log.h"
#include "RegisterStateFile.h"
#include "placeholder_def.h"

#ifdef	PROFILE
#define	PROFILE_DMACZONE "DMAC"
#endif

#define LOG_NAME            ("dmac")
#define STATE_REGS_XML      ("dmac/regs.xml")
#define STATE_REGS_STAT     ("D_STAT")
#define STATE_REGS_D9_SADR  ("D9_SADR")

using namespace Framework;
using namespace std;
using namespace std::tr1;
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
    throw runtime_error("Not implemented.");
}

CDMAC::CDMAC(uint8* ram, uint8* spr) :
m_ram(ram),
m_spr(spr),
m_D_STAT(0),
m_D_ENABLE(0),
m_D0(*this, 0, DummyTransfertFunction),
m_D1(*this, 1, DummyTransfertFunction),
m_D2(*this, 2, DummyTransfertFunction),
m_D3_CHCR(0),
m_D3_MADR(0),
m_D3_QWC(0),
m_D4(*this, 4, DummyTransfertFunction),
m_D5_CHCR(0),
m_D5_MADR(0),
m_D5_QWC(0),
m_D6_CHCR(0),
m_D6_MADR(0),
m_D6_QWC(0),
m_D6_TADR(0),
m_D8(*this, 8, bind(&CDMAC::ReceiveDMA8, this, PLACEHOLDER_1, PLACEHOLDER_2, PLACEHOLDER_3, PLACEHOLDER_4)),
m_D8_SADR(0),
m_D9(*this, 9, bind(&CDMAC::ReceiveDMA9, this, PLACEHOLDER_1, PLACEHOLDER_2, PLACEHOLDER_3, PLACEHOLDER_4)),
m_D9_SADR(0)
{
    Reset();
}

CDMAC::~CDMAC()
{

}

void CDMAC::Reset()
{
	m_D_STAT	= 0;
	m_D_ENABLE	= 0;

    //Reset Channel 0
    m_D0.Reset();

	//Reset Channel 1
	m_D1.Reset();

	//Reset Channel 2
	m_D2.Reset();

	//Reset Channel 4
	m_D4.Reset();

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
        throw runtime_error("Unsupported channel.");
        break;
    }
}

bool CDMAC::IsInterruptPending()
{
	uint16 nMask, nStatus;
	nMask	= (uint16)((m_D_STAT & 0x63FF0000) >> 16);
	nStatus = (uint16)((m_D_STAT & 0x0000E3FF) >>  0);

	return ((nMask & nStatus) != 0);
}

void CDMAC::ResumeDMA0()
{
    m_D0.Execute();
}

void CDMAC::ResumeDMA1()
{
    m_D1.Execute();
}

uint32 CDMAC::ResumeDMA3(void* pBuffer, uint32 nSize)
{
	void* pDst;

    assert(m_D3_CHCR & CHCR_STR);
	if(!(m_D3_CHCR & CHCR_STR)) return 0;

	nSize = min(nSize, m_D3_QWC);

	if(m_D3_MADR & 0x80000000)
	{
		pDst = m_spr + (m_D3_MADR & (PS2::SPRSIZE - 1));
	}
	else
	{
		pDst = m_ram + (m_D3_MADR & (PS2::EERAMSIZE - 1));
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
	return ((nTag == 0x00) || (nTag == 0x07));
}

uint32 CDMAC::ReceiveDMA8(uint32 nDstAddress, uint32 nCount, uint32 unused, bool nTagIncluded)
{
	assert(nTagIncluded == false);

	uint32 nSrcAddress = m_D8_SADR;
	nSrcAddress &= (PS2::SPRSIZE - 1);
	nDstAddress &= (PS2::EERAMSIZE - 1);

	memcpy(m_ram + nDstAddress, m_spr + nSrcAddress, nCount * 0x10);

	m_D8_SADR += (nCount * 0x10);

	return nCount;
}

uint32 CDMAC::ReceiveDMA9(uint32 nSrcAddress, uint32 nCount, uint32 unused, bool nTagIncluded)
{
	assert(nTagIncluded == false);

	uint32 nDstAddress = m_D9_SADR;
	nDstAddress &= (PS2::SPRSIZE - 1);
	nSrcAddress &= (PS2::EERAMSIZE - 1);

	memcpy(m_spr + nDstAddress, m_ram + nSrcAddress, nCount * 0x10);

	m_D9_SADR += (nCount * 0x10);

	return nCount;
}

uint32 CDMAC::GetRegister(uint32 nAddress)
{
#ifdef _DEBUG
//	DisassembleGet(nAddress);
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

	case D1_CHCR + 0x0:
		return m_D1.ReadCHCR();
		break;
	case D1_CHCR + 0x4:
	case D1_CHCR + 0x8:
	case D1_CHCR + 0xC:
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
	case D8_CHCR + 0x0:
		return m_D8.ReadCHCR();
		break;
	case D8_CHCR + 0x4:
	case D8_CHCR + 0x8:
	case D8_CHCR + 0xC:
		return 0;
		break;

    case D8_MADR + 0x0:
        return m_D8.m_nMADR;
        break;
	case D8_MADR + 0x4:
	case D8_MADR + 0x8:
	case D8_MADR + 0xC:
		return 0;
		break;

    //Channel 9
    case D9_CHCR + 0x0:
		return m_D9.ReadCHCR();
		break;
	case D9_CHCR + 0x4:
	case D9_CHCR + 0x8:
	case D9_CHCR + 0xC:
		return 0;
		break;

    case D9_MADR + 0x0:
        return m_D9.m_nMADR;
        break;
	case D9_MADR + 0x4:
	case D9_MADR + 0x8:
	case D9_MADR + 0xC:
		return 0;
		break;

    //General Registers
    case 0x1000E010:
		return m_D_STAT;
		break;

	case D_ENABLER + 0x0:
		return m_D_ENABLE;
		break;
	case D_ENABLER + 0x4:
	case D_ENABLER + 0x8:
	case D_ENABLER + 0xC:
		break;

	default:
        CLog::GetInstance().Print(LOG_NAME, "Read to an unhandled IO port (0x%0.8X).\r\n", nAddress);
        break;
	}

	return 0;
}

void CDMAC::SetRegister(uint32 nAddress, uint32 nData)
{

#ifdef PROFILE
	CProfiler::GetInstance().BeginZone(PROFILE_DMACZONE);
#endif

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
		m_D0.m_nMADR = nData;
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
	case D1_CHCR + 0x4:
	case D1_CHCR + 0x8:
	case D1_CHCR + 0xC:
		break;

	case D1_MADR + 0x0:
		m_D1.m_nMADR = nData;
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
		m_D2.m_nMADR = nData;
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
		m_D3_MADR = nData;
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
		m_D4.m_nMADR = nData;
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
//			if(IsInterruptPending())
//			{
//				CINTC::CheckInterrupts();
//			}
		}
		break;
	case D5_CHCR + 0x4:
	case D5_CHCR + 0x8:
	case D5_CHCR + 0xC:
		break;

	//D5_MADR
	case D5_MADR + 0x0:
		m_D5_MADR = nData;
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
		m_D6_MADR = nData;
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
		m_D8.m_nMADR = nData;
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
		m_D8_SADR = nData;
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
		m_D9.m_nMADR = nData;
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
		m_D9_SADR = nData;
		break;
	case D9_SADR + 0x4:
	case D9_SADR + 0x8:
	case D9_SADR + 0xC:
		break;

	case D_STAT + 0x0:
		uint32 nStat, nMask;
		nStat = nData & 0x0000FFFF;
		nMask = nData & 0xFFFF0000;

		//Set the masks
		m_D_STAT ^= nMask;

		//Clear the interrupt status
		m_D_STAT &= ~nStat;
		break;
	case D_STAT + 0x4:
	case D_STAT + 0x8:
	case D_STAT + 0xC:
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

#ifdef PROFILE
	CProfiler::GetInstance().EndZone();
#endif

}

void CDMAC::LoadState(CZipArchiveReader& archive)
{
    CRegisterStateFile registerFile(*archive.BeginReadFile(STATE_REGS_XML));
    m_D_STAT    = registerFile.GetRegister32(STATE_REGS_STAT);
    m_D9_SADR   = registerFile.GetRegister32(STATE_REGS_D9_SADR);

    m_D1.LoadState(archive);
    m_D2.LoadState(archive);
    m_D4.LoadState(archive);
    m_D9.LoadState(archive);
}

void CDMAC::SaveState(CZipArchiveWriter& archive)
{
    CRegisterStateFile* registerFile = new CRegisterStateFile(STATE_REGS_XML);
    registerFile->SetRegister32(STATE_REGS_STAT,    m_D_STAT);
    registerFile->SetRegister32(STATE_REGS_D9_SADR, m_D9_SADR);
    archive.InsertFile(registerFile);

    m_D1.SaveState(archive);
    m_D2.SaveState(archive);
    m_D4.SaveState(archive);
    m_D9.SaveState(archive);
}

void CDMAC::DisassembleGet(uint32 nAddress)
{
	switch(nAddress)
	{
	case D2_CHCR:
		printf("DMAC: = D2_CHCR.\r\n");
		break;
	case D2_TADR:
		printf("DMAC: = D2_TADR.\r\n");
		break;
	case D3_QWC:
		printf("DMAC: = D3_QWC.\r\n");
		break;
	case D_STAT:
		printf("DMAC: = D_STAT.\r\n");
		break;
	case D_ENABLER:
		printf("DMAC: = D_ENABLER.\r\n");
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
	case D_STAT:
		CLog::GetInstance().Print(LOG_NAME, "D_STAT = 0x%0.8X.\r\n", nData);
        break;
	case D_ENABLEW:
		CLog::GetInstance().Print(LOG_NAME, "D_ENABLEW = 0x%0.8X.\r\n", nData);
        break;
	}
}
