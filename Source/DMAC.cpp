#include <stdio.h>
#include "PS2VM.h"
#include "DMAC.h"
#include "INTC.h"
#include "GIF.h"
#include "SIF.h"
#include "VIF.h"
#include "IPU.h"
#include "Profiler.h"

#ifdef	PROFILE
#define	PROFILE_DMACZONE "DMAC"
#endif

using namespace Framework;

uint32				CDMAC::m_D_STAT		= 0;
uint32				CDMAC::m_D_ENABLE	= 0;

CDMAC::CChannel		CDMAC::m_D1(1, CVIF::ReceiveDMA1, NULL);

CDMAC::CChannel		CDMAC::m_D2(2, CGIF::ReceiveDMA, NULL);

uint32				CDMAC::m_D3_CHCR	= 0;
uint32				CDMAC::m_D3_MADR	= 0;
uint32				CDMAC::m_D3_QWC		= 0;

CDMAC::CChannel		CDMAC::m_D4(4, CIPU::ReceiveDMA, CIPU::DMASliceDoneCallback);

uint32				CDMAC::m_D5_CHCR	= 0;
uint32				CDMAC::m_D5_MADR	= 0;
uint32				CDMAC::m_D5_QWC		= 0;

uint32				CDMAC::m_D6_CHCR	= 0;
uint32				CDMAC::m_D6_MADR	= 0;
uint32				CDMAC::m_D6_QWC		= 0;
uint32				CDMAC::m_D6_TADR	= 0;

CDMAC::CChannel		CDMAC::m_D9(9, ReceiveSPRDMA, NULL);
uint32				CDMAC::m_D9_SADR	= 0;

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

void CDMAC::Reset()
{
	m_D_STAT	= 0;
	m_D_ENABLE	= 0;

	//Reset Channel 1
	m_D1.Reset();

	//Reset Channel 2
	m_D2.Reset();

	//Reset Channel 4
	m_D4.Reset();

	//Reset Channel 9
	m_D9.Reset();
	m_D9_SADR = 0;
}

bool CDMAC::IsInterruptPending()
{
	uint16 nMask, nStatus;
	nMask	= (uint16)((m_D_STAT & 0x63FF0000) >> 16);
	nStatus = (uint16)((m_D_STAT & 0x0000E3FF) >>  0);

	return ((nMask & nStatus) != 0);
}

uint32 CDMAC::ResumeDMA3(void* pBuffer, uint32 nSize)
{
	void* pDst;

	if(!(m_D3_CHCR & CHCR_STR)) return 0;

	nSize = min(nSize, m_D3_QWC);

	if(m_D3_MADR & 0x80000000)
	{
		pDst = CPS2VM::m_pSPR + (m_D3_MADR & (CPS2VM::SPRSIZE - 1));
	}
	else
	{
		pDst = CPS2VM::m_pRAM + (m_D3_MADR & (CPS2VM::RAMSIZE - 1));
	}

	memcpy(pDst, pBuffer, nSize * 0x10);

	m_D3_MADR	+= (nSize * 0x10);
	m_D3_QWC	-= nSize;

	if(m_D3_QWC == 0)
	{
		m_D3_CHCR &= ~CHCR_STR;
		m_D_STAT |= 0x08;
		if(IsInterruptPending())
		{
			CINTC::CheckInterrupts();
		}
	}

	return nSize;
}

void CDMAC::ResumeDMA4()
{
//	if(!(m_D4.m_CHCR & CHCR_STR)) return;
	if(m_D4.m_CHCR.nSTR == 0) return;

	switch(m_D4.m_CHCR.nMOD)
	{
	case 0x00:
		m_D4.ExecuteNormal();
		break;
	case 0x01:
		m_D4.ExecuteSourceChain();
		break;
	default:
		assert(0);
		break;
	}
}

uint64 CDMAC::FetchDMATag(uint32 nAddress)
{
	if(nAddress & 0x80000000)
	{
		return *(uint64*)&CPS2VM::m_pSPR[nAddress & 0x3FFF];
	}
	else
	{
		return *(uint64*)&CPS2VM::m_pRAM[nAddress & 0x1FFFFFF];
	}
}

bool CDMAC::IsEndTagId(uint32 nTag)
{
	nTag = ((nTag >> 28) & 0x07);
	return ((nTag == 0x00) || (nTag == 0x07));
}

uint32 CDMAC::ReceiveSPRDMA(uint32 nSrcAddress, uint32 nCount, bool nTagIncluded)
{
	uint32 nDstAddress;

	assert(nTagIncluded == false);

	nDstAddress = m_D9_SADR;
	nDstAddress &= (CPS2VM::SPRSIZE - 1);
	nSrcAddress &= (CPS2VM::RAMSIZE - 1);

	memcpy(CPS2VM::m_pSPR + nDstAddress, CPS2VM::m_pRAM + nSrcAddress, nCount * 0x10);

	m_D9_SADR += (nCount * 0x10);

	return nCount;
}

uint32 CDMAC::GetRegister(uint32 nAddress)
{
	DisassembleGet(nAddress);

	switch(nAddress)
	{
	case D1_CHCR + 0x0:
		return m_D1.ReadCHCR();
		break;
	case D1_CHCR + 0x4:
	case D1_CHCR + 0x8:
	case D1_CHCR + 0xC:
		return 0;
		break;

	case D2_CHCR:
		return m_D2.ReadCHCR();
		break;

	case 0x1000A004:
	case 0x1000A008:
	case 0x1000A00C:
		return 0;
		break;

	case 0x1000A030:
		return m_D2.m_nTADR;
		break;

	case 0x1000A034:
	case 0x1000A038:
	case 0x1000A03C:
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

	case D9_CHCR + 0x0:
		return m_D9.ReadCHCR();
		break;
	case D9_CHCR + 0x4:
	case D9_CHCR + 0x8:
	case D9_CHCR + 0xC:
		return 0;
		break;

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
		printf("DMAC: Read to an unhandled IO port (0x%0.8X, PC: 0x%0.8X).\r\n", nAddress, CPS2VM::m_EE.m_State.nPC);
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
	case 0x1000A000:
		m_D2.WriteCHCR(nData);
		break;
	case 0x1000A004:
	case 0x1000A008:
	case 0x1000A00C:
		break;

	//D2_MADR
	case 0x1000A010:
		m_D2.m_nMADR = nData;
		break;
	case 0x1000A014:
	case 0x1000A018:
	case 0x1000A01C:
		break;

	//D2_QWC
	case 0x1000A020:
		m_D2.m_nQWC = nData;
		break;
	case 0x1000A024:
	case 0x1000A028:
	case 0x1000A02C:
		break;

	//D2_TADR
	case 0x1000A030:
		m_D2.m_nTADR = nData;
		break;
	case 0x1000A034:
	case 0x1000A038:
	case 0x1000A03C:
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
	case 0x1000B400:
		m_D4.WriteCHCR(nData);
		break;
	case 0x1000B404:
	case 0x1000B408:
	case 0x1000B40C:
		break;

	//D4_MADR
	case 0x1000B410:
		m_D4.m_nMADR = nData;
		break;
	case 0x1000B414:
	case 0x1000B418:
	case 0x1000B41C:
		break;

	//D4_QWC
	case 0x1000B420:
		m_D4.m_nQWC = nData;
		break;
	case 0x1000B424:
	case 0x1000B428:
	case 0x1000B42C:
		break;

	//D4_TADR
	case 0x1000B430:
		m_D4.m_nTADR = nData;
		break;
	case 0x1000B434:
	case 0x1000B438:
	case 0x1000B43C:
		break;

	//D5_CHCR
	case 0x1000C000:
		m_D5_CHCR = nData;
		if(m_D5_CHCR & 0x100)
		{
			memcpy(CPS2VM::m_pRAM + m_D5_MADR, CSIF::m_pRAM, m_D5_QWC * 0x10);
			m_D5_CHCR	&= ~0x100;
			m_D_STAT	|= 0x20;
			if(IsInterruptPending())
			{
				CINTC::CheckInterrupts();
			}
		}
		break;
	case 0x1000C004:
	case 0x1000C008:
	case 0x1000C00C:
		break;

	//D5_MADR
	case 0x1000C010:
		m_D5_MADR = nData;
		break;
	case 0x1000C014:
	case 0x1000C018:
	case 0x1000C01C:
		break;

	//D5_QWC
	case 0x1000C020:
		m_D5_QWC = nData;
	case 0x1000C024:
	case 0x1000C028:
	case 0x1000C02C:
		break;

	//D6_CHCR
	case 0x1000C400:
		m_D6_CHCR = nData;
		if(m_D6_CHCR & 0x100)
		{
			CSIF::ReceiveDMA(m_D6_MADR, m_D6_TADR, m_D6_QWC * 0x10);
			m_D6_CHCR &= ~0x100;
		}
		break;
	case 0x1000C404:
	case 0x1000C408:
	case 0x1000C40C:
		break;

	//D6_MADR
	case 0x1000C410:
		m_D6_MADR = nData;
		break;
	case 0x1000C414:
	case 0x1000C418:
	case 0x1000C41C:
		break;

	//D6_QWC
	case 0x1000C420:
		m_D6_QWC = nData;
		break;
	case 0x1000C424:
	case 0x1000C428:
	case 0x1000C42C:
		break;

	//D6_TADR
	case 0x1000C430:
		m_D6_TADR = nData;
		break;
	case 0x1000C434:
	case 0x1000C438:
	case 0x1000C43C:
		break;

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

	case 0x1000E010:
		uint32 nStat, nMask;
		nStat = nData & 0x0000FFFF;
		nMask = nData & 0xFFFF0000;

		//Set the masks
//		m_D_STAT &= 0x0000FFFF;
//		m_D_STAT |= nMask;
		m_D_STAT ^= nMask;

		//Clear the interrupt status
		m_D_STAT &= ~nStat;

		//Trigger interrupts
		if(IsInterruptPending())
		{
			CINTC::CheckInterrupts();
		}

		break;
	case 0x1000E014:
	case 0x1000E018:
	case 0x1000E01C:
		break;

	case D_ENABLEW + 0x0:
		m_D_ENABLE = nData;
		break;
	case D_ENABLEW + 0x4:
	case D_ENABLEW + 0x8:
	case D_ENABLEW + 0xC:
		break;

	default:
		printf("DMAC: Wrote to an unhandled IO port (0x%0.8X, 0x%0.8X, PC: 0x%0.8X).\r\n", nAddress, nData, CPS2VM::m_EE.m_State.nPC);
		break;
	}

	DisassembleSet(nAddress, nData);

#ifdef PROFILE
	CProfiler::GetInstance().EndZone();
#endif

}

void CDMAC::LoadState(CStream* pStream)
{
	pStream->Read(&m_D_STAT, 4);
}

void CDMAC::SaveState(CStream* pStream)
{
	pStream->Write(&m_D_STAT, 4);
}

void CDMAC::DisassembleGet(uint32 nAddress)
{
	if(!CPS2VM::m_Logging.GetDMACLoggingStatus()) return;

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
	if(!CPS2VM::m_Logging.GetDMACLoggingStatus()) return;

	switch(nAddress)
	{
	case 0x1000A000:
		printf("DMAC: D2_CHCR = 0x%0.8X.\r\n", nData);
		break;
	case 0x1000A010:
		printf("DMAC: D2_MADR = 0x%0.8X.\r\n", nData);
		break;
	case 0x1000A020:
		printf("DMAC: D2_SIZE = 0x%0.8X.\r\n", nData);
		break;
	case 0x1000A030:
		printf("DMAC: D2_TADR = 0x%0.8X.\r\n", nData);
		break;
	case D3_CHCR:
		printf("DMAC: D3_CHCR = 0x%0.8X.\r\n", nData);
		break;
	case D3_MADR:
		printf("DMAC: D3_MADR = 0x%0.8X.\r\n", nData);
		break;
	case D3_QWC:
		printf("DMAC: D3_QWC = 0x%0.8X.\r\n", nData);
		break;
	case 0x1000B400:
		printf("DMAC: D4_CHCR = 0x%0.8X.\r\n", nData);
		break;
	case 0x1000B410:
		printf("DMAC: D4_MADR = 0x%0.8X.\r\n", nData);
		break;
	case 0x1000B420:
		printf("DMAC: D4_QWC = 0x%0.8X.\r\n", nData);
		break;
	case 0x1000B430:
		printf("DMAC: D4_TADR = 0x%0.8X.\r\n", nData);
		break;
	case 0x1000C000:
		printf("DMAC: D5_CHCR = 0x%0.8X.\r\n", nData);
		break;
	case 0x1000C010:
		printf("DMAC: D5_MADR = 0x%0.8X.\r\n", nData);
		break;
	case 0x1000C020:
		printf("DMAC: D5_QWC = 0x%0.8X.\r\n", nData);
		break;
	case 0x1000C400:
		printf("DMAC: D6_CHCR = 0x%0.8X.\r\n", nData);
		break;
	case 0x1000C410:
		printf("DMAC: D6_MADR = 0x%0.8X.\r\n", nData);
		break;
	case 0x1000C420:
		printf("DMAC: D6_QWC = 0x%0.8X.\r\n", nData);
		break;
	case 0x1000C430:
		printf("DMAC: D6_TADR = 0x%0.8X.\r\n", nData);
		break;
	case D9_CHCR:
		printf("DMAC: D9_CHCR = 0x%0.8X.\r\n", nData);
		break;
	case D9_MADR:
		printf("DMAC: D9_MADR = 0x%0.8X.\r\n", nData);
		break;
	case D9_QWC:
		printf("DMAC: D9_QWC = 0x%0.8X.\r\n", nData);
		break;
	case D9_TADR:
		printf("DMAC: D9_TADR = 0x%0.8X.\r\n", nData);
		break;
	case D9_SADR:
		printf("DMAC: D9_SADR = 0x%0.8X.\r\n", nData);
		break;
	case 0x1000E010:
		printf("DMAC: D_STAT = 0x%0.8X. (PC = 0x%0.8X)\r\n", nData, CPS2VM::m_EE.m_State.nPC);
		break;
	case D_ENABLEW:
		printf("DMAC: D_ENABLEW = 0x%0.8X. (PC = 0x%0.8X)\r\n", nData, CPS2VM::m_EE.m_State.nPC);
		break;
	}
}

////////////////////////////////////////
//Channel class implementation
////////////////////////////////////////

CDMAC::CChannel::CChannel(unsigned int nNumber, DMARECEIVEMETHOD pReceive, DMASLICEDONECALLBACK pSliceDone)
{
	m_nNumber		= nNumber;
	m_pReceive		= pReceive;
	m_pSliceDone	= pSliceDone;
}

void CDMAC::CChannel::Reset()
{
	memset(&m_CHCR, 0, sizeof(CHCR));
	m_nMADR		= 0;
	m_nQWC		= 0;
	m_nTADR		= 0;
	m_nSCCTRL	= 0;
}

uint32 CDMAC::CChannel::ReadCHCR()
{
	return *(uint32*)&m_CHCR;
}

void CDMAC::CChannel::WriteCHCR(uint32 nValue)
{
	bool nSuspend;

	nSuspend = false;

	//We need to check if the purpose of this write is to suspend the current transfer
	if(m_D_ENABLE & ENABLE_CPND)
	{
		nSuspend = (m_CHCR.nSTR != 0) && ((nValue & CHCR_STR) == 0);
		//nSuspend = (((m_nCHCR & CHCR_STR) != 0) & ((nValue & CHCR_STR) == 0));
	}

	if(m_CHCR.nSTR == 1)
	{
		m_CHCR.nSTR = ~m_CHCR.nSTR;
		m_CHCR.nSTR = ((nValue & CHCR_STR) != 0) ? 1 : 0;
		//m_nCHCR &= ~CHCR_STR;
		//m_nCHCR |= (nValue & CHCR_STR);
	}
	else
	{
		m_CHCR = *(CHCR*)&nValue;
		//m_nCHCR = nValue;
	}

	if(m_CHCR.nSTR != 0)
	{
		switch(m_CHCR.nMOD)
		{
		case 0x00:
			ExecuteNormal();
			break;
		case 0x01:
			//if(m_D4_QWC == 0)
			//{
			//	m_D4_CHCR |= CHCR_INITXFER;
			//}
			//m_D4_CHCR |= CHCR_INITXFER;

			//If transfer was suspended
			if(m_nSCCTRL & SCCTRL_SUSPENDED)
			{
				m_nSCCTRL &= (~SCCTRL_SUSPENDED);
			}
			else
			{
				m_nSCCTRL |= SCCTRL_INITXFER;
			}

			ExecuteSourceChain();
			break;
		default:
			assert(0);
			break;
		}
	}
	else
	{
		if(nSuspend)
		{
			m_nSCCTRL |= SCCTRL_SUSPENDED;
		}
	}
}

void CDMAC::CChannel::ExecuteNormal()
{
	uint32 nRecv;

	nRecv = m_pReceive(m_nMADR, m_nQWC, false);

	m_nMADR	+= nRecv * 0x10;
	m_nQWC	-= nRecv;

	if(m_nQWC == 0)
	{
		ClearSTR();
	}

	if(m_pSliceDone != NULL)
	{
		m_pSliceDone();
	}
}

void CDMAC::CChannel::ExecuteSourceChain()
{
	uint64 nTag;
	uint32 nRecv;
	uint8 nID;

	//Execute current
	if(m_nQWC != 0)
	{
		nRecv = m_pReceive(m_nMADR, m_nQWC, false);

		m_nMADR	+= nRecv * 0x10;
		m_nQWC	-= nRecv;

		if(m_nQWC != 0)
		{
			//Transfer isn't finished, suspend for now
			if(m_pSliceDone != NULL)
			{
				m_pSliceDone();
			}
			return;
		}
	}

	while(m_CHCR.nSTR == 1)
	{
		//Check if we've finished our DMA transfer
		if(m_nQWC == 0)
		{
			if(m_nSCCTRL & SCCTRL_INITXFER)
			{
				//Clear this bit
				m_nSCCTRL &= ~SCCTRL_INITXFER;
			}
			else
			{
				if(IsEndTagId((uint32)m_CHCR.nTAG << 16))
				{
					ClearSTR();
					continue;
				}
			}
		}
		else
		{
			//Suspend transfer
			break;
		}

		if(m_CHCR.nTTE == 1)
		{
			m_pReceive(m_nTADR, 1, true);
		}

		nTag = FetchDMATag(m_nTADR);

		//Save higher 16 bits of tag into CHCR
		m_CHCR.nTAG = nTag >> 16;
		//m_nCHCR &= ~0xFFFF0000;
		//m_nCHCR |= nTag & 0xFFFF0000;

		nID = (uint8)((nTag >> 28) & 0x07);

		switch(nID)
		{
		case 0:
			//REFE - Data to transfer is pointer in memory address, transfer is done
			m_nMADR		= (uint32)((nTag >>  32) & 0xFFFFFFFF);
			m_nQWC		= (uint32)((nTag >>   0) & 0x0000FFFF);
			m_nTADR		= m_nTADR + 0x10;
			break;
		case 1:
			//CNT - Data to transfer is after the tag, next tag is after the data
			m_nMADR		= m_nTADR + 0x10;
			m_nQWC		= (uint32)(nTag & 0xFFFF);
			m_nTADR		= (m_nQWC * 0x10) + m_nMADR;
			break;
		case 2:
			//NEXT - Transfers data after tag, next tag is at position in ADDR field
			m_nMADR		= m_nTADR + 0x10;
			m_nQWC		= (uint32)((nTag >>   0) & 0x0000FFFF);
			m_nTADR		= (uint32)((nTag >>  32) & 0xFFFFFFFF);
			break;
		case 3:
			//REF - Data to transfer is pointed in memory address, next tag is after this tag
			m_nMADR		= (uint32)((nTag >>  32) & 0xFFFFFFFF);
			m_nQWC		= (uint32)((nTag >>   0) & 0x0000FFFF);
			m_nTADR		= m_nTADR + 0x10;
			break;
		case 5:
			//CALL - Transfers QWC after the tag, saves next address in ASR, TADR = ADDR
			assert(m_CHCR.nASP < 2);
			m_nMADR				= m_nTADR + 0x10;
			m_nQWC				= (uint32)(nTag & 0xFFFF);
			m_nASR[m_CHCR.nASP]	= m_nMADR + (m_nQWC * 0x10);
			m_nTADR				= (uint32)((nTag >>  32) & 0xFFFFFFFF);
			m_CHCR.nASP++;
			break;
		case 6:
			//RET - Transfers QWC after the tag, pops TADR from ASR
			assert(m_CHCR.nASP > 0);
			m_CHCR.nASP--;

			m_nMADR		= m_nTADR + 0x10;
			m_nQWC		= (uint32)(nTag & 0xFFFF);
			m_nTADR		= m_nASR[m_CHCR.nASP];
			break;
		case 7:
			//END - Data to transfer is after the tag, transfer is finished
			m_nMADR		= m_nTADR + 0x10;
			m_nQWC		= (uint32)(nTag & 0xFFFF);
			break;
		default:
			m_nQWC = 0;
			assert(0);
			break;
		}

		if(m_nQWC != 0)
		{
			nRecv = m_pReceive(m_nMADR, m_nQWC, false);

			m_nMADR		+= nRecv * 0x10;
			m_nQWC		-= nRecv;
		}
	}

	if(m_pSliceDone != NULL)
	{
		m_pSliceDone();
	}
}

void CDMAC::CChannel::ClearSTR()
{
	m_CHCR.nSTR = ~m_CHCR.nSTR;

	//Set interrupt
	m_D_STAT |= (1 << m_nNumber);
	if(IsInterruptPending())
	{
		CINTC::CheckInterrupts();
	}
}
