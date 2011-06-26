#include <stdio.h>
#include <exception>
#include <functional>
#include "IPU.h"
#include "IPU_MacroblockAddressIncrementTable.h"
#include "IPU_MacroblockTypeITable.h"
#include "IPU_MacroblockTypePTable.h"
#include "IPU_MacroblockTypeBTable.h"
#include "IPU_MotionCodeTable.h"
#include "IPU_DmVectorTable.h"
#include "mpeg2/DcSizeLuminanceTable.h"
#include "mpeg2/DcSizeChrominanceTable.h"
#include "mpeg2/DctCoefficientTable0.h"
#include "mpeg2/DctCoefficientTable1.h"
#include "mpeg2/CodedBlockPatternTable.h"
#include "mpeg2/QuantiserScaleTable.h"
#include "mpeg2/InverseScanTable.h"
#include "idct/TrivialC.h"
#include "idct/IEEE1180.h"
#include "DMAC.h"
#include "Log.h"
#include "PtrMacro.h"

#define LOG_NAME ("ipu")
//#define _DECODE_LOGGING

using namespace IPU;
using namespace MPEG2;
using namespace Framework;

CIPU::CIPU() 
: m_IPU_CTRL(0)
, m_cmdThread(NULL)
, m_isBusy(false)
, m_currentCmd(NULL)
, m_currentCmdCode(0)
, m_cmdThreadOver(false)
{
	m_cmdThread = new boost::thread(std::tr1::bind(&CIPU::CommandThread, this));
}

CIPU::~CIPU()
{
	m_cmdThreadOver = true;
	m_cmdThread->join();
	DELETEPTR(m_cmdThread);
}

void CIPU::Reset()
{
	m_IPU_CTRL			= 0;
	m_IPU_CMD[0]		= 0;
	m_IPU_CMD[1]		= 0;

    m_busyWhileReadingCMD = false;
    m_busyWhileReadingTOP = false;
}

uint32 CIPU::GetRegister(uint32 nAddress)
{
#ifdef _DEBUG
//	DisassembleGet(nAddress);
#endif

	switch(nAddress)
	{
	case IPU_CMD + 0x0:
        m_busyWhileReadingCMD = m_isBusy;
		return m_IPU_CMD[0];
		break;
	case IPU_CMD + 0x4:
        return GetBusyBit(m_busyWhileReadingCMD);
		break;
	case IPU_CMD + 0x8:
	case IPU_CMD + 0xC:
		break;

	case IPU_CTRL + 0x0:
		return m_IPU_CTRL | GetBusyBit(m_isBusy);
		break;
	case IPU_CTRL + 0x4:
	case IPU_CTRL + 0x8:
	case IPU_CTRL + 0xC:
		break;

	case IPU_BP + 0x0:
        assert(!m_isBusy);
		return ((m_IN_FIFO.GetSize() / 0x10) << 8) | (m_IN_FIFO.GetBitIndex());
		break;

	case IPU_BP + 0x4:
	case IPU_BP + 0x8:
	case IPU_BP + 0xC:
		break;

	case IPU_TOP + 0x0:
        m_busyWhileReadingTOP = m_isBusy;
        if(!m_isBusy)
        {
//            if(m_IN_FIFO.GetSize() < 4)
//            {
//				throw std::runtime_error("Not enough data...");
//            }
			unsigned int availableSize = std::min<unsigned int>(32, m_IN_FIFO.GetAvailableBits());
            return m_IN_FIFO.PeekBits_MSBF(availableSize);
        }
        else
        {
            return 0;
        }
		break;

	case IPU_TOP + 0x4:
        return GetBusyBit(m_busyWhileReadingTOP);
        break;

	case IPU_TOP + 0x8:
	case IPU_TOP + 0xC:
		break;

	default:
		CLog::GetInstance().Print(LOG_NAME, "Reading an unhandled register (0x%0.8X).\r\n", nAddress);
		break;
	}

	return 0;
}

void CIPU::SetRegister(uint32 nAddress, uint32 nValue)
{
#ifdef _DEBUG
	DisassembleSet(nAddress, nValue);
#endif

	switch(nAddress)
	{
	case IPU_CMD + 0x0:
	    //Set BUSY states
		{
			boost::mutex::scoped_lock cmdMutexLock(m_cmdMutex);
			assert(m_isBusy == false);
			m_currentCmdCode = nValue;
			m_isBusy = true;
			m_cmdCondition.notify_one();
		}
#ifdef _DEBUG
		DisassembleCommand(nValue);
#endif
		break;
	case IPU_CMD + 0x4:
	case IPU_CMD + 0x8:
	case IPU_CMD + 0xC:
		break;

	case IPU_CTRL + 0x0:
        if((nValue & 0x40000000) && m_isBusy)
        {
			throw std::runtime_error("Humm...");
        }
		nValue &= 0x3FFF0000;
		m_IPU_CTRL &= ~0x3FFF0000;
		m_IPU_CTRL |= nValue;
		break;
	case IPU_CTRL + 0x4:
	case IPU_CTRL + 0x8:
	case IPU_CTRL + 0xC:
		break;

	case IPU_IN_FIFO + 0x0:
	case IPU_IN_FIFO + 0x4:
	case IPU_IN_FIFO + 0x8:
	case IPU_IN_FIFO + 0xC:
		m_IN_FIFO.Write(&nValue, 4);
		break;

	default:
		CLog::GetInstance().Print(LOG_NAME, "Writing 0x%0.8X to an unhandled register (0x%0.8X).\r\n", nValue, nAddress);
		break;
	}
}

void CIPU::CommandThread()
{
    while(!m_cmdThreadOver)
    {
		if(m_isBusy)
		{
			if(m_currentCmd == NULL)
			{
				InitializeCommand(m_currentCmdCode);
			}

			try
			{
				m_currentCmd->Execute();
				m_currentCmd = NULL;

				//Clear BUSY states
				m_isBusy = false;
			}
			catch(const Framework::CBitStream::CBitStreamException&)
			{
				m_IN_FIFO.WaitForData();
			}
		}
		else
		{
			boost::mutex::scoped_lock cmdMutexLock(m_cmdMutex);
			boost::xtime xt;
			boost::xtime_get(&xt, boost::TIME_UTC);
			xt.nsec += 1000000;
			m_cmdCondition.timed_wait(cmdMutexLock, xt);
		}
    }
}

void CIPU::InitializeCommand(uint32 value)
{
    unsigned int nCmd = (value >> 28);

	switch(nCmd)
	{
	case 0:
		//BCLR
		{
			m_BCLRCommand.Initialize(&m_IN_FIFO, value);
			m_currentCmd = &m_BCLRCommand;
		}
		break;
//	case 1:
//		//IDEC
//		DecodeIntra( \
//			static_cast<uint8>((nValue >> 27) & 1), \
//			static_cast<uint8>((nValue >> 26) & 1), \
//			static_cast<uint8>((nValue >> 25) & 1), \
//			static_cast<uint8>((nValue >> 24) & 1), \
//			static_cast<uint8>((nValue >> 16) & 0x1F), \
//			static_cast<uint8>((nValue >>  0) & 0x3F));
//		break;
	case 2:
		//BDEC
		{
			CBDECCommand::CONTEXT context;
			context.isMpeg1			= GetIsMPEG1CoeffVLCTable();
			context.isZigZag		= GetIsZigZagScan();
			context.isLinearQScale	= GetIsLinearQScale();
			context.dcPrecision		= GetDcPrecision();
			context.intraIq			= m_nIntraIQ;
			context.nonIntraIq		= m_nNonIntraIQ;
			context.dcPredictor		= m_nDcPredictor;
			m_BDECCommand.Initialize(&m_IN_FIFO, &m_OUT_FIFO, value, context);
			m_currentCmd = &m_BDECCommand;
		}
		break;
	case 3:
		//VDEC
		{
			m_VDECCommand.Initialize(&m_IN_FIFO, value, GetPictureType(), &m_IPU_CMD[0]);
			m_currentCmd = &m_VDECCommand;
		}
		break;
	case 4:
		//FDEC
		{
			m_FDECCommand.Initialize(&m_IN_FIFO, value, &m_IPU_CMD[0]);
			m_currentCmd = &m_FDECCommand;
		}
		break;
	case 5:
		//SETIQ
		{
			uint8* matrix = (value & 0x08000000) ? m_nNonIntraIQ : m_nIntraIQ;
			m_SETIQCommand.Initialize(&m_IN_FIFO, matrix);
			m_currentCmd = &m_SETIQCommand;
		}
		break;
	case 6:
		//SETVQ
		{
			m_SETVQCommand.Initialize(&m_IN_FIFO, m_nVQCLUT);
			m_currentCmd = &m_SETVQCommand;
		}
		break;
	case 7:
		//CSC
		{
			m_CSCCommand.Initialize(&m_IN_FIFO, &m_OUT_FIFO, value);
			m_currentCmd = &m_CSCCommand;
		}
		break;
	case 9:
		//SETTH
		{
			m_SETTHCommand.Initialize(value, &m_nTH0, &m_nTH1);
			m_currentCmd = &m_SETTHCommand;
		}
		break;
	default:
		assert(0);
		CLog::GetInstance().Print(LOG_NAME, "Unhandled command execution requested (%d).\r\n", value >> 28);
		break;
	}
}

void CIPU::SetDMA3ReceiveHandler(const Dma3ReceiveHandler& receiveHandler)
{
    m_OUT_FIFO.SetReceiveHandler(receiveHandler);
}

uint32 CIPU::ReceiveDMA4(uint32 nAddress, uint32 nQWC, bool nTagIncluded, uint8* ram)
{
    assert(nTagIncluded == false);

	uint32 totalSize = 0;

	while(1)
	{
		bool done = false;
		uint32 availableFifoSize = 0;

		while(1)
		{
			availableFifoSize = CINFIFO::BUFFERSIZE - m_IN_FIFO.GetSize();
			if(availableFifoSize == 0)
			{
				if(!m_isBusy)
				{
					done = true;
					break;
				}
				else
				{
					m_IN_FIFO.WaitForSpace();
				}
			}
			else
			{
				break;
			}
		}

		if(done)
		{
			break;
		}

		uint32 nSize = std::min<uint32>(nQWC * 0x10, availableFifoSize);
		assert((nSize & 0xF) == 0);

		m_IN_FIFO.Write(ram + nAddress, nSize);

		nAddress += nSize;
		totalSize += nSize;
		nQWC -= (nSize / 0x10);

		if(nQWC == 0)
		{
			break;
		}
	}

	return totalSize / 0x10;
}
/*
void CIPU::DecodeIntra(uint8 nOFM, uint8 nDTE, uint8 nSGN, uint8 nDTD, uint8 nQSC, uint8 nFB)
{
	assert(0);

	CIDecFifo IDecFifo;

	m_IN_FIFO.Advance(nFB);

	bool nResetDc = true;

	while(1)
	{
		uint8 nDCTType;

		uint32 nMBType = CMacroblockTypeITable::GetInstance()->GetSymbol(&m_IN_FIFO) >> 16;

		if(nDTD != 0)
		{
			nDCTType = static_cast<uint8>(m_IN_FIFO.GetBits_MSBF(1));
		}
		else
		{
			nDCTType = 0;
		}

		if(nMBType == 2)
		{
			nQSC = static_cast<uint8>(m_IN_FIFO.GetBits_MSBF(5));
		}

		IDecFifo.Reset();

		DecodeBlock(&IDecFifo, 1, nResetDc, nDCTType, nQSC, 0);
		nResetDc = false;

		ColorSpaceConversion(&IDecFifo, nOFM, nDTE, 1);

		if(m_IN_FIFO.PeekBits_MSBF(23) == 0)
		{
			//We're done decoding
			break;
		}
		
		uint32 nMBAIncrement = CMacroblockAddressIncrementTable::GetInstance()->GetSymbol(&m_IN_FIFO) >> 16;
		assert(nMBAIncrement == 1);
	}
}
*/

uint32 CIPU::GetPictureType()
{
	return (m_IPU_CTRL >> 24) & 0x7;
}

uint32 CIPU::GetDcPrecision()
{
	return (m_IPU_CTRL >> 16) & 0x3;
}

bool CIPU::GetIsMPEG2()
{
	return (m_IPU_CTRL & 0x00800000) == 0;
}

bool CIPU::GetIsLinearQScale()
{
	return (m_IPU_CTRL & 0x00400000) == 0;
}

bool CIPU::GetIsMPEG1CoeffVLCTable()
{
	return (m_IPU_CTRL & 0x00200000) == 0;
}

bool CIPU::GetIsZigZagScan()
{
	return (m_IPU_CTRL & 0x00100000) == 0;
}

void CIPU::DequantiseBlock(int16* pBlock, uint8 nMBI, uint8 nQSC, bool isLinearQScale, uint32 dcPrecision, uint8* intraIq, uint8* nonIntraIq)
{
	int16 nQuantScale;

	if(isLinearQScale)
	{
		nQuantScale = (int16)CQuantiserScaleTable::m_nTable0[nQSC];
	}
	else
	{
		nQuantScale = (int16)CQuantiserScaleTable::m_nTable1[nQSC];
	}

	if(nMBI == 1)
	{
		int16 nIntraDcMult = 0;

		switch(dcPrecision)
		{
		case 0:
			nIntraDcMult = 8;
			break;
		case 1:
			nIntraDcMult = 4;
			break;
		case 2:
			nIntraDcMult = 2;
			break;
		}

		pBlock[0] = nIntraDcMult * pBlock[0];
		
		for(unsigned int i = 1; i < 64; i++)
		{
			int16 nSign = 0;

			if(pBlock[i] == 0)
			{
				nSign = 0;
			}
			else
			{
				nSign = (pBlock[i] > 0) ? 0x0001 : 0xFFFF;
			}

			pBlock[i] = (pBlock[i] * static_cast<int16>(intraIq[i]) * nQuantScale * 2) / 32;

			if(nSign != 0)
			{
				if((pBlock[i] & 1) == 0)
				{
					pBlock[i] = (pBlock[i] - nSign) | 1;
				}
			}
		}
	}
	else
	{
		for(unsigned int i = 0; i < 64; i++)
		{
			int16 nSign = 0;

			if(pBlock[i] == 0)
			{
				nSign = 0;
			}
			else
			{
				nSign = (pBlock[i] > 0) ? 0x0001 : 0xFFFF;
			}

			pBlock[i] = (((pBlock[i] * 2) + nSign) * static_cast<int16>(nonIntraIq[i]) * nQuantScale) / 32;

			if(nSign != 0)
			{
				if((pBlock[i] & 1) == 0)
				{
					pBlock[i] = (pBlock[i] - nSign) | 1;
				}
			}
		}
	}

	//Saturate
	int16 nSum = 0;

	for(unsigned int i = 0; i < 64; i++)
	{
		if(pBlock[i] > 2047)
		{
			pBlock[i] = 2047;
		}
		if(pBlock[i] < -2048)
		{
			pBlock[i] = -2048;
		}
		nSum += pBlock[i];
	}

	if(nSum & 1)
	{

	}
}

void CIPU::InverseScan(int16* pBlock, bool isZigZag)
{
	int16 nTemp[0x40];

	memcpy(nTemp, pBlock, sizeof(int16) * 0x40);
    unsigned int* pTable = isZigZag ? CInverseScanTable::m_nTable0 : CInverseScanTable::m_nTable1;

	for(unsigned int i = 0; i < 64; i++)
	{
		pBlock[i] = nTemp[pTable[i]];
	}
}

uint32 CIPU::GetBusyBit(bool condition) const
{
    return condition ? 0x80000000 : 0x00000000;
}

void CIPU::DisassembleGet(uint32 nAddress)
{
	switch(nAddress)
	{
	case IPU_CMD:
		CLog::GetInstance().Print(LOG_NAME, "IPU_CMD\r\n");
		break;
	case IPU_CTRL:
		CLog::GetInstance().Print(LOG_NAME, "IPU_CTRL\r\n");
		break;
	case IPU_BP:
		CLog::GetInstance().Print(LOG_NAME, "IPU_BP\r\n");
		break;
	case IPU_TOP:
		CLog::GetInstance().Print(LOG_NAME, "IPU_TOP\r\n");
		break;
	}
}

void CIPU::DisassembleSet(uint32 nAddress, uint32 nValue)
{
	switch(nAddress)
	{
	case IPU_CMD + 0x0:
		CLog::GetInstance().Print(LOG_NAME, "IPU_CMD = 0x%0.8X\r\n", nValue);
		break;
	case IPU_CMD + 0x4:
	case IPU_CMD + 0x8:
	case IPU_CMD + 0xC:
		break;

	case IPU_CTRL + 0x0:
		CLog::GetInstance().Print(LOG_NAME, "IPU_CTRL = 0x%0.8X\r\n", nValue);
		break;
	case IPU_CTRL + 0x4:
	case IPU_CTRL + 0x8:
	case IPU_CTRL + 0xC:
		break;

	case IPU_IN_FIFO + 0x0:
	case IPU_IN_FIFO + 0x4:
	case IPU_IN_FIFO + 0x8:
	case IPU_IN_FIFO + 0xC:
		CLog::GetInstance().Print(LOG_NAME, "IPU_IN_FIFO = 0x%0.8X\r\n", nValue);
		break;
	}

}

void CIPU::DisassembleCommand(uint32 nValue)
{
	switch(nValue >> 28)
	{
	case 0:
		CLog::GetInstance().Print(LOG_NAME, "BCLR(bp = %i);\r\n", nValue & 0x7F);
		break;
	case 2:
		CLog::GetInstance().Print(LOG_NAME, "BDEC(mbi = %i, dcr = %i, dt = %i, qsc = %i, fb = %i);\r\n", \
			(nValue >> 27) & 1, \
			(nValue >> 26) & 1, \
			(nValue >> 25) & 1, \
			(nValue >> 16) & 0x1F, \
			nValue & 0x3F);
		break;
	case 3:
		{
			uint32 tbl = (nValue >> 26) & 0x3;
			const char* tblName = NULL;
			switch(tbl)
			{
			case 0:
				//Macroblock Address Increment
				tblName = "MBI";
				break;
			case 1:
				//Macroblock Type
				switch(GetPictureType())
				{
				case 1:
					//I Picture
					tblName = "MB Type (I)";
					break;
				case 2:
					//P Picture
					tblName = "MB Type (P)";
					break;
				case 3:
					//B Picture
					tblName = "MB Type (B)";
					break;
				default:
					assert(0);
					return;
					break;
				}
				break;
			case 2:
				tblName = "Motion Type";
				break;
			case 3:
				tblName = "DM Vector";
				break;
			}
			CLog::GetInstance().Print(LOG_NAME, "VDEC(tbl = %i (%s), bp = %i);\r\n", tbl, tblName, nValue & 0x3F);
		}
		break;
	case 4:
		CLog::GetInstance().Print(LOG_NAME, "FDEC(bp = %i);\r\n", nValue & 0x3F);
		break;
	case 5:
		CLog::GetInstance().Print(LOG_NAME, "SETIQ(iqm = %i, bp = %i);\r\n", (nValue & 0x08000000) != 0 ? 1 : 0, nValue & 0x7F);
		break;
	case 6:
		CLog::GetInstance().Print(LOG_NAME, "SETVQ();\r\n");
		break;
	case 7:
		CLog::GetInstance().Print(LOG_NAME, "CSC(ofm = %i, dte = %i, mbc = %i);\r\n", \
			(nValue >> 27) & 1, \
			(nValue >> 26) & 1, \
			(nValue >>  0) & 0x7FF);
		break;
	case 9:
		CLog::GetInstance().Print(LOG_NAME, "SETTH(th0 = 0x%0.4X, th1 = 0x%0.4X);\r\n", nValue & 0x1FF, (nValue >> 16) & 0x1FF);
		break;
	}
}

/////////////////////////////////////////////
//COutFifoBase class implementation
/////////////////////////////////////////////

CIPU::COutFifoBase::~COutFifoBase()
{

}

/////////////////////////////////////////////
//OUT FIFO class implementation
/////////////////////////////////////////////

CIPU::COUTFIFO::COUTFIFO()
{
	m_pBuffer	= NULL;
	m_nAlloc	= 0;
	m_nSize		= 0;
}

CIPU::COUTFIFO::~COUTFIFO()
{
	DELETEPTR(m_pBuffer);
}

void CIPU::COUTFIFO::SetReceiveHandler(const Dma3ReceiveHandler& handler)
{
    m_receiveHandler = handler;
}

void CIPU::COUTFIFO::Write(void* pData, unsigned int nSize)
{
	RequestGrow(nSize);

	memcpy(m_pBuffer + m_nSize, pData, nSize);
	m_nSize += nSize;
}

void CIPU::COUTFIFO::Flush()
{
	//Write to memory through DMA channel 3
    assert((m_nSize & 0x0F) == 0);
	uint32 nCopied = m_receiveHandler(m_pBuffer, m_nSize / 0x10);
	nCopied *= 0x10;

	assert(m_nSize == nCopied);

	memmove(m_pBuffer, m_pBuffer + nCopied, m_nSize - nCopied);
	m_nSize -= nCopied;
}

void CIPU::COUTFIFO::RequestGrow(unsigned int nSize)
{
	while(m_nAlloc <= (nSize + m_nSize))
	{
		m_nAlloc += GROWSIZE;
		m_pBuffer = (uint8*)realloc(m_pBuffer, m_nAlloc);
	}
}

/////////////////////////////////////////////
//IN FIFO class implementation
/////////////////////////////////////////////

CIPU::CINFIFO::CINFIFO()
{
	m_nSize			= 0;
	m_nBitPosition	= 0;
}

CIPU::CINFIFO::~CINFIFO()
{

}

void CIPU::CINFIFO::Write(void* pData, unsigned int nSize)
{
    boost::mutex::scoped_lock accessLock(m_accessMutex);

	if(nSize + m_nSize > BUFFERSIZE) 
    {
        assert(0);
        return;
    }

    memcpy(m_nBuffer + m_nSize, pData, nSize);
	m_nSize += nSize;

    m_dataNeededCondition.notify_all();
}

uint32 CIPU::CINFIFO::GetBits_LSBF(uint8 nBits)
{
	//Shouldn't be used
	return 0;
}

uint32 CIPU::CINFIFO::GetBits_MSBF(uint8 nBits)
{
	uint32 nValue = PeekBits_MSBF(nBits);
	Advance(nBits);

	return nValue;
}

bool CIPU::CINFIFO::TryPeekBits_LSBF(uint8 nBits, uint32& result)
{
	//Shouldn't be used
	return false;
}

bool CIPU::CINFIFO::TryPeekBits_MSBF(uint8 nBits, uint32& result)
{
    boost::mutex::scoped_lock accessLock(m_accessMutex);

    int bitsAvailable = (m_nSize * 8) - m_nBitPosition;
    int bitsNeeded = nBits;
    assert(bitsAvailable >= 0);
	if(bitsAvailable < bitsNeeded)
	{
		return false;
	}

	unsigned int nBitPosition = m_nBitPosition;
	uint32 nTemp = 0;

	for(unsigned int i = 0; i < nBits; i++)
	{
        nTemp       <<= 1;
		uint8 nByte   = *(m_nBuffer + (nBitPosition / 8));
		uint8 nBit    = (nByte >> (7 - (nBitPosition & 7))) & 1;
		nTemp        |= nBit;

		nBitPosition++;
	}

	result = nTemp;
	return true;
}

void CIPU::CINFIFO::Advance(uint8 nBits)
{
    if(nBits == 0) return;

    boost::mutex::scoped_lock accessLock(m_accessMutex);

    m_nBitPosition += nBits;

	bool advanced = false;

	while(m_nBitPosition >= 128)
	{
		if(m_nSize == 0)
		{
			assert(0);
		}

		if((m_nSize == 0) && (m_nBitPosition != 0))
		{
			//Humm, this seems to happen when the DMA4 has done the transfer
			//but we need more data...
            assert(0);
            m_dataNeededCondition.wait(accessLock);
        }

		//Discard the read bytes
		memmove(m_nBuffer, m_nBuffer + 16, m_nSize - 16);
		m_nSize -= 16;
		m_nBitPosition -= 128;

		advanced = true;
	}

	if(advanced)
	{
		m_dataConsumedCondition.notify_all();
	}
}

void CIPU::CINFIFO::WaitForData()
{
	boost::mutex::scoped_lock accessLock(m_accessMutex);
	boost::xtime xt;
	boost::xtime_get(&xt, boost::TIME_UTC);
	xt.nsec += 10000000;
	m_dataNeededCondition.timed_wait(accessLock, xt);
}

void CIPU::CINFIFO::WaitForSpace()
{
	boost::mutex::scoped_lock accessLock(m_accessMutex);
	boost::xtime xt;
	boost::xtime_get(&xt, boost::TIME_UTC);
	xt.nsec += 10000;
	m_dataConsumedCondition.timed_wait(accessLock, xt);
}

void CIPU::CINFIFO::SeekToByteAlign()
{
	//Shouldn't be used
}

bool CIPU::CINFIFO::IsOnByteBoundary()
{
	//Shouldn't be used
	return false;
}

uint8 CIPU::CINFIFO::GetBitIndex() const
{
    boost::mutex::scoped_lock accessLock(const_cast<CINFIFO*>(this)->m_accessMutex);
	return m_nBitPosition;
}

void CIPU::CINFIFO::SetBitPosition(unsigned int nPosition)
{
    boost::mutex::scoped_lock accessLock(m_accessMutex);
    m_nBitPosition = nPosition;
}

unsigned int CIPU::CINFIFO::GetSize()
{
    boost::mutex::scoped_lock accessLock(m_accessMutex);
    return m_nSize;
}

unsigned int CIPU::CINFIFO::GetAvailableBits()
{
    boost::mutex::scoped_lock accessLock(m_accessMutex);
    return (m_nSize * 8) - m_nBitPosition;
}

void CIPU::CINFIFO::Reset()
{
    boost::mutex::scoped_lock accessLock(m_accessMutex);
    m_nSize = 0;
}

/////////////////////////////////////////////
//IDEC FIFO class implementation
/////////////////////////////////////////////

CIPU::CIDecFifo::CIDecFifo()
{
	Reset();
}

CIPU::CIDecFifo::~CIDecFifo()
{
	
}

void CIPU::CIDecFifo::Reset()
{
	m_nReadPtr = 0;
	m_nWritePtr = 0;
}

void CIPU::CIDecFifo::Write(void* pBuffer, unsigned int nSize)
{
	assert((m_nWritePtr + nSize) <= BUFFERSIZE);
	memcpy(m_nBuffer + m_nWritePtr, pBuffer, nSize);
	m_nWritePtr += nSize;
}

void CIPU::CIDecFifo::Flush()
{
	//Nothing to do
}

bool CIPU::CIDecFifo::TryPeekBits_MSBF(uint8 nLength, uint32& result)
{
	return false;
}

uint32 CIPU::CIDecFifo::GetBits_MSBF(uint8 nLength)
{
	assert(nLength == 8);
	assert(m_nReadPtr < BUFFERSIZE);
	return m_nBuffer[m_nReadPtr++];
}

bool CIPU::CIDecFifo::TryPeekBits_LSBF(uint8 nLength, uint32& result)
{
	return false;
}

uint32 CIPU::CIDecFifo::GetBits_LSBF(uint8 nLength)
{
	throw std::exception();
}

void CIPU::CIDecFifo::SeekToByteAlign()
{
	throw std::exception();
}

bool CIPU::CIDecFifo::IsOnByteBoundary()
{
	throw std::exception();
}

void CIPU::CIDecFifo::Advance(uint8)
{

}

uint8 CIPU::CIDecFifo::GetBitIndex() const
{
	assert(0);
	return 0;
}

/////////////////////////////////////////////
//BCLR command implementation
/////////////////////////////////////////////

CIPU::CBCLRCommand::CBCLRCommand()
: m_IN_FIFO(NULL)
, m_commandCode(0)
{

}

void CIPU::CBCLRCommand::Initialize(CINFIFO* fifo, uint32 commandCode)
{
	m_IN_FIFO = fifo;
	m_commandCode = commandCode;
}

void CIPU::CBCLRCommand::Execute()
{
	m_IN_FIFO->Reset();
	m_IN_FIFO->SetBitPosition(m_commandCode & 0x7F);
}

/////////////////////////////////////////////
//BDEC command implementation
/////////////////////////////////////////////

CIPU::CBDECCommand::CBDECCommand()
: m_IN_FIFO(NULL)
, m_currentBlockIndex(0)
, m_state(STATE_ADVANCE)
, m_codedBlockPattern(0)
, m_mbi(false)
, m_dcr(false)
, m_dt(false)
, m_qsc(0)
, m_fb(0)
{
	m_blocks[0].block = m_yBlock[0];	m_blocks[0].channel = 0;
	m_blocks[1].block = m_yBlock[1];	m_blocks[1].channel = 0;
	m_blocks[2].block = m_yBlock[2];	m_blocks[2].channel = 0;
	m_blocks[3].block = m_yBlock[3];	m_blocks[3].channel = 0;
	m_blocks[4].block = m_cbBlock;		m_blocks[4].channel = 1;
	m_blocks[5].block = m_crBlock;		m_blocks[5].channel = 2;

	memset(&m_context, 0, sizeof(m_context));
}

void CIPU::CBDECCommand::Initialize(CINFIFO* inFifo, COutFifoBase* outFifo, uint32 commandCode, const CONTEXT& context)
{
	m_mbi		= static_cast<uint8>((commandCode >> 27) & 1) != 0;
	m_dcr		= static_cast<uint8>((commandCode >> 26) & 1) != 0;
	m_dt		= static_cast<uint8>((commandCode >> 25) & 1) != 0;
	m_qsc		= static_cast<uint8>((commandCode >> 16) & 0x1F);
	m_fb		= static_cast<uint8>(commandCode & 0x3F);

	m_context = context;

	m_IN_FIFO	= inFifo;
	m_OUT_FIFO	= outFifo;
	m_state		= STATE_ADVANCE;

	m_codedBlockPattern = 0;
	m_currentBlockIndex = 0;
}

void CIPU::CBDECCommand::Execute()
{
	while(1)
	{
		switch(m_state)
		{
		case STATE_ADVANCE:
			{
				m_IN_FIFO->Advance(m_fb);
				m_state = STATE_READCBP;
			}
			break;
		case STATE_READCBP:
			{
				if(!m_mbi)
				{
					//Not an Intra Macroblock, so we need to fetch the pattern code
					m_codedBlockPattern = static_cast<uint8>(CCodedBlockPatternTable::GetInstance()->GetSymbol(m_IN_FIFO));
				}
				else
				{
					m_codedBlockPattern = 0x3F;
				}
				m_state = STATE_RESETDC;
			}
			break;
		case STATE_RESETDC:
			{
				if(m_dcr)
				{
					int16 resetValue = 0;

					//Reset the DC prediction values
					switch(m_context.dcPrecision)
					{
					case 0:
						resetValue = 128;
						break;
					case 1:
						resetValue = 256;
						break;
					case 2:
						resetValue = 512;
						break;
					default:
						resetValue = 0;
						assert(0);
						break;
					}

					m_context.dcPredictor[0] = resetValue;
					m_context.dcPredictor[1] = resetValue;
					m_context.dcPredictor[2] = resetValue;
				}
				m_state = STATE_DECODEBLOCK_BEGIN;
			}
			break;
		case STATE_DECODEBLOCK_BEGIN:
			{
				BLOCKENTRY& blockInfo(m_blocks[m_currentBlockIndex]);
				memset(blockInfo.block, 0, sizeof(int16) * 64);

				if((m_codedBlockPattern & (1 << (5 - m_currentBlockIndex))))
				{
					m_readDctCoeffsCommand.Initialize(m_IN_FIFO, 
						blockInfo.block, blockInfo.channel, 
						m_context.dcPredictor, m_mbi, m_context.isMpeg1);

					m_state = STATE_DECODEBLOCK_READCOEFFS;
				}
				else
				{
					m_state = STATE_DECODEBLOCK_GOTONEXT;
				}
			}
			break;
		case STATE_DECODEBLOCK_READCOEFFS:
			{
				m_readDctCoeffsCommand.Execute();

				BLOCKENTRY& blockInfo(m_blocks[m_currentBlockIndex]);
				int16 blockTemp[0x40];

				DequantiseBlock(blockInfo.block, m_mbi, m_qsc, 
					m_context.isLinearQScale, m_context.dcPrecision, m_context.intraIq, m_context.nonIntraIq);
				InverseScan(blockInfo.block, m_context.isZigZag);

				memcpy(blockTemp, blockInfo.block, sizeof(int16) * 0x40);

				IDCT::CIEEE1180::GetInstance()->Transform(blockTemp, blockInfo.block);

				m_state = STATE_DECODEBLOCK_GOTONEXT;
			}
			break;
		case STATE_DECODEBLOCK_GOTONEXT:
			{
				m_currentBlockIndex++;
				if(m_currentBlockIndex == 6)
				{
					m_state = STATE_DONE;
				}
				else
				{
					m_state = STATE_DECODEBLOCK_BEGIN;
				}
			}
			break;
		case STATE_DONE:
			{
				//Write blocks into out FIFO
				for(unsigned int i = 0; i < 8; i++)
				{
					m_OUT_FIFO->Write(m_blocks[0].block + (i * 8), sizeof(int16) * 0x8);
					m_OUT_FIFO->Write(m_blocks[1].block + (i * 8), sizeof(int16) * 0x8);
				}

				for(unsigned int i = 0; i < 8; i++)
				{
					m_OUT_FIFO->Write(m_blocks[2].block + (i * 8), sizeof(int16) * 0x8);
					m_OUT_FIFO->Write(m_blocks[3].block + (i * 8), sizeof(int16) * 0x8);
				}

				m_OUT_FIFO->Write(m_blocks[4].block, sizeof(int16) * 0x40);
				m_OUT_FIFO->Write(m_blocks[5].block, sizeof(int16) * 0x40);

				m_OUT_FIFO->Flush();
			}
			return;
			break;
		}
	}
}

/////////////////////////////////////////////
//BDEC ReadDct subcommand implementation
/////////////////////////////////////////////

CIPU::CBDECCommand_ReadDct::CBDECCommand_ReadDct()
: m_state(STATE_INIT)
, m_IN_FIFO(NULL)
, m_coeffTable(NULL)
, m_block(NULL)
, m_dcPredictor(NULL)
, m_dcDiff(0)
, m_channelId(0)
, m_mbi(false)
, m_isMpeg1(false)
, m_blockIndex(0)
{

}

void CIPU::CBDECCommand_ReadDct::Initialize(CINFIFO* fifo, int16* block, unsigned int channelId, int16* dcPredictor, bool mbi, bool isMpeg1)
{
	m_state			= STATE_INIT;
	m_IN_FIFO		= fifo;
	m_block			= block;
	m_dcPredictor	= dcPredictor;
	m_channelId		= channelId;
	m_mbi			= mbi;
	m_isMpeg1		= isMpeg1;
	m_coeffTable	= NULL;
	m_blockIndex	= 0;
	m_dcDiff		= 0;

	if(m_mbi && !m_isMpeg1)
	{
		m_coeffTable = CDctCoefficientTable1::GetInstance();
	}
	else
	{
		m_coeffTable = CDctCoefficientTable0::GetInstance();
	}
}

void CIPU::CBDECCommand_ReadDct::Execute()
{
	while(1)
	{
		switch(m_state)
		{
		case STATE_INIT:
			{
#ifdef _DECODE_LOGGING
				CLog::GetInstance().Print(LOG_NAME, "Block = ");
#endif
				if(m_mbi)
				{
					m_readDcDiffCommand.Initialize(m_IN_FIFO, m_channelId, &m_dcDiff);
					m_state = STATE_READDCDIFF;
				}
				else
				{
					m_state = STATE_CHECKEOB;
				}
			}
			break;
		case STATE_READDCDIFF:
			{
				m_readDcDiffCommand.Execute();
				m_block[0] = static_cast<int16>(m_dcPredictor[m_channelId] + m_dcDiff);
				m_dcPredictor[m_channelId] = m_block[0];
#ifdef _DECODE_LOGGING
				CLog::GetInstance().Print(LOG_NAME, "[%d]:%d ", 0, block[0]);
#endif
				m_blockIndex = 1;
				m_state = STATE_CHECKEOB;
			}

			break;
		case STATE_CHECKEOB:
			{
				if((m_blockIndex != 0) && m_coeffTable->IsEndOfBlock(m_IN_FIFO))
				{
					m_state = STATE_SKIPEOB;
				}
				else
				{
					m_state = STATE_READCOEFF;
				}
			}
			break;
		case STATE_READCOEFF:
			{
				MPEG2::RUNLEVELPAIR runLevelPair;
				if(m_blockIndex == 0)
				{
					m_coeffTable->GetRunLevelPairDc(m_IN_FIFO, &runLevelPair, !m_isMpeg1);
				}
				else
				{
					m_coeffTable->GetRunLevelPair(m_IN_FIFO, &runLevelPair, !m_isMpeg1);
				}
				m_blockIndex += runLevelPair.nRun;
			
				if(m_blockIndex < 0x40)
				{
					m_block[m_blockIndex] = static_cast<int16>(runLevelPair.nLevel);
#ifdef _DECODE_LOGGING
					CLog::GetInstance().Print(LOG_NAME, "[%i]:%i ", index, runLevelPair.nLevel);
#endif
				}
				else
				{
					assert(0);
					break;
				}

				m_blockIndex++;
				m_state = STATE_CHECKEOB;
			}
			break;
		case STATE_SKIPEOB:
			m_coeffTable->SkipEndOfBlock(m_IN_FIFO);
			return;
			break;
		}
	}
}

/////////////////////////////////////////////
//BDEC ReadDcDiff subcommand implementation
/////////////////////////////////////////////
CIPU::CBDECCommand_ReadDcDiff::CBDECCommand_ReadDcDiff()
: m_state(STATE_READSIZE)
, m_result(NULL)
, m_IN_FIFO(NULL)
, m_channelId(0)
, m_dcSize(0)
{

}

void CIPU::CBDECCommand_ReadDcDiff::Initialize(CINFIFO* fifo, unsigned int channelId, int16* result)
{
	m_IN_FIFO = fifo;
	m_channelId = channelId;
	m_state = STATE_READSIZE;
	m_dcSize = 0;
	m_result = result;
}

void CIPU::CBDECCommand_ReadDcDiff::Execute()
{
	while(1)
	{
		switch(m_state)
		{
		case STATE_READSIZE:
			{
				switch(m_channelId)
				{
				case 0:
					m_dcSize = static_cast<uint8>(CDcSizeLuminanceTable::GetInstance()->GetSymbol(m_IN_FIFO));
					break;
				case 1:
				case 2:
					m_dcSize = static_cast<uint8>(CDcSizeChrominanceTable::GetInstance()->GetSymbol(m_IN_FIFO));
					break;
				}
				m_state = STATE_READDIFF;
			}
			break;
		case STATE_READDIFF:
			{
				int16 result = 0;
				if(m_dcSize == 0)
				{
					result = 0;
				}
				else
				{
					int16 halfRange = (1 << (m_dcSize - 1));
					result = static_cast<int16>(m_IN_FIFO->GetBits_MSBF(m_dcSize));

					if(result < halfRange)
					{
						result = (result + 1) - (2 * halfRange);
					}
				}
				(*m_result) = result;
				m_state = STATE_DONE;
			}
			break;
		case STATE_DONE:
			return;
			break;
		}
	}
}

/////////////////////////////////////////////
//VDEC command implementation
/////////////////////////////////////////////

CIPU::CVDECCommand::CVDECCommand()
: m_IN_FIFO(NULL)
, m_state(STATE_ADVANCE)
, m_commandCode(0)
, m_result(NULL)
, m_table(NULL)
{

}

void CIPU::CVDECCommand::Initialize(CINFIFO* fifo, uint32 commandCode, uint32 pictureType, uint32* result)
{
	m_IN_FIFO		= fifo;
	m_commandCode	= commandCode;
	m_state			= STATE_ADVANCE;
	m_result		= result;

	uint32 tbl = (commandCode >> 26) & 0x03;
	switch(tbl)
	{
	case 0:
		//Macroblock Address Increment
		m_table = CMacroblockAddressIncrementTable::GetInstance();
		break;
	case 1:
		//Macroblock Type
		switch(pictureType)
		{
		case 1:
			//I Picture
			m_table = CMacroblockTypeITable::GetInstance();
			break;
		case 2:
			//P Picture
			m_table = CMacroblockTypePTable::GetInstance();
			break;
		case 3:
			//B Picture
			m_table = CMacroblockTypeBTable::GetInstance();
			break;
		default:
			assert(0);
			return;
			break;
		}
		break;
	case 2:
		m_table = CMotionCodeTable::GetInstance();
		break;
	case 3:
		m_table = CDmVectorTable::GetInstance();
		break;
	default:
		assert(0);
		return;
		break;
	}
}

void CIPU::CVDECCommand::Execute()
{
	while(1)
	{
		switch(m_state)
		{
		case STATE_ADVANCE:
			{
				m_IN_FIFO->Advance(static_cast<uint8>(m_commandCode & 0x3F));
				m_state = STATE_DECODE;
			}
			break;
		case STATE_DECODE:
			{
				(*m_result) = m_table->GetSymbol(m_IN_FIFO);
				m_state = STATE_DONE;
//				CLog::GetInstance().Print(LOG_NAME, "VDEC (%dth) result: %0.8X.\r\n", currentVDEC++, (*m_result));
			}
			break;
		case STATE_DONE:
			return;
			break;
		}
	}
}

/////////////////////////////////////////////
//FDEC command implementation
/////////////////////////////////////////////

CIPU::CFDECCommand::CFDECCommand()
: m_IN_FIFO(NULL)
, m_state(STATE_ADVANCE)
, m_commandCode(0)
, m_result(NULL)
{

}

void CIPU::CFDECCommand::Initialize(CINFIFO* fifo, uint32 commandCode, uint32* result)
{
	m_IN_FIFO		= fifo;
	m_commandCode	= commandCode;
	m_state			= STATE_ADVANCE;
	m_result		= result;
}

void CIPU::CFDECCommand::Execute()
{
	while(1)
	{
		switch(m_state)
		{
		case STATE_ADVANCE:
			{
				m_IN_FIFO->Advance(static_cast<uint8>(m_commandCode & 0x3F));
				m_state = STATE_DECODE;
			}
			break;
		case STATE_DECODE:
			{
				(*m_result) = m_IN_FIFO->PeekBits_MSBF(32);
				m_state = STATE_DONE;
				//CLog::GetInstance().Print(LOG_NAME, "FDEC result: %0.8X.\r\n", (*m_result));
			}
			break;
		case STATE_DONE:
			return;
			break;
		}
	}
}

/////////////////////////////////////////////
//SETIQ command implementation
/////////////////////////////////////////////

CIPU::CSETIQCommand::CSETIQCommand()
: m_IN_FIFO(NULL)
, m_matrix(NULL)
, m_currentIndex(0)
{

}

void CIPU::CSETIQCommand::Initialize(CINFIFO* fifo, uint8* matrix)
{
	m_IN_FIFO		= fifo;
	m_matrix		= matrix;
	m_currentIndex	= 0;
}

void CIPU::CSETIQCommand::Execute()
{
	while(m_currentIndex != 0x40)
	{
		m_matrix[m_currentIndex] = static_cast<uint8>(m_IN_FIFO->GetBits_MSBF(8));
		m_currentIndex++;
	}
}

/////////////////////////////////////////////
//SETVQ command implementation
/////////////////////////////////////////////

CIPU::CSETVQCommand::CSETVQCommand()
: m_IN_FIFO(NULL)
, m_clut(NULL)
, m_currentIndex(0)
{

}

void CIPU::CSETVQCommand::Initialize(CINFIFO* fifo, uint16* clut)
{
	m_IN_FIFO		= fifo;
	m_clut			= clut;
	m_currentIndex	= 0;
}

void CIPU::CSETVQCommand::Execute()
{
	while(m_currentIndex != 0x10)
	{
		m_clut[m_currentIndex] = static_cast<uint16>(m_IN_FIFO->GetBits_MSBF(16));
		m_currentIndex++;
	}
}

/////////////////////////////////////////////
//CSC command implementation
/////////////////////////////////////////////
CIPU::CCSCCommand::CCSCCommand()
: m_state(STATE_READBLOCKSTART)
, m_OFM(0)
, m_DTE(0)
, m_MBC(0)
, m_IN_FIFO(NULL)
, m_OUT_FIFO(NULL)
, m_currentIndex(0)
{
	GenerateCbCrMap();
}

void CIPU::CCSCCommand::Initialize(CINFIFO* input, COUTFIFO* output, uint32 commandCode)
{
	m_state = STATE_READBLOCKSTART;

	m_IN_FIFO = input;
	m_OUT_FIFO = output;

	m_OFM = static_cast<uint8> ((commandCode >> 27) & 1);
	m_DTE = static_cast<uint8> ((commandCode >> 26) & 1);
	m_MBC = static_cast<uint16>((commandCode >>  0) & 0x7FF);

	m_currentIndex = 0;
}

void CIPU::CCSCCommand::Execute()
{
	while(1)
	{
		switch(m_state)
		{
		case STATE_READBLOCKSTART:
			{
				if(m_MBC == 0)
				{
					m_state = STATE_DONE;
				}
				else
				{
					m_state = STATE_READBLOCK;
					m_currentIndex = 0;
				}
			}
			break;
		case STATE_READBLOCK:
			{
				if(m_currentIndex == BLOCK_SIZE)
				{
					m_state = STATE_CONVERTBLOCK;
				}
				else
				{
					m_block[m_currentIndex] = static_cast<uint8>(m_IN_FIFO->GetBits_MSBF(8));
					m_currentIndex++;
				}
			}
			break;
		case STATE_CONVERTBLOCK:
			{
				uint32 nPixel[0x100];

				uint8* pY = m_block;
				uint8* nBlockCb = m_block + 0x100;
				uint8* nBlockCr = m_block + 0x140;

				uint32* pPixel = nPixel;
				unsigned int* pCbCrMap = m_nCbCrMap;

				for(unsigned int i = 0; i < 16; i++)
				{
					for(unsigned int j = 0; j < 16; j++)
					{
						float nY  = pY[j];
						float nCb = nBlockCb[pCbCrMap[j]];
						float nCr = nBlockCr[pCbCrMap[j]];

						//nY = nBlockCb[pCbCrMap[j]];
						//nCb = 128;
						//nCr = 128;

						float nR = nY								+ 1.402f	* (nCr - 128);
						float nG = nY - 0.34414f	* (nCb - 128)	- 0.71414f	* (nCr - 128);
						float nB = nY + 1.772f		* (nCb - 128);

						if(nR < 0) { nR = 0; } if(nR > 255) { nR = 255; }
						if(nG < 0) { nG = 0; } if(nG > 255) { nG = 255; }
						if(nB < 0) { nB = 0; } if(nB > 255) { nB = 255; }

						pPixel[j] = (((uint8)nB) << 16) | (((uint8)nG) << 8) | (((uint8)nR) << 0);
					}

					pY			+= 0x10;
					pCbCrMap	+= 0x10;
					pPixel		+= 0x10;
				}

				m_OUT_FIFO->Write(nPixel, sizeof(uint32) * 0x100);
				m_OUT_FIFO->Flush();

				m_MBC--;
				m_state = STATE_READBLOCKSTART;
			}
			break;
		case STATE_DONE:
			return;
			break;
		}
	}
}

void CIPU::CCSCCommand::GenerateCbCrMap()
{
	unsigned int* pCbCrMap = m_nCbCrMap;
	for(unsigned int i = 0; i < 0x40; i += 0x8)
	{
		for(unsigned int j = 0; j < 0x10; j += 2)
		{
			pCbCrMap[j + 0x00] = (j / 2) + i;
			pCbCrMap[j + 0x01] = (j / 2) + i;

			pCbCrMap[j + 0x10] = (j / 2) + i;
			pCbCrMap[j + 0x11] = (j / 2) + i;
		}

		pCbCrMap += 0x20;
	}
}

/////////////////////////////////////////////
//SETTH command implementation
/////////////////////////////////////////////

CIPU::CSETTHCommand::CSETTHCommand()
: m_commandCode(0)
, m_TH0(NULL)
, m_TH1(NULL)
{

}

void CIPU::CSETTHCommand::Initialize(uint32 commandCode, uint16* TH0, uint16* TH1)
{
	m_commandCode = commandCode;
	m_TH0 = TH0;
	m_TH1 = TH1;
}

void CIPU::CSETTHCommand::Execute()
{
	(*m_TH0) = static_cast<uint16>((m_commandCode >>  0) & 0x1FF);
	(*m_TH1) = static_cast<uint16>((m_commandCode >> 16) & 0x1FF);
}
