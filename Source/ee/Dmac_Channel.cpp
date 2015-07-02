#include <string.h>
#include <assert.h>
#include <boost/lexical_cast.hpp>
#include "../RegisterStateFile.h"
#include "../Log.h"
#include "Dmac_Channel.h"
#include "DMAC.h"

#define LOG_NAME				("dmac")
#define STATE_PREFIX			("dmac/channel_")
#define STATE_SUFFIX			(".xml")
#define STATE_REGS_CHCR			("CHCR")
#define STATE_REGS_MADR			("MADR")
#define STATE_REGS_QWC			("QWC")
#define STATE_REGS_TADR			("TADR")
#define STATE_REGS_SCCTRL		("SCCTRL")
#define STATE_REGS_ASR0			("ASR0")
#define STATE_REGS_ASR1			("ASR1")

using namespace Dmac;

CChannel::CChannel(CDMAC& dmac, unsigned int nNumber, const DmaReceiveHandler& pReceive) 
: m_dmac(dmac)
, m_nNumber(nNumber)
, m_pReceive(pReceive)
{

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
	m_nASR[0]	= 0;
	m_nASR[1]	= 0;
}

void CChannel::SaveState(Framework::CZipArchiveWriter& archive)
{
	std::string path = STATE_PREFIX + boost::lexical_cast<std::string>(m_nNumber) + STATE_SUFFIX;
	CRegisterStateFile* registerFile = new CRegisterStateFile(path.c_str());
	registerFile->SetRegister32(STATE_REGS_CHCR,	m_CHCR);
	registerFile->SetRegister32(STATE_REGS_MADR,	m_nMADR);
	registerFile->SetRegister32(STATE_REGS_QWC,		m_nQWC);
	registerFile->SetRegister32(STATE_REGS_TADR,	m_nTADR);
	registerFile->SetRegister32(STATE_REGS_SCCTRL,	m_nSCCTRL);
	registerFile->SetRegister32(STATE_REGS_ASR0,	m_nASR[0]);
	registerFile->SetRegister32(STATE_REGS_ASR1,	m_nASR[1]);
	archive.InsertFile(registerFile);
}

void CChannel::LoadState(Framework::CZipArchiveReader& archive)
{
	std::string path = STATE_PREFIX + boost::lexical_cast<std::string>(m_nNumber) + STATE_SUFFIX;
	CRegisterStateFile registerFile(*archive.BeginReadFile(path.c_str()));
	m_CHCR		<<= registerFile.GetRegister32(STATE_REGS_CHCR);
	m_nMADR		= registerFile.GetRegister32(STATE_REGS_MADR);
	m_nQWC		= registerFile.GetRegister32(STATE_REGS_QWC);
	m_nTADR		= registerFile.GetRegister32(STATE_REGS_TADR);
	m_nSCCTRL	= registerFile.GetRegister32(STATE_REGS_SCCTRL);
	m_nASR[0]	= registerFile.GetRegister32(STATE_REGS_ASR0);
	m_nASR[1]	= registerFile.GetRegister32(STATE_REGS_ASR1);
}

uint32 CChannel::ReadCHCR()
{
	return *(uint32*)&m_CHCR;
}

void CChannel::WriteCHCR(uint32 nValue)
{
	bool nSuspend = false;

	//We need to check if the purpose of this write is to suspend the current transfer
	if(m_dmac.m_D_ENABLE & CDMAC::ENABLE_CPND)
	{
		nSuspend = (m_CHCR.nSTR != 0) && ((nValue & CDMAC::CHCR_STR) == 0);
	}

	if(m_CHCR.nSTR == 1)
	{
		m_CHCR.nSTR = ((nValue & CDMAC::CHCR_STR) != 0) ? 1 : 0;
	}
	else
	{
		m_CHCR = *(CHCR*)&nValue;
	}

	if(m_CHCR.nSTR != 0)
	{
		if(m_nQWC == 0)
		{
			m_nSCCTRL |= SCCTRL_INITXFER;
		}
		m_nSCCTRL &= ~SCCTRL_RETTOP;
		Execute();
	}
}

void CChannel::Execute()
{
	if(m_CHCR.nSTR != 0)
	{
		if(m_dmac.m_D_ENABLE)
		{
			//TODO: Need to check cases where this is done on channels other than 4
			assert(m_nNumber == 4);
			return;
		}
		if((m_nNumber == 1) && (m_CHCR.nDIR == 0))
		{
			//Humm, destination mode, not supported for now.
			CLog::GetInstance().Print(LOG_NAME, "Warning: Using destination mode for channel %d. Cancelling transfer.\r\n", m_nNumber);
			ClearSTR();
			return;
		}
		switch(m_CHCR.nMOD)
		{
		case 0x00:
			ExecuteNormal();
			break;
		case 0x01:
		case 0x03:		//FFXII uses 3 here, assuming source chain mode
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
	uint32 qwc = m_nQWC;

	if(m_dmac.m_D_CTRL.mfd == 0x02 && m_nNumber == 8)
	{
		//Adjust QWC if we're in MFIFO mode
		uint32 ringBufferAddr = m_nMADR - m_dmac.m_D_RBOR;
		uint32 ringBufferSize = m_dmac.m_D_RBSR + 0x10;
		assert(ringBufferAddr < ringBufferSize);

		qwc = std::min<int32>(m_nQWC, (ringBufferSize - ringBufferAddr) / 0x10);
	}

	uint32 nRecv = m_pReceive(m_nMADR, qwc, 0, false);

	m_nMADR	+= nRecv * 0x10;
	m_nQWC	-= nRecv;

	if(m_nQWC == 0)
	{
		ClearSTR();
	}

	if(m_dmac.m_D_CTRL.mfd == 0x02 && m_nNumber == 8)
	{
		//Loop MADR if needed
		uint32 ringBufferSize = m_dmac.m_D_RBSR + 0x10;
		if(m_nMADR == (m_dmac.m_D_RBOR + ringBufferSize))
		{
			m_nMADR = m_dmac.m_D_RBOR;
		}
	}
}

void CChannel::ExecuteSourceChain()
{
	//Execute current
	if(m_nQWC != 0)
	{
		uint32 nRecv = m_pReceive(m_nMADR, m_nQWC, 0, false);

		m_nMADR	+= nRecv * 0x10;
		m_nQWC	-= nRecv;

		if(m_nQWC != 0)
		{
			//Transfer isn't finished, suspend for now
			return;
		}
	}

	while(m_CHCR.nSTR == 1)
	{
		//Check if MFIFO is enabled with this channel
		if(m_dmac.m_D_CTRL.mfd == 0x02 && m_nNumber == 1)
		{
			//Hold transfer if not ready
			if(m_nTADR == m_dmac.m_D8.m_nMADR)
			{
				break;
			}
		}

		//Check if device received DMAtag
		if(m_CHCR.nReserved0)
		{
			assert(m_CHCR.nTTE);
			m_CHCR.nReserved0 = 0;
			if(m_pReceive(m_nTADR, 1, 0, true) != 1)
			{
				//Device didn't receive DmaTag, break for now
				m_CHCR.nReserved0 = 1;
				break;
			}
		}
		else
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

					//Transfer must also end when we encounter a RET tag with ASR == 0
					if(m_nSCCTRL & SCCTRL_RETTOP)
					{
						ClearSTR();
						m_nSCCTRL &= ~SCCTRL_RETTOP;
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
				m_CHCR.nReserved0 = 0;
				if(m_pReceive(m_nTADR, 1, 0, true) != 1)
				{
					//Device didn't receive DmaTag, break for now
					m_CHCR.nReserved0 = 1;
					break;
				}
			}
		}

		//Half-Life does this...
		if(m_nTADR == 0)
		{
			ClearSTR();
			continue;
		}

		uint64 nTag = m_dmac.FetchDMATag(m_nTADR);

		//Save higher 16 bits of tag into CHCR
		m_CHCR.nTAG = nTag >> 16;
		//m_nCHCR &= ~0xFFFF0000;
		//m_nCHCR |= nTag & 0xFFFF0000;

		uint8 nID = static_cast<uint8>((nTag >> 28) & 0x07);

		switch(nID)
		{
		case DMATAG_REFE:
			//REFE - Data to transfer is pointer in memory address, transfer is done
			m_nMADR		= (uint32)((nTag >>  32) & 0xFFFFFFFF);
			m_nQWC		= (uint32)((nTag >>   0) & 0x0000FFFF);
			m_nTADR		= m_nTADR + 0x10;
			break;
		case DMATAG_CNT:
			//CNT - Data to transfer is after the tag, next tag is after the data
			m_nMADR		= m_nTADR + 0x10;
			m_nQWC		= (uint32)(nTag & 0xFFFF);
			m_nTADR		= (m_nQWC * 0x10) + m_nMADR;
			break;
		case DMATAG_NEXT:
			//NEXT - Transfers data after tag, next tag is at position in ADDR field
			m_nMADR		= m_nTADR + 0x10;
			m_nQWC		= (uint32)((nTag >>   0) & 0x0000FFFF);
			m_nTADR		= (uint32)((nTag >>  32) & 0xFFFFFFFF);
			break;
		case DMATAG_REF:
		case DMATAG_REFS:
			//REF/REFS - Data to transfer is pointed in memory address, next tag is after this tag
			m_nMADR		= (uint32)((nTag >>  32) & 0xFFFFFFFF);
			m_nQWC		= (uint32)((nTag >>   0) & 0x0000FFFF);
			m_nTADR		= m_nTADR + 0x10;
			break;
		case DMATAG_CALL:
			//CALL - Transfers QWC after the tag, saves next address in ASR, TADR = ADDR
			assert(m_CHCR.nASP < 2);
			m_nMADR				= m_nTADR + 0x10;
			m_nQWC				= (uint32)(nTag & 0xFFFF);
			m_nASR[m_CHCR.nASP]	= m_nMADR + (m_nQWC * 0x10);
			m_nTADR				= (uint32)((nTag >>  32) & 0xFFFFFFFF);
			m_CHCR.nASP++;
			break;
		case DMATAG_RET:
			//RET - Transfers QWC after the tag, pops TADR from ASR
			m_nMADR = m_nTADR + 0x10;
			m_nQWC = (uint32)(nTag & 0xFFFF);
			if(m_CHCR.nASP > 0)
			{
				m_CHCR.nASP--;
				m_nTADR = m_nASR[m_CHCR.nASP];
			}
			else 
			{
				m_nSCCTRL |= SCCTRL_RETTOP;
			}
			break;
		case DMATAG_END:
			//END - Data to transfer is after the tag, transfer is finished
			m_nMADR		= m_nTADR + 0x10;
			m_nQWC		= (uint32)(nTag & 0xFFFF);
			break;
		default:
			m_nQWC = 0;
			assert(0);
			break;
		}

		uint32 qwc = m_nQWC;
		if(nID == DMATAG_CNT && m_dmac.m_D_CTRL.mfd == 0x02 && m_nNumber == 1)
		{
			//Adjust QWC in MFIFO mode
			uint32 ringBufferAddr = m_nMADR - m_dmac.m_D_RBOR;
			uint32 ringBufferSize = m_dmac.m_D_RBSR + 0x10;
			assert(ringBufferAddr <= ringBufferSize);

			qwc = std::min<int32>(m_nQWC, (ringBufferSize - ringBufferAddr) / 0x10);
		}

		if(qwc != 0)
		{
			uint32 nRecv = m_pReceive(m_nMADR, qwc, 0, false);

			m_nMADR		+= nRecv * 0x10;
			m_nQWC		-= nRecv;
		}

		if(m_dmac.m_D_CTRL.mfd == 0x02 && m_nNumber == 1)
		{
			if(nID == DMATAG_CNT)
			{
				//Loop MADR if needed
				uint32 ringBufferSize = m_dmac.m_D_RBSR + 0x10;
				if(m_nMADR == (m_dmac.m_D_RBOR + ringBufferSize))
				{
					m_nMADR = m_dmac.m_D_RBOR;
				}
			}

			//Mask TADR because it's in the ring buffer zone
			m_nTADR -= m_dmac.m_D_RBOR;
			m_nTADR &= m_dmac.m_D_RBSR;
			m_nTADR += m_dmac.m_D_RBOR;
		}
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

	m_dmac.UpdateCpCond();
}
