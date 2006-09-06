#include <stdio.h>
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
#include "PS2VM.h"
#include "PtrMacro.h"

using namespace IPU;
using namespace MPEG2;
using namespace Framework;

uint8			CIPU::m_nIntraIQ[0x40];
uint8			CIPU::m_nNonIntraIQ[0x40];
uint16			CIPU::m_nVQCLUT[0x10];
uint16			CIPU::m_nTH0;
uint16			CIPU::m_nTH1;
int16			CIPU::m_nDcPredictor[3];
uint32			CIPU::m_IPU_CMD[2];
uint32			CIPU::m_IPU_CTRL = 0;
uint32			CIPU::m_nPendingCommand = 0;
unsigned int	CIPU::m_nCbCrMap[0x100];

CIPU::COUTFIFO	CIPU::m_OUT_FIFO;
CIPU::CINFIFO	CIPU::m_IN_FIFO;

void CIPU::Reset()
{
	m_IPU_CTRL			= 0;
	m_IPU_CMD[0]		= 0;
	m_IPU_CMD[1]		= 0;
	m_nPendingCommand	= 0;

	GenerateCbCrMap();
}

uint32 CIPU::GetRegister(uint32 nAddress)
{
	DisassembleGet(nAddress);

	switch(nAddress)
	{
	case IPU_CMD + 0x0:
		return m_IPU_CMD[0];
		break;
	case IPU_CMD + 0x4:
		return m_IPU_CMD[1];
		break;
	case IPU_CMD + 0x8:
	case IPU_CMD + 0xC:
		break;

	case IPU_CTRL + 0x0:
		return m_IPU_CTRL;
		break;
	case IPU_CTRL + 0x4:
	case IPU_CTRL + 0x8:
	case IPU_CTRL + 0xC:
		break;

	case IPU_BP + 0x0:
		return ((m_IN_FIFO.GetSize() / 0x10) << 8) | (m_IN_FIFO.GetBitPosition());
		break;

	case IPU_BP + 0x4:
	case IPU_BP + 0x8:
	case IPU_BP + 0xC:
		break;

	case IPU_TOP + 0x0:
		return m_IN_FIFO.PeekBits_MSBF(32);
		break;
	case IPU_TOP + 0x4:
	case IPU_TOP + 0x8:
	case IPU_TOP + 0xC:
		break;

	default:
		printf("IPU: Reading an unhandled register (0x%0.8X).\r\n", nAddress);
		break;
	}

	return 0;
}

void CIPU::SetRegister(uint32 nAddress, uint32 nValue)
{
	DisassembleSet(nAddress, nValue);

	switch(nAddress)
	{
	case IPU_CMD + 0x0:
		ExecuteCommand(nValue);
		break;
	case IPU_CMD + 0x4:
	case IPU_CMD + 0x8:
	case IPU_CMD + 0xC:
		break;

	case IPU_CTRL + 0x0:
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
		printf("IPU: Writing 0x%0.8X to an unhandled register (0x%0.8X).\r\n", nValue, nAddress);
		break;
	}
}

void CIPU::ExecuteCommand(uint32 nValue)
{
	unsigned int nCmd;

	nCmd = (nValue >> 28);

	//Clear BUSY states
	m_IPU_CTRL		&= ~0x80000000;
	m_IPU_CMD[1]	&= ~0x80000000;

	if((nCmd == 2) || (nCmd == 3) || (nCmd == 4))
	{
		if(IsExecutionRisky(nCmd))
		{
			//Command will wait because we're probably gonna be out of data
			//even though this scheme will probably fail at the end...
			m_IPU_CTRL			|= 0x80000000;
			m_IPU_CMD[1]		|= 0x80000000;
			m_nPendingCommand	= nValue;
			return;
		}
	}

	switch(nCmd)
	{
	case 0:
		//BCLR
		m_IN_FIFO.Reset();
		m_IN_FIFO.SetBitPosition(nValue & 0x7F);
		break;
	case 1:
		//IDEC
		DecodeIntra( \
			static_cast<uint8>((nValue >> 27) & 1), \
			static_cast<uint8>((nValue >> 26) & 1), \
			static_cast<uint8>((nValue >> 25) & 1), \
			static_cast<uint8>((nValue >> 24) & 1), \
			static_cast<uint8>((nValue >> 16) & 0x1F), \
			static_cast<uint8>((nValue >>  0) & 0x3F));
		break;
	case 2:
		//BDEC
		DecodeBlock( \
			(uint8)((nValue >> 27) & 1), \
			(uint8)((nValue >> 26) & 1), \
			(uint8)((nValue >> 25) & 1), \
			(uint8)((nValue >> 16) & 0x1F), \
			(uint8)(nValue & 0x3F));
		break;
	case 3:
		//VDEC
		VariableLengthDecode((uint8)((nValue >> 26) & 0x03), (uint8)(nValue & 0x3F));
		break;
	case 4:
		//FDEC
		FixedLengthDecode((uint8)(nValue & 0x3F));
		break;
	case 5:
		//SETIQ
		if(nValue & 0x08000000)
		{
			LoadIQMatrix(m_nNonIntraIQ);
		}
		else
		{
			LoadIQMatrix(m_nIntraIQ);
		}
		break;
	case 6:
		//SETVQ
		LoadVQCLUT();
		break;
	case 7:
		//CSC
		ColorSpaceConversion( \
			(uint8)((nValue >> 27) & 1), \
			(uint8)((nValue >> 26) & 1), \
			(uint16)((nValue >>  0) & 0x7FF));
		break;
	case 9:
		//SETTH
		SetThresholdValues(nValue);
		break;
	default:
		printf("IPU: Unhandled command execution requested (%i).\r\n", nValue >> 28);
		break;
	}

	DisassembleCommand(nValue);
}

uint32 CIPU::ReceiveDMA(uint32 nAddress, uint32 nQWC, bool nTagIncluded)
{
	uint32 nSize;

	assert(nTagIncluded == false);

	nSize = min(nQWC * 0x10, CINFIFO::BUFFERSIZE - m_IN_FIFO.GetSize());

	if(nSize == 0) return 0;

	m_IN_FIFO.Write(CPS2VM::m_pRAM + nAddress, nSize);

	return nSize / 0x10;
}

void CIPU::DMASliceDoneCallback()
{
	uint32 nCommand;

	//Execute pending commands
	if(m_nPendingCommand != 0)
	{
		nCommand = m_nPendingCommand;
		m_nPendingCommand = 0;
		ExecuteCommand(nCommand);
	}
}

void CIPU::DecodeIntra(uint8 nOFM, uint8 nDTE, uint8 nSGN, uint8 nDTD, uint8 nQSC, uint8 nFB)
{
	bool nResetDc;

	m_IN_FIFO.SkipBits(nFB);

	nResetDc = true;

	while(1)
	{
		uint32 nMBType;
		uint8 nDCTType;

		nMBType = CMacroblockTypeITable::GetInstance()->Decode(&m_IN_FIFO) >> 16;

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

		DecodeBlock(1, nResetDc, nDCTType, nQSC, 0);
		nResetDc = false;
	}
}

void CIPU::DecodeBlock(uint8 nMBI, uint8 nDCR, uint8 nDT, uint8 nQSC, uint8 nFB)
{
	struct BLOCKENTRY
	{
		int16*			pBlock;
		unsigned int	nChannel;
	};

	unsigned int i;
	int16 nResetValue;
	uint8 nCodedBlockPattern;

	int16 nYBlock[4][64];
	int16 nCbBlock[64];
	int16 nCrBlock[64];
	int16 nTemp[64];

	BLOCKENTRY Block[6] = 
	{
		{ nYBlock[0],	0 },
		{ nYBlock[1],	0 },
		{ nYBlock[2],	0 },
		{ nYBlock[3],	0 },
		{ nCbBlock,		1 },
		{ nCrBlock,		2 },
	};

	m_IN_FIFO.SkipBits(nFB);

	if(nMBI == 0)
	{
		//Not an Intra Macroblock, so we need to fetch the pattern code
		nCodedBlockPattern = (uint8)CCodedBlockPatternTable::GetInstance()->Decode(&m_IN_FIFO);
	}
	else
	{
		nCodedBlockPattern = 0x3F;
	}

	if(nDCR == 1)
	{
		//Reset the DC prediction values
		switch(GetDcPrecision())
		{
		case 0:
			nResetValue = 128;
			break;
		case 1:
			nResetValue = 256;
			break;
		case 2:
			nResetValue = 512;
			break;
		default:
			nResetValue = 0;
			assert(0);
			break;
		}

		m_nDcPredictor[0] = nResetValue;
		m_nDcPredictor[1] = nResetValue;
		m_nDcPredictor[2] = nResetValue;
	}

	for(i = 0; i < 6; i++)
	{
		memset(Block[i].pBlock, 0, sizeof(int16) * 64);

		if((nCodedBlockPattern & (1 << (5 - i))))
		{
			DecodeDctCoefficients(Block[i].nChannel, Block[i].pBlock, nMBI);
			DequantiseBlock(Block[i].pBlock, nMBI, nQSC);
			InverseScan(Block[i].pBlock);

			memcpy(nTemp, Block[i].pBlock, sizeof(int16) * 0x40);

			IDCT::CIEEE1180::GetInstance()->Transform(nTemp, Block[i].pBlock);
			//IDCT::CTrivialC::GetInstance()->Transform(nTemp, Block[i].pBlock);

			//SaturateIDCT(Block[i].pBlock);

			//DumpBlock(Block[i].pBlock);
		}
	}

	//Write blocks into out FIFO
	for(i = 0; i < 8; i++)
	{
		m_OUT_FIFO.Write(Block[0].pBlock + (i * 8), sizeof(int16) * 0x8);
		m_OUT_FIFO.Write(Block[1].pBlock + (i * 8), sizeof(int16) * 0x8);
	}

	for(i = 0; i < 8; i++)
	{
		m_OUT_FIFO.Write(Block[2].pBlock + (i * 8), sizeof(int16) * 0x8);
		m_OUT_FIFO.Write(Block[3].pBlock + (i * 8), sizeof(int16) * 0x8);
	}

	m_OUT_FIFO.Write(Block[4].pBlock, sizeof(int16) * 0x40);
	m_OUT_FIFO.Write(Block[5].pBlock, sizeof(int16) * 0x40);

	m_OUT_FIFO.Flush();

/*
	if(m_IN_FIFO.PeekBits_MSBF(23) == 0)
	{
		CPS2VM::m_EE.ToggleBreakpoint(CPS2VM::m_EE.m_State.nPC);
	}
*/
}

void CIPU::VariableLengthDecode(uint8 nTBL, uint8 nFB)
{
	CVLCTable* pTable;

	switch(nTBL)
	{
	case 0:
		//Macroblock Address Increment
		pTable = CMacroblockAddressIncrementTable::GetInstance();
		break;
	case 1:
		//Macroblock Type
		switch(GetPictureType())
		{
		case 1:
			//I Picture
			pTable = CMacroblockTypeITable::GetInstance();
			break;
		case 2:
			//P Picture
			pTable = CMacroblockTypePTable::GetInstance();
			break;
		case 3:
			//B Picture
			pTable = CMacroblockTypeBTable::GetInstance();
			break;
		default:
			assert(0);
			return;
			break;
		}
		break;
	case 2:
		pTable = CMotionCodeTable::GetInstance();
		break;
	case 3:
		pTable = CDmVectorTable::GetInstance();
		break;
	default:
		assert(0);
		return;
		break;
	}

	m_IN_FIFO.SkipBits(nFB);
	m_IPU_CMD[0] = pTable->Decode(&m_IN_FIFO);
}

void CIPU::FixedLengthDecode(uint8 nFB)
{
	/*
	//Clear error code
	m_IPU_CTRL		&= ~0x80000000;
	m_IPU_CMD[1]	&= ~0x80000000;

	if(m_IN_FIFO.GetSize() == 0)
	{
		m_IPU_CTRL			|= 0x80000000;
		m_IPU_CMD[1]		|= 0x80000000;
		m_nPendingCommand	 = 0x40000000 | nFB;
		return;
	}
*/
	m_IN_FIFO.SkipBits(nFB);
	m_IPU_CMD[0] = m_IN_FIFO.PeekBits_MSBF(32);
}

void CIPU::LoadIQMatrix(uint8* pMatrix)
{
	unsigned int i;

	for(i = 0; i < 0x40; i++)
	{
		pMatrix[i] = (uint8)m_IN_FIFO.GetBits_MSBF(8);
	}
}

void CIPU::LoadVQCLUT()
{
	unsigned int i;

	for(i = 0; i < 0x10; i++)
	{
		m_nVQCLUT[i] = (uint16)m_IN_FIFO.GetBits_MSBF(16);
	}
}

void CIPU::ColorSpaceConversion(uint8 nOFM, uint8 nDTE, uint16 nMBC)
{
	unsigned int* pCbCrMap;
	uint8* pY;
	uint32* pPixel;
	double nY, nCb, nCr, nR, nG, nB;
	uint8 nBlockY[0x100];
	uint32 nPixel[0x100];
	uint8 nBlockCb[0x40];
	uint8 nBlockCr[0x40];
	unsigned int i, j;

	while(nMBC != 0)
	{
		//Get Y data (blocks 0 and 1)
		for(i = 0; i < 0x80; i += 0x10)
		{
			for(j = 0; j < 8; j++)
			{
				nBlockY[i + j + 0x00] = (uint8)m_IN_FIFO.GetBits_MSBF(8);
			}
			for(j = 0; j < 8; j++)
			{
				nBlockY[i + j + 0x08] = (uint8)m_IN_FIFO.GetBits_MSBF(8);
			}
		}

		//Get Y data (blocks 2 and 3)
		for(i = 0; i < 0x80; i += 0x10)
		{
			for(j = 0; j < 8; j++)
			{
				nBlockY[i + j + 0x80] = (uint8)m_IN_FIFO.GetBits_MSBF(8);
			}
			for(j = 0; j < 8; j++)
			{
				nBlockY[i + j + 0x88] = (uint8)m_IN_FIFO.GetBits_MSBF(8);
			}
		}

		//Get Cb data
		for(i = 0; i < 0x40; i++)
		{
			nBlockCb[i] = (uint8)m_IN_FIFO.GetBits_MSBF(8);
		}

		//Get Cr data
		for(i = 0; i < 0x40; i++)
		{
			nBlockCr[i] = (uint8)m_IN_FIFO.GetBits_MSBF(8);
		}

		pY			= nBlockY;
		pPixel		= nPixel;
		pCbCrMap	= m_nCbCrMap;

		for(i = 0; i < 16; i++)
		{
			for(j = 0; j < 16; j++)
			{
				nY	= pY[j];
				nCb	= nBlockCb[pCbCrMap[j]];
				nCr = nBlockCr[pCbCrMap[j]];

				//nY = nBlockCb[pCbCrMap[j]];
				//nCb = 128;
				//nCr = 128;

				nR = nY								+ 1.402		* (nCr - 128);
				nG = nY - 0.34414	* (nCb - 128)	- 0.71414	* (nCr - 128);
				nB = nY + 1.772		* (nCb - 128);

				if(nR < 0) { nR = 0; } if(nR > 255) { nR = 255; }
				if(nG < 0) { nG = 0; } if(nG > 255) { nG = 255; }
				if(nB < 0) { nB = 0; } if(nB > 255) { nB = 255; }

				pPixel[j] = (((uint8)nB) << 16) | (((uint8)nG) << 8) | (((uint8)nR) << 0);
			}

			pY			+= 0x10;
			pCbCrMap	+= 0x10;
			pPixel		+= 0x10;
		}

		m_OUT_FIFO.Write(nPixel, sizeof(uint32) * 0x100);
		m_OUT_FIFO.Flush();

		nMBC--;
	}
/*
	if(m_IN_FIFO.GetSize() != 0)
	{
		assert(0);
	}
*/
}

void CIPU::SetThresholdValues(uint32 nValue)
{
	m_nTH0 = (uint16)(nValue >>  0) & 0x1FF;
	m_nTH1 = (uint16)(nValue >> 16) & 0x1FF;
}

bool CIPU::IsExecutionRisky(unsigned int nCmd)
{
	uint32 nCHCR;

	nCHCR = CDMAC::GetRegister(CDMAC::D4_CHCR);

	if((nCHCR & CDMAC::CHCR_STR) == 0) return true;
	if(nCmd == 2)
	{
		if(CDMAC::IsEndTagId(nCHCR))
		{
			if(CDMAC::GetRegister(CDMAC::D4_QWC) < 0x10) return true;
		}
	}
	else
	{
		if(m_IN_FIFO.GetSize() <= 0x80) return true;
	}
	return false;
}

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

void CIPU::DecodeDctCoefficients(unsigned int nChannel, int16* pBlock, uint8 nMBI)
{
	unsigned int nIndex;

	RUNLEVELPAIR RunLevelPair;
	CDctCoefficientTable* pDctCoeffTable;

	if(nMBI && !GetIsMPEG1CoeffVLCTable())
	{
		pDctCoeffTable = CDctCoefficientTable1::GetInstance();
	}
	else
	{
		pDctCoeffTable = CDctCoefficientTable0::GetInstance();
	}

	//If it's an intra macroblock
	if(nMBI == 1)
	{
		pBlock[0] = (int16)(m_nDcPredictor[nChannel] + GetDcDifferential(nChannel));
		m_nDcPredictor[nChannel] = pBlock[0];

		nIndex = 1;
	}
	else
	{
		pDctCoeffTable->GetRunLevelPairDc(&m_IN_FIFO, &RunLevelPair, GetIsMPEG2());

		nIndex = 0;

		nIndex += RunLevelPair.nRun;
		pBlock[nIndex] = (int16)RunLevelPair.nLevel;

		nIndex++;
	}

	while(!pDctCoeffTable->IsEndOfBlock(&m_IN_FIFO))
	{
		pDctCoeffTable->GetRunLevelPair(&m_IN_FIFO, &RunLevelPair, GetIsMPEG2());

		nIndex += RunLevelPair.nRun;
		
		if(nIndex < 0x40)
		{
			pBlock[nIndex] = (int16)RunLevelPair.nLevel;
		}
		else
		{
			assert(0);
			break;
		}

		nIndex++;
	}

	//Done decoding
	pDctCoeffTable->SkipEndOfBlock(&m_IN_FIFO);
}

void CIPU::DequantiseBlock(int16* pBlock, uint8 nMBI, uint8 nQSC)
{
	int16 nQuantScale, nIntraDcMult, nSign, nSum;
	unsigned int i;

	if(GetIsLinearQScale())
	{
		nQuantScale = (int16)CQuantiserScaleTable::m_nTable0[nQSC];
	}
	else
	{
		nQuantScale = (int16)CQuantiserScaleTable::m_nTable1[nQSC];
	}

	if(nMBI == 1)
	{
		switch(GetDcPrecision())
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
		
		for(i = 1; i < 64; i++)
		{
			if(pBlock[i] == 0)
			{
				nSign = 0;
			}
			else
			{
				nSign = (pBlock[i] > 0) ? 0x0001 : 0xFFFF;
			}

			pBlock[i] = (pBlock[i] * (int16)m_nIntraIQ[i] * nQuantScale * 2) / 32;

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
		for(i = 0; i < 64; i++)
		{
			if(pBlock[i] == 0)
			{
				nSign = 0;
			}
			else
			{
				nSign = (pBlock[i] > 0) ? 0x0001 : 0xFFFF;
			}

			pBlock[i] = (((pBlock[i] * 2) + nSign) * (int16)m_nNonIntraIQ[i] * nQuantScale) / 32;

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
	nSum = 0;

	for(i = 0; i < 64; i++)
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

void CIPU::InverseScan(int16* pBlock)
{
	int16 nTemp[0x40];
	unsigned int* pTable;
	unsigned int i;

	memcpy(nTemp, pBlock, sizeof(int16) * 0x40);

	if(GetIsZigZagScan())
	{
		pTable = CInverseScanTable::m_nTable0;
	}
	else
	{
		pTable = CInverseScanTable::m_nTable1;
	}


	for(i = 0; i < 64; i++)
	{
		pBlock[i] = nTemp[pTable[i]];
	}
}

int16 CIPU::GetDcDifferential(unsigned int nChannel)
{
	uint8 nDcSize;
	int16 nDcDiff, nHalfRange;

	switch(nChannel)
	{
	case 0:
		nDcSize = (uint8)CDcSizeLuminanceTable::GetInstance()->Decode(&m_IN_FIFO);
		break;
	case 1:
	case 2:
		nDcSize = (uint8)CDcSizeChrominanceTable::GetInstance()->Decode(&m_IN_FIFO);
		break;
	}

	if(nDcSize == 0)
	{
		nDcDiff = 0;
	}
	else
	{
		nHalfRange = (1 << (nDcSize - 1));
		nDcDiff = (int16)m_IN_FIFO.GetBits_MSBF(nDcSize);

		if(nDcDiff < nHalfRange)
		{
			nDcDiff = (nDcDiff + 1) - (2 * nHalfRange);
		}
	}

	return nDcDiff;
}

void CIPU::GenerateCbCrMap()
{
	unsigned int* pCbCrMap;
	unsigned int i, j;

	pCbCrMap = m_nCbCrMap;
	for(i = 0; i < 0x40; i += 0x8)
	{
		for(j = 0; j < 0x10; j += 2)
		{
			pCbCrMap[j + 0x00] = (j / 2) + i;
			pCbCrMap[j + 0x01] = (j / 2) + i;

			pCbCrMap[j + 0x10] = (j / 2) + i;
			pCbCrMap[j + 0x11] = (j / 2) + i;
		}

		pCbCrMap += 0x20;
	}
}

void CIPU::DisassembleGet(uint32 nAddress)
{
	if(!CPS2VM::m_Logging.GetIPULoggingStatus()) return;

	switch(nAddress)
	{
	case IPU_CMD:
		printf("IPU: = IPU_CMD (PC: 0x%0.8X).\r\n", CPS2VM::m_EE.m_State.nPC);
		break;
	case IPU_CTRL:
		printf("IPU: = IPU_CTRL (PC: 0x%0.8X).\r\n", CPS2VM::m_EE.m_State.nPC);
		break;
	case IPU_BP:
		printf("IPU: = IPU_BP (PC: 0x%0.8X).\r\n", CPS2VM::m_EE.m_State.nPC);
		break;
	case IPU_TOP:
		printf("IPU: = IPU_TOP (PC: 0x%0.8X).\r\n", CPS2VM::m_EE.m_State.nPC);
		break;
	}
}

void CIPU::DisassembleSet(uint32 nAddress, uint32 nValue)
{
	if(!CPS2VM::m_Logging.GetIPULoggingStatus()) return;

	switch(nAddress)
	{
	case IPU_CMD + 0x0:
		printf("IPU: IPU_CMD = 0x%0.8X (PC: 0x%0.8X).\r\n", nValue, CPS2VM::m_EE.m_State.nPC);
		break;
	case IPU_CMD + 0x4:
	case IPU_CMD + 0x8:
	case IPU_CMD + 0xC:
		break;

	case IPU_CTRL + 0x0:
		printf("IPU: IPU_CTRL = 0x%0.8X (PC: 0x%0.8X).\r\n", nValue, CPS2VM::m_EE.m_State.nPC);
		break;
	case IPU_CTRL + 0x4:
	case IPU_CTRL + 0x8:
	case IPU_CTRL + 0xC:
		break;

	case IPU_IN_FIFO + 0x0:
	case IPU_IN_FIFO + 0x4:
	case IPU_IN_FIFO + 0x8:
	case IPU_IN_FIFO + 0xC:
		printf("IPU: IPU_IN_FIFO = 0x%0.8X (PC: 0x%0.8X).\r\n", nValue, CPS2VM::m_EE.m_State.nPC);
		break;
	}

}

void CIPU::DisassembleCommand(uint32 nValue)
{
	if(!CPS2VM::m_Logging.GetIPULoggingStatus()) return;

	switch(nValue >> 28)
	{
	case 0:
		printf("IPU: BCLR(bp = %i);\r\n", nValue & 0x7F);
		break;
	case 2:
		printf("IPU: BDEC(mbi = %i, dcr = %i, dt = %i, qsc = %i, fb = %i);\r\n", \
			(nValue >> 27) & 1, \
			(nValue >> 26) & 1, \
			(nValue >> 25) & 1, \
			(nValue >> 16) & 0x1F, \
			nValue & 0x3F);
		break;
	case 3:
		printf("IPU: VDEC(tbl = %i, bp = %i);\r\n", (nValue >> 26) & 0x3, nValue & 0x3F);
		break;
	case 4:
		printf("IPU: FDEC(bp = %i);\r\n", nValue & 0x3F);
		break;
	case 5:
		printf("IPU: SETIQ(iqm = %i, bp = %i);\r\n", (nValue & 0x08000000) != 0 ? 1 : 0, nValue & 0x7F);
		break;
	case 6:
		printf("IPU: SETVQ();\r\n");
		break;
	case 7:
		printf("IPU: CSC(ofm = %i, dte = %i, mbc = %i);\r\n", \
			(nValue >> 27) & 1, \
			(nValue >> 26) & 1, \
			(nValue >>  0) & 0x7FF);
		break;
	case 9:
		printf("IPU: SETTH(th0 = 0x%0.4X, th1 = 0x%0.4X);\r\n", nValue & 0x1FF, (nValue >> 16) & 0x1FF);
		break;
	}
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

void CIPU::COUTFIFO::Write(void* pData, unsigned int nSize)
{
	RequestGrow(nSize);

	memcpy(m_pBuffer + m_nSize, pData, nSize);
	m_nSize += nSize;
}

void CIPU::COUTFIFO::Flush()
{
	uint32 nCopied;

	//Write to memory through DMA channel 3
	nCopied = CDMAC::ResumeDMA3(m_pBuffer, m_nSize / 0x10);
	nCopied *= 0x10;

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
	if(nSize + m_nSize > BUFFERSIZE) return;

	memcpy(m_nBuffer + m_nSize, pData, nSize);
	m_nSize += nSize;
}

uint32 CIPU::CINFIFO::GetBits_LSBF(uint8 nBits)
{
	//Shouldn't be used
	return 0;
}

uint32 CIPU::CINFIFO::GetBits_MSBF(uint8 nBits)
{
	uint32 nValue;

	nValue = PeekBits_MSBF(nBits);
	SkipBits(nBits);

	return nValue;
}

uint32 CIPU::CINFIFO::PeekBits_LSBF(uint8 nBits)
{
	//Shouldn't be used
	return 0;
}

uint32 CIPU::CINFIFO::PeekBits_MSBF(uint8 nBits)
{
	unsigned int i, nBitPosition;
	uint8 nByte, nBit;
	uint32 nTemp;

	if(m_nSize < 16)
	{
		assert(0);
		return 0;
	}

	nBitPosition = m_nBitPosition;
	nTemp = 0;

	for(i = 0; i < nBits; i++)
	{
		nTemp	<<= 1;
		nByte	  = *(m_nBuffer + (nBitPosition / 8));
		nBit	  = (nByte >> (7 - (nBitPosition & 7))) & 1;
		nTemp	 |= nBit;

		nBitPosition++;
	}

	return nTemp;
}

void CIPU::CINFIFO::SkipBits(uint8 nBits)
{
	if(nBits == 0) return;

	m_nBitPosition += nBits;

	while(m_nBitPosition >= 128)
	{
		if(m_nSize == 0)
		{
			assert(0);
		}
		//if((m_nSize == 0) && (m_nBitPosition != 0))
		//{
			//Humm, this seems to happen when the DMA4 has done the transfer
			//but we need more data...
		//	break;
		//}

		//Discard the read bytes
		memmove(m_nBuffer, m_nBuffer + 16, m_nSize - 16);
		m_nSize -= 16;
		m_nBitPosition -= 128;

		//Get some more data from the DMA
		CDMAC::ResumeDMA4();
	}
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

unsigned int CIPU::CINFIFO::GetBitPosition()
{
	return m_nBitPosition;
}

void CIPU::CINFIFO::SetBitPosition(unsigned int nPosition)
{
	m_nBitPosition = nPosition;
}

unsigned int CIPU::CINFIFO::GetSize()
{
	return m_nSize;
}

void CIPU::CINFIFO::Reset()
{
	m_nSize = 0;
}
