#include <string.h>
#include <assert.h>
#include "Dmac_Channel.h"
#include "DMAC.h"

using namespace Dmac;

CChannel::CChannel(CDMAC& dmac, unsigned int nNumber, const DmaReceiveHandler& pReceive, DMASLICEDONECALLBACK pSliceDone) :
m_dmac(dmac)
{
	m_nNumber		= nNumber;
	m_pReceive		= pReceive;
	m_pSliceDone	= pSliceDone;
}

CChannel::~CChannel()
{

}

void CChannel::Reset()
{
	memset(&m_CHCR, 0, sizeof(CHCR));
	m_nMADR		= 0;
	m_nQWC		= 0;
	m_nTADR		= 0;
	m_nSCCTRL	= 0;
}

uint32 CChannel::ReadCHCR()
{
	return *(uint32*)&m_CHCR;
}

void CChannel::WriteCHCR(uint32 nValue)
{
	bool nSuspend;

	nSuspend = false;

	//We need to check if the purpose of this write is to suspend the current transfer
    if(m_dmac.m_D_ENABLE & CDMAC::ENABLE_CPND)
	{
        nSuspend = (m_CHCR.nSTR != 0) && ((nValue & CDMAC::CHCR_STR) == 0);
		//nSuspend = (((m_nCHCR & CHCR_STR) != 0) & ((nValue & CHCR_STR) == 0));
	}

	if(m_CHCR.nSTR == 1)
	{
		m_CHCR.nSTR = ~m_CHCR.nSTR;
        m_CHCR.nSTR = ((nValue & CDMAC::CHCR_STR) != 0) ? 1 : 0;
		//m_nCHCR &= ~CHCR_STR;
		//m_nCHCR |= (nValue & CHCR_STR);
	}
	else
	{
		m_CHCR = *(CHCR*)&nValue;
		//m_nCHCR = nValue;
	}

	if(m_CHCR.nSTR == 0)
	{
		if(nSuspend)
		{
			m_nSCCTRL |= SCCTRL_SUSPENDED;
		}
	}

    //Wake up DMAC thread
    Execute();
//    m_CHCR.nSTR = 0;
//    m_dmac.m_waitCondition.notify_all();
}

void CChannel::Execute()
{
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
}

void CChannel::ExecuteNormal()
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

void CChannel::ExecuteSourceChain()
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
                if(CDMAC::IsEndTagId((uint32)m_CHCR.nTAG << 16))
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

		nTag = m_dmac.FetchDMATag(m_nTADR);

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

void CChannel::SetReceiveHandler(const DmaReceiveHandler& handler)
{
    m_pReceive = handler;
}

void CChannel::ClearSTR()
{
	m_CHCR.nSTR = ~m_CHCR.nSTR;

	//Set interrupt
	m_dmac.m_D_STAT |= (1 << m_nNumber);
	if(m_dmac.IsInterruptPending())
	{
//		CINTC::CheckInterrupts();
	}
}
