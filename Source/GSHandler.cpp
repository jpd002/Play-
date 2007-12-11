#include <stdio.h>
#include <string.h>
#include <functional>
#include "GSHandler.h"
#include "PS2VM.h"
#include "INTC.h"
#include "PtrMacro.h"
#include "Log.h"

#define R_REG(a, v, r)					\
	if((a) & 0x4)						\
	{									\
		v = (uint32)(r >> 32);			\
	}									\
	else								\
	{									\
		v = (uint32)(r & 0xFFFFFFFF);	\
	}

#define W_REG(a, v, r)				\
	if((a) & 0x4)					\
	{								\
		(r) &= 0x00000000FFFFFFFFULL;	\
		(r) |= (uint64)(v) << 32;	\
	}								\
	else							\
	{								\
		(r) &= 0xFFFFFFFF00000000ULL;	\
		(r) |= (v);					\
	}

using namespace Framework;
using namespace std::tr1;
using namespace boost;

int CGSHandler::STORAGEPSMCT32::m_nBlockSwizzleTable[4][8] =
{
	{	0,	1,	4,	5,	16,	17,	20,	21	},
	{	2,	3,	6,	7,	18,	19,	22,	23	},
	{	8,	9,	12,	13,	24,	25,	28,	29	},
	{	10,	11,	14,	15,	26,	27,	30,	31	},
};

int CGSHandler::STORAGEPSMCT32::m_nColumnSwizzleTable[2][8] =
{
	{	0,	1,	4,	5,	8,	9,	12,	13,	},
	{	2,	3,	6,	7,	10,	11,	14,	15,	},
};

int CGSHandler::STORAGEPSMCT16::m_nBlockSwizzleTable[8][4] =
{
	{	0,	2,	8,	10,	},
	{	1,	3,	9,	11,	},
	{	4,	6,	12,	14,	},
	{	5,	7,	13,	15,	},
	{	16,	18,	24,	26,	},
	{	17,	19,	25,	27,	},
	{	20,	22,	28,	30,	},
	{	21,	23,	29,	31,	},
};

int CGSHandler::STORAGEPSMCT16::m_nColumnSwizzleTable[2][16] =
{
	{	0,	2,	8,	10,	16,	18,	24,	26,	1,	3,	9,	11,	17,	19,	25,	27,	},
	{	4,	6,	12,	14,	20,	22,	28,	30,	5,	7,	13,	15,	21,	23,	29,	31,	},
};

int CGSHandler::STORAGEPSMCT16S::m_nBlockSwizzleTable[8][4] =
{
	{	0,	2,	16,	18,	},
	{	1,	3,	17,	19,	},
	{	8,	10,	24,	26,	},
	{	9,	11,	25,	27,	},
	{	4,	6,	20,	22,	},
	{	5,	7,	21,	23,	},
	{	12,	14,	28,	30,	},
	{	13,	15,	29,	31,	},
};

int CGSHandler::STORAGEPSMCT16S::m_nColumnSwizzleTable[2][16] =
{
	{	0,	2,	8,	10,	16,	18,	24,	26,	1,	3,	9,	11,	17,	19,	25,	27,	},
	{	4,	6,	12,	14,	20,	22,	28,	30,	5,	7,	13,	15,	21,	23,	29,	31,	},
};

int CGSHandler::STORAGEPSMT8::m_nBlockSwizzleTable[4][8] =
{
	{	0,	1,	4,	5,	16,	17,	20,	21	},
	{	2,	3,	6,	7,	18,	19,	22,	23	},
	{	8,	9,	12,	13,	24,	25,	28,	29	},
	{	10,	11,	14,	15,	26,	27,	30,	31	},
};

int CGSHandler::STORAGEPSMT8::m_nColumnWordTable[2][2][8] =
{
	{
		{	0,	1,	4,	5,	8,	9,	12,	13,	},
		{	2,	3,	6,	7,	10,	11,	14,	15,	},
	},
	{
		{	8,	9,	12,	13,	0,	1,	4,	5,	},
		{	10,	11,	14,	15,	2,	3,	6,	7,	},
	},
};

int CGSHandler::STORAGEPSMT4::m_nBlockSwizzleTable[8][4] =
{
	{	0,	2,	8,	10,	},
	{	1,	3,	9,	11,	},
	{	4,	6,	12,	14,	},
	{	5,	7,	13,	15,	},
	{	16,	18,	24,	26,	},
	{	17,	19,	25,	27,	},
	{	20,	22,	28,	30,	},
	{	21,	23,	29,	31,	}
};

int CGSHandler::STORAGEPSMT4::m_nColumnWordTable[2][2][8] =
{
	{
		{	0,	1,	4,	5,	8,	9,	12,	13,	},
		{	2,	3,	6,	7,	10,	11,	14,	15,	},
	},
	{
		{	8,	9,	12,	13,	0,	1,	4,	5,	},
		{	10,	11,	14,	15,	2,	3,	6,	7,	},
	},
};

CGSHandler::CGSHandler() :
m_thread(NULL)
{
	m_pRAM = (uint8*)malloc(RAMSIZE);

	for(int i = 0; i < PSM_MAX; i++)
	{
		m_pTransferHandler[i]					= &CGSHandler::TrxHandlerInvalid;
	}

	m_pTransferHandler[PSMCT32]					= &CGSHandler::TrxHandlerCopy<STORAGEPSMCT32>;
	m_pTransferHandler[PSMCT24]					= &CGSHandler::TrxHandlerPSMCT24;
	m_pTransferHandler[PSMCT16]					= &CGSHandler::TrxHandlerCopy<STORAGEPSMCT16>;
	m_pTransferHandler[PSMT8]					= &CGSHandler::TrxHandlerCopy<STORAGEPSMT8>;
	m_pTransferHandler[PSMT4]					= &CGSHandler::TrxHandlerPSMT4;
	m_pTransferHandler[PSMT8H]					= &CGSHandler::TrxHandlerPSMT8H;
	m_pTransferHandler[PSMT4HL]					= &CGSHandler::TrxHandlerPSMT4H<24, 0x0F000000>;
	m_pTransferHandler[PSMT4HH]					= &CGSHandler::TrxHandlerPSMT4H<28, 0xF0000000>;

	Reset();

    m_thread = new thread(bind(&CGSHandler::ThreadProc, this));
}

CGSHandler::~CGSHandler()
{
	FREEPTR(m_pRAM);
}

void CGSHandler::Reset()
{
	memset(m_nReg, 0, sizeof(uint64) * 0x80);
	m_nReg[GS_REG_PRMODECONT] = 1;
	memset(m_pRAM, 0, RAMSIZE);
	m_nPMODE = 0;
}

void CGSHandler::SaveState(CStream* pS)
{
	pS->Write(m_nReg, sizeof(uint64) * 0x80);

	pS->Write(&m_nPMODE,			8);
	pS->Write(&m_nDISPFB1,			8);
	pS->Write(&m_nDISPLAY1,			8);
	pS->Write(&m_nDISPFB2,			8);
	pS->Write(&m_nDISPLAY2,			8);
	pS->Write(&m_nCSR,				8);
	pS->Write(&m_nIMR,				8);

	pS->Write(&m_nCrtIsInterlaced,	4);
	pS->Write(&m_nCrtMode,			4);
	pS->Write(&m_nCrtIsFrameMode,	4);

	pS->Write(&m_TrxCtx,			sizeof(TRXCONTEXT));

	pS->Write(m_pRAM,				RAMSIZE);
}

void CGSHandler::LoadState(CStream* pS)
{
	pS->Read(m_nReg, sizeof(uint64) * 0x80);

	pS->Read(&m_nPMODE,				8);
	pS->Read(&m_nDISPFB1,			8);
	pS->Read(&m_nDISPLAY1,			8);
	pS->Read(&m_nDISPFB2,			8);
	pS->Read(&m_nDISPLAY2,			8);
	pS->Read(&m_nCSR,				8);
	pS->Read(&m_nIMR,				8);

	pS->Read(&m_nCrtIsInterlaced,	4);
	pS->Read(&m_nCrtMode,			4);
	pS->Read(&m_nCrtIsFrameMode,	4);

	pS->Read(&m_TrxCtx,				sizeof(TRXCONTEXT));

	pS->Read(m_pRAM,				RAMSIZE);

	UpdateViewport();
}

void CGSHandler::SetVBlank()
{
	m_nCSR |= 0x08;

//	CINTC::AssertLine(CINTC::INTC_LINE_VBLANK_START);
}

void CGSHandler::ResetVBlank()
{
	m_nCSR &= ~0x08;

	//Alternate current field
	m_nCSR ^= 0x2000;

//	CINTC::AssertLine(CINTC::INTC_LINE_VBLANK_END);
}

uint32 CGSHandler::ReadPrivRegister(uint32 nAddress)
{
	uint32 nData;
	switch(nAddress >> 4)
	{
	case 0x1200100:
		//Force CSR to have the H-Blank bit set.
		m_nCSR |= 0x04;
		R_REG(nAddress, nData, m_nCSR);
		break;
	default:
		printf("GS: Read an unhandled priviledged register (0x%0.8X).\r\n", nAddress);
		nData = 0xCCCCCCCC;
		break;
	}
	return nData;
}

void CGSHandler::WritePrivRegister(uint32 nAddress, uint32 nData)
{
	switch(nAddress >> 4)
	{
	case 0x1200000:
		W_REG(nAddress, nData, m_nPMODE);
		if(!(nAddress & 0x4))
		{
			if((m_nPMODE & 0x01) && (m_nPMODE & 0x02))
			{
				printf("GS: Warning. Both read circuits were enabled. Using RC1 for display.\r\n");
				m_nPMODE &= ~0x02;
			}
		}
		break;
	case 0x1200007:
		W_REG(nAddress, nData, m_nDISPFB1);
		if(nAddress & 0x04)
		{
//			if(CPS2VM::m_Logging.GetGSLoggingStatus())
//			{
//				DISPFB* dispfb;
//				dispfb = GetDispFb(0);
//				printf("GS: DISPFB1(FBP: 0x%0.8X, FBW: %i, PSM: %i, DBX: %i, DBY: %i);\r\n", \
//					dispfb->GetBufPtr(), \
//					dispfb->GetBufWidth(), \
//					dispfb->nPSM, \
//					dispfb->nX, \
//					dispfb->nY);
//			}
#ifdef _DEBUG
			DISPFB* dispfb;
			dispfb = GetDispFb(0);
            CLog::GetInstance().Print("gs", "DISPFB1(FBP: 0x%0.8X, FBW: %i, PSM: %i, DBX: %i, DBY: %i);\r\n", \
				dispfb->GetBufPtr(), \
				dispfb->GetBufWidth(), \
				dispfb->nPSM, \
				dispfb->nX, \
				dispfb->nY);
#endif
		}
		break;
	case 0x1200008:
		W_REG(nAddress, nData, m_nDISPLAY1);
		if(nAddress & 0x04)
		{
			UpdateViewport();
		}
		break;
	case 0x1200009:
		W_REG(nAddress, nData, m_nDISPFB2);
		if(nAddress & 0x04)
		{
//			if(CPS2VM::m_Logging.GetGSLoggingStatus())
//			{
//				DISPFB* dispfb;
//				dispfb = GetDispFb(1);
//				printf("GS: DISPFB2(FBP: 0x%0.8X, FBW: %i, PSM: %i, DBX: %i, DBY: %i);\r\n", \
//					dispfb->GetBufPtr(), \
//					dispfb->GetBufWidth(), \
//					dispfb->nPSM, \
//					dispfb->nX, \
//					dispfb->nY);
//			}
#ifdef _DEBUG
			DISPFB* dispfb;
			dispfb = GetDispFb(1);
            CLog::GetInstance().Print("gs", "DISPFB2(FBP: 0x%0.8X, FBW: %i, PSM: %i, DBX: %i, DBY: %i);\r\n", \
				dispfb->GetBufPtr(), \
				dispfb->GetBufWidth(), \
				dispfb->nPSM, \
				dispfb->nX, \
				dispfb->nY);
#endif
			Flip();
		}
		break;
	case 0x120000A:
		W_REG(nAddress, nData, m_nDISPLAY2);
		if(nAddress & 0x04)
		{
			UpdateViewport();
		}
		break;
	case 0x1200100:
		W_REG(nAddress, nData, m_nCSR);
		if(!(nAddress & 0x04))
		{
			if(nData & 0x08)
			{
				ResetVBlank();
			}
		}
		break;
	case 0x1200101:
		W_REG(nAddress, nData, m_nIMR);
		break;
	default:
		printf("GS: Wrote to an unhandled priviledged register (0x%0.8X, 0x%0.8X).\r\n", nAddress, nData);
		break;
	}
}

void CGSHandler::Initialize()
{
    m_mailBox.SendCall(bind(&CGSHandler::InitializeImpl, this));
}

void CGSHandler::Flip()
{
    m_mailBox.SendCall(bind(&CGSHandler::FlipImpl, this));
}

void CGSHandler::WriteRegister(uint8 registerId, uint64 value)
{
    m_mailBox.SendCall(bind(&CGSHandler::WriteRegisterImpl, this, registerId, value));
}

void CGSHandler::FeedImageData(void* data, uint32 length)
{
    m_mailBox.SendCall(bind(&CGSHandler::FeedImageDataImpl, this, data, length));
}

void CGSHandler::UpdateViewport()
{
    m_mailBox.SendCall(bind(&CGSHandler::UpdateViewportImpl, this));
}

void CGSHandler::WriteRegisterImpl(uint8 nRegister, uint64 nData)
{
	m_nReg[nRegister] = nData;

	switch(nRegister)
	{
	case GS_REG_TRXDIR:
		BITBLTBUF* pBuf;
		TRXREG* pReg;
		unsigned int nPixelSize;

		pBuf = GetBitBltBuf();
		pReg = GetTrxReg();

		//We need to figure out the pixel size of the source stream
		switch(pBuf->nDstPsm)
		{
		case PSMCT32:
			nPixelSize = 32;
			break;
		case PSMCT24:
			nPixelSize = 24;
			break;
		case PSMCT16:
			nPixelSize = 16;
			break;
		case PSMT8:
		case PSMT8H:
			nPixelSize = 8;
			break;
		case PSMT4:
		case PSMT4HH:
		case PSMT4HL:
			nPixelSize = 4;
			break;
		default:
			assert(0);
			break;
		}

		m_TrxCtx.nSize	= (pReg->nRRW * pReg->nRRH * nPixelSize) / 8;
		m_TrxCtx.nRRX	= 0;
		m_TrxCtx.nRRY	= 0;
		m_TrxCtx.nDirty	= false;

		break;
	}

#ifdef _DEBUG
	DisassembleWrite(nRegister, nData);
#endif
}

void CGSHandler::FeedImageDataImpl(void* pData, uint32 nLength)
{
	BITBLTBUF* pBuf;

	if(m_TrxCtx.nSize == 0)
	{
#ifdef _DEBUG
		printf("GS: Warning. Received image data when no transfer was expected.\r\n");
#endif
		return;
	}

	if(m_TrxCtx.nSize < nLength)
	{
		nLength = m_TrxCtx.nSize;
		//assert(0);
		//return;
	}

	pBuf = GetBitBltBuf();
	if(pBuf->nDstPsm == 1) return;
	m_TrxCtx.nDirty |= ((this)->*(m_pTransferHandler[pBuf->nDstPsm]))(pData, nLength);

	m_TrxCtx.nSize -= nLength;

	if(m_TrxCtx.nSize == 0)
	{
		if(m_TrxCtx.nDirty)
		{
			BITBLTBUF* pBuf;
			TRXREG* pReg;
			uint32 nSize;

			pBuf = GetBitBltBuf();
			pReg = GetTrxReg();

			nSize = (pBuf->GetDstWidth() * pReg->nRRH * GetPsmPixelSize(pBuf->nDstPsm)) / 8;

			ProcessImageTransfer(pBuf->GetDstPtr(), nSize);
		}
	}
}

void CGSHandler::FetchImagePSMCT16(uint16* pDst, uint32 nBufPos, uint32 nBufWidth, uint32 nWidth, uint32 nHeight)
{
	CPixelIndexorPSMCT16 Indexor(m_pRAM, nBufPos, nBufWidth);

	for(unsigned int j = 0; j < nHeight; j++)
	{
		for(unsigned int i = 0; i < nWidth; i++)
		{
			pDst[i] = Indexor.GetPixel(i, j);
		}

		pDst += (nWidth);
	}
}

void CGSHandler::FetchImagePSMCT16S(uint16* pDst, uint32 nBufPos, uint32 nBufWidth, uint32 nWidth, uint32 nHeight)
{
	CPixelIndexorPSMCT16S Indexor(m_pRAM, nBufPos, nBufWidth);

	for(unsigned int j = 0; j < nHeight; j++)
	{
		for(unsigned int i = 0; i < nWidth; i++)
		{
			pDst[i] = Indexor.GetPixel(i, j);
		}

		pDst += (nWidth);
	}
}

void CGSHandler::FetchImagePSMCT32(uint32* pDst, uint32 nBufPos, uint32 nBufWidth, uint32 nWidth, uint32 nHeight)
{
	CPixelIndexorPSMCT32 Indexor(m_pRAM, nBufPos, nBufWidth);

	for(unsigned int j = 0; j < nHeight; j++)
	{
		for(unsigned int i = 0; i < nWidth; i++)
		{
			pDst[i] = Indexor.GetPixel(i, j);
		}

		pDst += (nWidth);
	}
}

bool CGSHandler::TrxHandlerInvalid(void* pData, uint32 nLength)
{
	assert(0);
	return false;
}

template <typename Storage>
bool CGSHandler::TrxHandlerCopy(void* pData, uint32 nLength)
{
	typename Storage::Unit* pSrc;
	uint32 nX, nY;
	TRXPOS* pTrxPos;
	TRXREG* pTrxReg;
	BITBLTBUF* pTrxBuf;
	bool nDirty = false;

	nLength /= sizeof(typename Storage::Unit);
	pTrxPos = GetTrxPos();
	pTrxReg = GetTrxReg();
	pTrxBuf = GetBitBltBuf();

	CPixelIndexor<Storage> Indexor(m_pRAM, pTrxBuf->GetDstPtr(), pTrxBuf->nDstWidth);

	pSrc = (typename Storage::Unit*)pData;

	for(unsigned int i = 0; i < nLength; i++)
	{
		typename Storage::Unit* pPixel;

		nX = (m_TrxCtx.nRRX + pTrxPos->nDSAX) % 2048;
		nY = (m_TrxCtx.nRRY + pTrxPos->nDSAY) % 2048;

		pPixel = Indexor.GetPixelAddress(nX, nY);

		if((*pPixel) != pSrc[i])
		{
			(*pPixel) = pSrc[i];
			nDirty = true;
		}

		m_TrxCtx.nRRX++;
		if(m_TrxCtx.nRRX == pTrxReg->nRRW)
		{
			m_TrxCtx.nRRX = 0;
			m_TrxCtx.nRRY++;
		}
	}

	return nDirty;
}

bool CGSHandler::TrxHandlerPSMCT24(void* pData, uint32 nLength)
{
	uint8* pSrc;
	uint32* pDstPixel;
	uint32 nSrcPixel;
	uint32 nX, nY;
	TRXPOS* pTrxPos;
	TRXREG* pTrxReg;
	BITBLTBUF* pTrxBuf;

	pTrxPos = GetTrxPos();
	pTrxReg = GetTrxReg();
	pTrxBuf = GetBitBltBuf();

	CPixelIndexorPSMCT32 Indexor(m_pRAM, pTrxBuf->GetDstPtr(), pTrxBuf->nDstWidth);

	pSrc = (uint8*)pData;

	for(unsigned int i = 0; i < nLength; i += 3)
	{
		nX = (m_TrxCtx.nRRX + pTrxPos->nDSAX) % 2048;
		nY = (m_TrxCtx.nRRY + pTrxPos->nDSAY) % 2048;

		pDstPixel = Indexor.GetPixelAddress(nX, nY);
		nSrcPixel = (*(uint32*)&pSrc[i]) & 0x00FFFFFF;
		(*pDstPixel) &= 0xFF000000;
		(*pDstPixel) |= nSrcPixel;

		m_TrxCtx.nRRX++;
		if(m_TrxCtx.nRRX == pTrxReg->nRRW)
		{
			m_TrxCtx.nRRX = 0;
			m_TrxCtx.nRRY++;
		}
	}

	return true;
}

bool CGSHandler::TrxHandlerPSMT4(void* pData, uint32 nLength)
{
	//Gotta rewrite this

	uint8* pSrc;
	TRXPOS* pTrxPos;
	TRXREG* pTrxReg;
	BITBLTBUF* pTrxBuf;

	pTrxPos = GetTrxPos();
	pTrxReg = GetTrxReg();
	pTrxBuf = GetBitBltBuf();

	CPixelIndexorPSMT4 Indexor(m_pRAM, pTrxBuf->GetDstPtr(), pTrxBuf->nDstWidth);

	assert(0);

	pSrc = (uint8*)pData;
/*
	for(unsigned int i = 0; i < nLength; i++)
	{
		uint8 nPixel[2];

		nPixel[0] = (pSrc[i] >> 0) & 0x0F;
		nPixel[1] = (pSrc[i] >> 4) & 0x0F;

		for(unsigned int j = 0; j < 2; j++)
		{
			uint32 nX, nY;

			nX = (m_TrxCtx.nRRX + pTrxPos->nDSAX) % 2048;
			nY = (m_TrxCtx.nRRY + pTrxPos->nDSAY) % 2048;

			Indexor.SetPixel(nX, nY, nPixel[j]);

			m_TrxCtx.nRRX++;
			if(m_TrxCtx.nRRX == pTrxReg->nRRW)
			{
				m_TrxCtx.nRRX = 0;
				m_TrxCtx.nRRY++;
			}
		}
	}
*/
	return true;
}

template <uint32 nShift, uint32 nMask>
bool CGSHandler::TrxHandlerPSMT4H(void* pData, uint32 nLength)
{
	uint8* pSrc;
	uint8 nSrcPixel;
	uint32 nX, nY;
	uint32* pDstPixel;
	TRXPOS* pTrxPos;
	TRXREG* pTrxReg;
	BITBLTBUF* pTrxBuf;

	pTrxPos = GetTrxPos();
	pTrxReg = GetTrxReg();
	pTrxBuf = GetBitBltBuf();

	CPixelIndexorPSMCT32 Indexor(m_pRAM, pTrxBuf->GetDstPtr(), pTrxBuf->nDstWidth);

	pSrc = (uint8*)pData;

	for(unsigned int i = 0; i < nLength; i++)
	{
		//Pixel 1
		nX = (m_TrxCtx.nRRX + pTrxPos->nDSAX) % 2048;
		nY = (m_TrxCtx.nRRY + pTrxPos->nDSAY) % 2048;

		nSrcPixel = pSrc[i] & 0x0F;

		pDstPixel = Indexor.GetPixelAddress(nX, nY);
		(*pDstPixel) &= ~nMask;
		(*pDstPixel) |= (nSrcPixel << nShift);

		m_TrxCtx.nRRX++;
		if(m_TrxCtx.nRRX == pTrxReg->nRRW)
		{
			m_TrxCtx.nRRX = 0;
			m_TrxCtx.nRRY++;
		}

		//Pixel 2
		nX = (m_TrxCtx.nRRX + pTrxPos->nDSAX) % 2048;
		nY = (m_TrxCtx.nRRY + pTrxPos->nDSAY) % 2048;

		nSrcPixel = (pSrc[i] & 0xF0);

		pDstPixel = Indexor.GetPixelAddress(nX, nY);
		(*pDstPixel) &= ~nMask;
		(*pDstPixel) |= (nSrcPixel << (nShift - 4));

		m_TrxCtx.nRRX++;
		if(m_TrxCtx.nRRX == pTrxReg->nRRW)
		{
			m_TrxCtx.nRRX = 0;
			m_TrxCtx.nRRY++;
		}
	}

	return true;
}

bool CGSHandler::TrxHandlerPSMT8H(void* pData, uint32 nLength)
{
	uint8* pSrc;
	uint8 nSrcPixel;
	uint32 nX, nY;
	uint32* pDstPixel;
	TRXPOS* pTrxPos;
	TRXREG* pTrxReg;
	BITBLTBUF* pTrxBuf;

	pTrxPos = GetTrxPos();
	pTrxReg = GetTrxReg();
	pTrxBuf = GetBitBltBuf();

	CPixelIndexorPSMCT32 Indexor(m_pRAM, pTrxBuf->GetDstPtr(), pTrxBuf->nDstWidth);

	pSrc = (uint8*)pData;

	for(unsigned int i = 0; i < nLength; i++)
	{
		nX = (m_TrxCtx.nRRX + pTrxPos->nDSAX) % 2048;
		nY = (m_TrxCtx.nRRY + pTrxPos->nDSAY) % 2048;

		nSrcPixel = pSrc[i];

		pDstPixel = Indexor.GetPixelAddress(nX, nY);
		(*pDstPixel) &= ~0xFF000000;
		(*pDstPixel) |= (nSrcPixel << 24);

		m_TrxCtx.nRRX++;
		if(m_TrxCtx.nRRX == pTrxReg->nRRW)
		{
			m_TrxCtx.nRRX = 0;
			m_TrxCtx.nRRY++;
		}
	}

	return true;
}

void CGSHandler::SetCrt(bool nIsInterlaced, unsigned int nMode, bool nIsFrameMode)
{
	m_nCrtMode			= nMode;
	m_nCrtIsInterlaced	= nIsInterlaced;
	m_nCrtIsFrameMode	= nIsFrameMode;

	UpdateViewport();
}

CGSHandler::DISPFB* CGSHandler::GetDispFb(unsigned int nUnit)
{
	switch(nUnit)
	{
	case 0:
		return (DISPFB*)&m_nDISPFB1;
		break;
	case 1:
		return (DISPFB*)&m_nDISPFB2;
		break;
	default:
		assert(0);
		return NULL;
		break;
	}
}

CGSHandler::TEXCLUT* CGSHandler::GetTexClut()
{
	return (TEXCLUT*)&m_nReg[GS_REG_TEXCLUT];
}

CGSHandler::FOGCOL* CGSHandler::GetFogCol()
{
	return (FOGCOL*)&m_nReg[GS_REG_FOGCOL];
}

CGSHandler::FRAME* CGSHandler::GetFrame(unsigned int nUnit)
{
	assert(nUnit < 2);
	return (FRAME*)&m_nReg[GS_REG_FRAME_1 + nUnit];
}

CGSHandler::TRXREG* CGSHandler::GetTrxReg()
{
	return (TRXREG*)&m_nReg[GS_REG_TRXREG];
}

CGSHandler::TRXPOS* CGSHandler::GetTrxPos()
{
	return (TRXPOS*)&m_nReg[GS_REG_TRXPOS];
}

CGSHandler::BITBLTBUF* CGSHandler::GetBitBltBuf()
{
	return (BITBLTBUF*)&m_nReg[GS_REG_BITBLTBUF];
}

unsigned int CGSHandler::GetCrtWidth()
{
	switch(m_nCrtMode)
	{
	case 0x02:
	case 0x03:
	case 0x1C:
		return 640;
		break;
	default:
		assert(0);
		return 640;
		break;
	}
}

unsigned int CGSHandler::GetCrtHeight()
{
	switch(m_nCrtMode)
	{
	case 0x02:
		return 448;
		break;
	case 0x03:
		return 512;
		break;
	case 0x1C:
		return 480;
		break;
	default:
		assert(0);
		return 448;
		break;
	}
}

bool CGSHandler::GetCrtIsInterlaced()
{
	return m_nCrtIsInterlaced;
}

bool CGSHandler::GetCrtIsFrameMode()
{
	return m_nCrtIsFrameMode;
}

unsigned int CGSHandler::GetPsmPixelSize(unsigned int nPSM)
{
	switch(nPSM)
	{
	case PSMCT32:
	case PSMT4HH:
	case PSMT4HL:
	case PSMT8H:
		return 32;
		break;
	case PSMCT24:
		return 24;
		break;
	case PSMCT16:
	case PSMCT16S:
		return 16;
		break;
	case PSMT8:
		return 8;
		break;
	case PSMT4:
		return 4;
		break;
	default:
		assert(0);
		return 0;
		break;
	}
}

void CGSHandler::DisassembleWrite(uint8 nRegister, uint64 nData)
{
	GSPRIM pr;
	double nX, nY, nZ;
	double nU, nV;
	double nS, nT;

//	if(!CPS2VM::m_Logging.GetGSLoggingStatus()) return;

	//Filtering
	//if(!((nRegister == GS_REG_FRAME_1) || (nRegister == GS_REG_FRAME_2))) return;
	//if(!((nRegister == GS_REG_TEST_1) || (nRegister == GS_REG_TEST_2))) return;

	switch(nRegister)
	{
	case GS_REG_PRIM:
		DECODE_PRIM(nData, pr);
		printf("GS: PRIM(PRI: %i, IIP: %i, TME: %i, FGE: %i, ABE: %i, AA1: %i, FST: %i, CTXT: %i, FIX: %i);\r\n", \
			pr.nType, \
			pr.nShading, \
			pr.nTexture, \
			pr.nFog, \
			pr.nAlpha, \
			pr.nAntiAliasing, \
			pr.nUseUV, \
			pr.nContext, \
			pr.nUseFloat);
		break;
	case GS_REG_RGBAQ:
		GSRGBAQ rgbaq;
		DECODE_RGBAQ(nData, rgbaq);
		printf("GS: RGBAQ(R: 0x%0.2X, G: 0x%0.2X, B: 0x%0.2X, A: 0x%0.2X, Q: %f);\r\n", \
			rgbaq.nR,
			rgbaq.nG,
			rgbaq.nB,
			rgbaq.nA,
			rgbaq.nQ);
		break;
	case GS_REG_ST:
		DECODE_ST(nData, nS, nT);
		printf("GS: ST(S: %f, T: %f);\r\n", \
			nS, \
			nT);
		break;
	case GS_REG_UV:
		DECODE_UV(nData, nU, nV);
		printf("GS: UV(U: %f, V: %f);\r\n", \
			nU, \
			nV);
		break;
	case GS_REG_XYZ2:
		DECODE_XYZ2(nData, nX, nY, nZ);
		printf("GS: XYZ2(%f, %f, %f);\r\n", \
			nX, \
			nY, \
			nZ);
		break;
	case GS_REG_XYZF2:
		{
			XYZF xyzf;
			xyzf = *reinterpret_cast<XYZF*>(&nData);
			printf("GS: XYZF2(%f, %f, %i, %i);\r\n", \
				xyzf.GetX(),
				xyzf.GetY(),
				xyzf.nZ,
				xyzf.nF);
		}
		break;
	case GS_REG_XYZ3:
		DECODE_XYZ2(nData, nX, nY, nZ);
		printf("GS: XYZ3(%f, %f, %f);\r\n", \
			nX, \
			nY, \
			nZ);
		break;
	case GS_REG_TEX0_1:
	case GS_REG_TEX0_2:
		{
			GSTEX0 tex;
			DECODE_TEX0(nData, tex);
			printf("GS: TEX0_%i(TBP: 0x%0.8X, TBW: %i, PSM: %i, TW: %i, TH: %i, TCC: %i, TFX: %i, CBP: 0x%0.8X, CPSM: %i, CSM: %i, CSA: %i, CLD: %i);\r\n", \
				nRegister == GS_REG_TEX0_1 ? 1 : 2, \
				tex.GetBufPtr(), \
				tex.GetBufWidth(), \
				tex.nPsm, \
				tex.GetWidth(), \
				tex.GetHeight(), \
				tex.nColorComp, \
				tex.nFunction, \
				tex.GetCLUTPtr(), \
				tex.nCPSM, \
				tex.nCSM, \
				tex.nCSA, \
				tex.nCLD);
		}
		break;
	case GS_REG_CLAMP_1:
	case GS_REG_CLAMP_2:
		{
			CLAMP clamp;
			clamp = *(CLAMP*)&nData;
			printf("GS: CLAMP_%i(WMS: %i, WMT: %i, MINU: %i, MAXU: %i, MINV: %i, MAXV: %i);\r\n", \
				nRegister == GS_REG_CLAMP_1 ? 1 : 2, \
				clamp.nWMS, \
				clamp.nWMT, \
				clamp.GetMinU(), \
				clamp.GetMaxU(), \
				clamp.GetMinV(), \
				clamp.GetMaxV());
		}
		break;
	case GS_REG_TEX1_1:
	case GS_REG_TEX1_2:
		{
			GSTEX1 tex1;
			DECODE_TEX1(nData, tex1);
			printf("GS: TEX1_%i(LCM: %i, MXL: %i, MMAG: %i, MMIN: %i, MTBA: %i, L: %i, K: %i);\r\n", \
				nRegister == GS_REG_TEX1_1 ? 1 : 2, \
				tex1.nLODMethod, \
				tex1.nMaxMip, \
				tex1.nMagFilter, \
				tex1.nMinFilter, \
				tex1.nMipBaseAddr, \
				tex1.nLODL, \
				tex1.nLODK);
		}
		break;
	case GS_REG_TEX2_1:
	case GS_REG_TEX2_2:
		TEX2 tex2;
		tex2 = *(TEX2*)&nData;
		printf("GS: TEX2_%i(PSM: %i, CBP: 0x%0.8X, CPSM: %i, CSM: %i, CSA: %i, CLD: %i);\r\n", \
			nRegister == GS_REG_TEX2_1 ? 1 : 2, \
			tex2.nPsm, \
			tex2.GetCLUTPtr(), \
			tex2.nCPSM, \
			tex2.nCSM, \
			tex2.nCSA, \
			tex2.nCLD);
		break;
	case GS_REG_XYOFFSET_1:
	case GS_REG_XYOFFSET_2:
		printf("GS: XYOFFSET_%i(%i, %i);\r\n", \
			nRegister == GS_REG_XYOFFSET_1 ? 1 : 2, \
			(uint32)((nData >>  0) & 0xFFFFFFFF), \
			(uint32)((nData >> 32) & 0xFFFFFFFF));
		break;
	case GS_REG_PRMODECONT:
		printf("GS: PRMODECONT(AC = %i);\r\n", \
			nData & 1);
		break;
	case GS_REG_PRMODE:
		GSPRMODE prm;
		DECODE_PRMODE(nData, prm);
		printf("GS: PMMODE(IIP: %i, TME: %i, FGE: %i, ABE: %i, AA1: %i, FST: %i, CTXT: %i, FIX: %i);\r\n", \
			prm.nShading, \
			prm.nTexture, \
			prm.nFog, \
			prm.nAlpha, \
			prm.nAntiAliasing, \
			prm.nUseUV, \
			prm.nContext, \
			prm.nUseFloat);
		break;
	case GS_REG_TEXCLUT:
		TEXCLUT clut;
		clut = *(TEXCLUT*)&nData;
		printf("GS: TEXCLUT(CBW: %i, COU: %i, COV: %i);\r\n", \
			clut.nCBW,
			clut.nCOU,
			clut.nCOV);
		break;
	case GS_REG_FOGCOL:
		FOGCOL fogcol;
		fogcol = *(FOGCOL*)&nData;
		printf("GS: FOGCOL(R: 0x%0.2X, G: 0x%0.2X, B: 0x%0.2X);\r\n", \
			fogcol.nFCR,
			fogcol.nFCG,
			fogcol.nFCB);
		break;
	case GS_REG_TEXA:
		GSTEXA TexA;
		DECODE_TEXA(nData, TexA);
		printf("GS: TEXA(TA0: 0x%0.2X, AEM: %i, TA1: 0x%0.2X);\r\n", \
			TexA.nTA0,
			TexA.nAEM,
			TexA.nTA1);
		break;
	case GS_REG_TEXFLUSH:
		printf("GS: TEXFLUSH();\r\n");
		break;
	case GS_REG_ALPHA_1:
	case GS_REG_ALPHA_2:
		GSALPHA alpha;
		DECODE_ALPHA(nData, alpha);
		printf("GS: ALPHA_%i(A: %i, B: %i, C: %i, D: %i, FIX: 0x%0.2X);\r\n", \
			nRegister == GS_REG_ALPHA_1 ? 1 : 2, \
			alpha.nA, \
			alpha.nB, \
			alpha.nC, \
			alpha.nD, \
			alpha.nFix);
		break;
	case GS_REG_SCISSOR_1:
		printf("GS: SCISSOR_1(%i, %i, %i, %i);\r\n", \
			(uint32)((nData >>  0) & 0xFFFF), \
			(uint32)((nData >> 16) & 0xFFFF), \
			(uint32)((nData >> 32) & 0xFFFF), \
			(uint32)((nData >> 48) & 0xFFFF));
		break;
	case GS_REG_TEST_1:
	case GS_REG_TEST_2:
		GSTEST tst;
		DECODE_TEST(nData, tst);
		printf("GS: TEST_%i(ATE: %i, ATST: %i, AREF: 0x%0.2X, AFAIL: %i, DATE: %i, DATM: %i, ZTE: %i, ZTST: %i);\r\n", \
			nRegister == GS_REG_TEST_1 ? 1 : 2, \
			tst.nAlphaEnabled, \
			tst.nAlphaMethod, \
			tst.nAlphaRef, \
			tst.nAlphaFail, \
			tst.nDestAlphaEnabled, \
			tst.nDestAlphaMode, \
			tst.nDepthEnabled, \
			tst.nDepthMethod);
		break;
	case GS_REG_FRAME_1:
	case GS_REG_FRAME_2:
		FRAME fr;
		fr = *(FRAME*)&nData;
		printf("GS: FRAME_%i(FBP: 0x%0.8X, FBW: %i, PSM: %i, FBMSK: %i);\r\n", \
			nRegister == GS_REG_FRAME_1 ? 1 : 2, \
			fr.GetBasePtr(), \
			fr.GetWidth(), \
			fr.nPsm, \
			fr.nMask);
		break;
	case GS_REG_ZBUF_1:
	case GS_REG_ZBUF_2:
		ZBUF zbuf;
		zbuf = *(ZBUF*)&nData;
		printf("GS: ZBUF_%i(ZBP: 0x%0.8X, PSM: %i, ZMSK: %i);\r\n", \
			nRegister == GS_REG_ZBUF_1 ? 1 : 2, \
			zbuf.GetBasePtr(), \
			zbuf.nPsm, \
			zbuf.nMask);
		break;
	case GS_REG_BITBLTBUF:
		BITBLTBUF buf;
		buf = *(BITBLTBUF*)&nData;
		printf("GS: BITBLTBUF(0x%0.8X, %i, %i, 0x%0.8X, %i, %i);\r\n", \
			buf.GetSrcPtr(), \
			buf.GetSrcWidth(), \
			buf.nSrcPsm, \
			buf.GetDstPtr(), \
			buf.GetDstWidth(), \
			buf.nDstPsm);
		break;
	case GS_REG_TRXPOS:
		printf("GS: TRXPOS(%i, %i, %i, %i, %i);\r\n", \
			(uint32)((nData >>  0) & 0xFFFF), \
			(uint32)((nData >> 16) & 0xFFFF), \
			(uint32)((nData >> 32) & 0xFFFF), \
			(uint32)((nData >> 48) & 0xFF),   \
			(uint32)((nData >> 59) & 0x1));
		break;
	case GS_REG_TRXREG:
		printf("GS: TRXREG(%i, %i);\r\n", \
			(uint32)((nData >>  0) & 0xFFFFFFFF),
			(uint32)((nData >> 32) & 0xFFFFFFFF));
		break;
	case GS_REG_TRXDIR:
		printf("GS: TRXDIR(%i);\r\n", \
			(uint32)((nData >>  0) & 0xFFFFFFFF));
		break;
	default:
		printf("GS: Unknown command (0x%X).\r\n", nRegister); 
		break;
	}
}

void CGSHandler::ThreadProc()
{
    while(1)
    {
        m_mailBox.WaitForCall();
        while(m_mailBox.IsPending())
        {
            m_mailBox.ReceiveCall();
        }
    }
}
