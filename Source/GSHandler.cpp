#include <stdio.h>
#include <string.h>
#include <functional>
#include <boost/scoped_array.hpp>
#include "AppConfig.h"
#include "GSHandler.h"
#include "GsPixelFormats.h"
#include "PtrMacro.h"
#include "Log.h"
#include "MemoryStateFile.h"
#include "RegisterStateFile.h"

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

#define STATE_RAM						("gs/ram")
#define STATE_REGS						("gs/regs")
#define STATE_TRXCTX					("gs/trxcontext")
#define STATE_PRIVREGS					("gs/privregs.xml")

#define STATE_PRIVREGS_PMODE			("PMODE")
#define STATE_PRIVREGS_SMODE2			("SMODE2")
#define STATE_PRIVREGS_DISPFB1			("DISPFB1")
#define STATE_PRIVREGS_DISPLAY1			("DISPLAY1")
#define STATE_PRIVREGS_DISPFB2			("DISPFB2")
#define STATE_PRIVREGS_DISPLAY2			("DISPLAY2")
#define STATE_PRIVREGS_CSR				("CSR")
#define STATE_PRIVREGS_IMR				("IMR")
#define STATE_PRIVREGS_CRTMODE			("CrtMode")

#define LOG_NAME						("gs")

CGSHandler::CGSHandler()
: m_threadDone(false)
, m_flipMode(FLIP_MODE_SMODE2)
, m_drawCallCount(0)
, m_pCLUT(nullptr)
, m_pRAM(nullptr)
{
	CAppConfig::GetInstance().RegisterPreferenceInteger(PREF_CGSHANDLER_FLIPMODE, FLIP_MODE_SMODE2);
	CAppConfig::GetInstance().RegisterPreferenceInteger(PREF_CGSHANDLER_PRESENTATION_MODE, CGSHandler::PRESENTATION_MODE_FIT);
	
	m_presentationParams.mode = static_cast<PRESENTATION_MODE>(CAppConfig::GetInstance().GetPreferenceInteger(PREF_CGSHANDLER_PRESENTATION_MODE));
	m_presentationParams.windowWidth = 512;
	m_presentationParams.windowHeight = 384;
	
	m_pRAM = new uint8[RAMSIZE];
	m_pCLUT	= new uint16[CLUTENTRYCOUNT];

	for(int i = 0; i < PSM_MAX; i++)
	{
		m_pTransferHandler[i]					= &CGSHandler::TrxHandlerInvalid;
	}

	m_pTransferHandler[PSMCT32]					= &CGSHandler::TrxHandlerCopy<CGsPixelFormats::STORAGEPSMCT32>;
	m_pTransferHandler[PSMCT24]					= &CGSHandler::TrxHandlerPSMCT24;
	m_pTransferHandler[PSMCT16]					= &CGSHandler::TrxHandlerCopy<CGsPixelFormats::STORAGEPSMCT16>;
	m_pTransferHandler[PSMT8]					= &CGSHandler::TrxHandlerCopy<CGsPixelFormats::STORAGEPSMT8>;
	m_pTransferHandler[PSMT4]					= &CGSHandler::TrxHandlerPSMT4;
	m_pTransferHandler[PSMT8H]					= &CGSHandler::TrxHandlerPSMT8H;
	m_pTransferHandler[PSMT4HL]					= &CGSHandler::TrxHandlerPSMT4H<24, 0x0F000000>;
	m_pTransferHandler[PSMT4HH]					= &CGSHandler::TrxHandlerPSMT4H<28, 0xF0000000>;

	ResetBase();

	m_thread = std::thread([&] () { ThreadProc(); });
}

CGSHandler::~CGSHandler()
{
	m_threadDone = true;
	m_thread.join();
	delete [] m_pRAM;
	delete [] m_pCLUT;
}

void CGSHandler::Reset()
{
	ResetBase();
	m_mailBox.SendCall(std::bind(&CGSHandler::ResetImpl, this), true);
}

void CGSHandler::ResetBase()
{
	memset(m_nReg, 0, sizeof(uint64) * 0x80);
	m_nReg[GS_REG_PRMODECONT] = 1;
	memset(m_pRAM, 0, RAMSIZE);
	memset(m_pCLUT, 0, CLUTSIZE);
	m_nPMODE = 0;
	m_nSMODE2 = 0;
	m_nDISPFB1.heldValue = 0;
	m_nDISPFB1.value.q = 0;
	m_nDISPLAY1.heldValue = 0;
	m_nDISPLAY1.value.q = 0;
	m_nDISPFB2.heldValue = 0;
	m_nDISPFB2.value.q = 0;
	m_nDISPLAY2.heldValue = 0;
	m_nDISPLAY2.value.q = 0;
	m_nCSR = 0;
	m_nIMR = 0;
	m_nCrtMode = 2;
	m_nCBP0	= 0;
	m_nCBP1	= 0;
}

void CGSHandler::ResetImpl()
{

}

void CGSHandler::SetPresentationParams(const PRESENTATION_PARAMS& presentationParams)
{
	m_presentationParams = presentationParams;
}

void CGSHandler::SaveState(Framework::CZipArchiveWriter& archive)
{
	archive.InsertFile(new CMemoryStateFile(STATE_RAM,		m_pRAM,		RAMSIZE));
	archive.InsertFile(new CMemoryStateFile(STATE_REGS,		m_nReg,		sizeof(uint64) * 0x80));
	archive.InsertFile(new CMemoryStateFile(STATE_TRXCTX,	&m_TrxCtx,	sizeof(TRXCONTEXT)));

	{
		CRegisterStateFile* registerFile = new CRegisterStateFile(STATE_PRIVREGS);

		registerFile->SetRegister64(STATE_PRIVREGS_PMODE,			m_nPMODE);
		registerFile->SetRegister64(STATE_PRIVREGS_SMODE2,			m_nSMODE2);
		registerFile->SetRegister64(STATE_PRIVREGS_DISPFB1,			m_nDISPFB1.value.q);
		registerFile->SetRegister64(STATE_PRIVREGS_DISPLAY1,		m_nDISPLAY1.value.q);
		registerFile->SetRegister64(STATE_PRIVREGS_DISPFB2,			m_nDISPFB2.value.q);
		registerFile->SetRegister64(STATE_PRIVREGS_DISPLAY2,		m_nDISPLAY2.value.q);
		registerFile->SetRegister64(STATE_PRIVREGS_CSR,				m_nCSR);
		registerFile->SetRegister64(STATE_PRIVREGS_IMR,				m_nIMR);
		registerFile->SetRegister32(STATE_PRIVREGS_CRTMODE,			m_nCrtMode);

		archive.InsertFile(registerFile);
	}
}

void CGSHandler::LoadState(Framework::CZipArchiveReader& archive)
{
	archive.BeginReadFile(STATE_RAM		)->Read(m_pRAM,		RAMSIZE);
	archive.BeginReadFile(STATE_REGS	)->Read(m_nReg,		sizeof(uint64) * 0x80);
	archive.BeginReadFile(STATE_TRXCTX	)->Read(&m_TrxCtx,	sizeof(TRXCONTEXT));

	{
		CRegisterStateFile registerFile(*archive.BeginReadFile(STATE_PRIVREGS));
		m_nPMODE			= registerFile.GetRegister64(STATE_PRIVREGS_PMODE);
		m_nSMODE2			= registerFile.GetRegister64(STATE_PRIVREGS_SMODE2);
		m_nDISPFB1.value.q	= registerFile.GetRegister64(STATE_PRIVREGS_DISPFB1);
		m_nDISPLAY1.value.q	= registerFile.GetRegister64(STATE_PRIVREGS_DISPLAY1);
		m_nDISPFB2.value.q	= registerFile.GetRegister64(STATE_PRIVREGS_DISPFB2);
		m_nDISPLAY2.value.q	= registerFile.GetRegister64(STATE_PRIVREGS_DISPLAY2);
		m_nCSR				= registerFile.GetRegister64(STATE_PRIVREGS_CSR);
		m_nIMR				= registerFile.GetRegister64(STATE_PRIVREGS_IMR);
		m_nCrtMode			= registerFile.GetRegister32(STATE_PRIVREGS_CRTMODE);
	}
}

void CGSHandler::SetVBlank()
{
	if(m_flipMode == FLIP_MODE_VBLANK)
	{
		Flip();
	}

	std::lock_guard<std::recursive_mutex> registerMutexLock(m_registerMutex);
	m_nCSR |= CSR_VSYNC_INT;
}

void CGSHandler::ResetVBlank()
{
	std::lock_guard<std::recursive_mutex> registerMutexLock(m_registerMutex);

	m_nCSR &= ~CSR_VSYNC_INT;

	//Alternate current field
	m_nCSR ^= 0x2000;
}

uint32 CGSHandler::ReadPrivRegister(uint32 nAddress)
{
	uint32 nData = 0;
	switch(nAddress & ~0x0F)
	{
	case GS_CSR:
		//Force CSR to have the H-Blank bit set.
		{
			std::lock_guard<std::recursive_mutex> registerMutexLock(m_registerMutex);
			m_nCSR |= 0x04;
			R_REG(nAddress, nData, m_nCSR);
		}
		break;
	case GS_IMR:
		R_REG(nAddress, nData, m_nIMR);
		break;
	default:
		CLog::GetInstance().Print(LOG_NAME, "Read an unhandled priviledged register (0x%0.8X).\r\n", nAddress);
		nData = 0xCCCCCCCC;
		break;
	}
	return nData;
}

void CGSHandler::WritePrivRegister(uint32 nAddress, uint32 nData)
{
	switch(nAddress & ~0x0F)
	{
	case GS_PMODE:
		W_REG(nAddress, nData, m_nPMODE);
		if(!(nAddress & 0x4))
		{
			if((m_nPMODE & 0x01) && (m_nPMODE & 0x02))
			{
				CLog::GetInstance().Print(LOG_NAME, "Warning. Both read circuits were enabled. Using RC1 for display.\r\n");
//				m_nPMODE &= ~0x02;
			}
		}
		break;
	case GS_SMODE2:
		{
			W_REG(nAddress, nData, m_nSMODE2);
			if(nAddress & 0x04)
			{
				if(m_flipMode == FLIP_MODE_SMODE2)
				{
					Flip();
				}
			}
		}
		break;
	case GS_DISPFB1:
		WriteToDelayedRegister(nAddress, nData, m_nDISPFB1);
		break;
	case GS_DISPLAY1:
		WriteToDelayedRegister(nAddress, nData, m_nDISPLAY1);
		break;
	case GS_DISPFB2:
		WriteToDelayedRegister(nAddress, nData, m_nDISPFB2);
		if(nAddress & 0x04)
		{
			if(m_flipMode == FLIP_MODE_DISPFB2)
			{
				Flip();
			}
		}
		break;
	case GS_DISPLAY2:
		WriteToDelayedRegister(nAddress, nData, m_nDISPLAY2);
		break;
	case GS_CSR:
		{
			if(!(nAddress & 0x04))
			{
				SetVBlank();
				{
					std::lock_guard<std::recursive_mutex> registerMutexLock(m_registerMutex);
					if(nData & CSR_FINISH_EVENT)
					{
						m_nCSR &= ~CSR_FINISH_EVENT;
					}
					if(nData & CSR_VSYNC_INT)
					{
						ResetVBlank();
					}
					if(nData & CSR_RESET)
					{
						m_nCSR |= CSR_RESET;
					}
				}
			}
		}
		break;
	case GS_IMR:
		W_REG(nAddress, nData, m_nIMR);
		break;
	default:
		CLog::GetInstance().Print(LOG_NAME, "Wrote to an unhandled priviledged register (0x%0.8X, 0x%0.8X).\r\n", nAddress, nData);
		break;
	}

#ifdef _DEBUG
	if(nAddress & 0x04)
	{
		DisassemblePrivWrite(nAddress);
	}
#endif
}

void CGSHandler::Initialize()
{
	m_mailBox.SendCall(std::bind(&CGSHandler::InitializeImpl, this));
}

void CGSHandler::Release()
{
	m_mailBox.SendCall(std::bind(&CGSHandler::ReleaseImpl, this), true);
}

void CGSHandler::Flip(bool showOnly)
{
	if(!showOnly)
	{
		m_mailBox.FlushCalls();
		m_mailBox.SendCall(std::bind(&CGSHandler::MarkNewFrame, this));
	}
	m_mailBox.SendCall(std::bind(&CGSHandler::FlipImpl, this));
}

void CGSHandler::FlipImpl()
{

}

void CGSHandler::MarkNewFrame()
{
	OnNewFrame(m_drawCallCount);
	m_drawCallCount = 0;
#ifdef _DEBUG
	CLog::GetInstance().Print(LOG_NAME, "Frame Done.\r\n---------------------------------------------------------------------------------\r\n");
#endif
}

void CGSHandler::WriteRegister(uint8 registerId, uint64 value)
{
	m_mailBox.SendCall(std::bind(&CGSHandler::WriteRegisterImpl, this, registerId, value));
}

void CGSHandler::FeedImageData(void* data, uint32 length)
{
	uint8* buffer = new uint8[length];
	memcpy(buffer, data, length);
	m_mailBox.SendCall(std::bind(&CGSHandler::FeedImageDataImpl, this, buffer, length));
}

void CGSHandler::WriteRegisterMassively(const RegisterWrite* writeList, unsigned int count)
{
	RegisterWrite* writeListCopy = new CGSHandler::RegisterWrite[count];
	memcpy(writeListCopy, writeList, sizeof(CGSHandler::RegisterWrite) * count);
	m_mailBox.SendCall(bind(&CGSHandler::WriteRegisterMassivelyImpl, this, writeListCopy, count));
}

void CGSHandler::WriteRegisterImpl(uint8 nRegister, uint64 nData)
{
	m_nReg[nRegister] = nData;

	switch(nRegister)
	{
	case GS_REG_TEX0_1:
	case GS_REG_TEX0_2:
		{
			unsigned int nContext = nRegister - GS_REG_TEX0_1;
			assert(nContext == 0 || nContext == 1);
			TEX0 tex0;
			tex0 <<= m_nReg[GS_REG_TEX0_1 + nContext];
			SyncCLUT(tex0);
		}
		break;

	case GS_REG_TEX2_1:
	case GS_REG_TEX2_2:
		{
			//Update TEX0
			unsigned int nContext = nRegister - GS_REG_TEX2_1;
			assert(nContext == 0 || nContext == 1);

			const uint64 nMask = 0xFFFFFFE003F00000ULL;
			m_nReg[GS_REG_TEX0_1 + nContext] &= ~nMask;
			m_nReg[GS_REG_TEX0_1 + nContext] |= nData & nMask;

			TEX0 tex0;
			tex0 <<= m_nReg[GS_REG_TEX0_1 + nContext];
			SyncCLUT(tex0);
		}
		break;

	case GS_REG_TRXDIR:
		BeginTransfer();
		break;

	case GS_REG_FINISH:
		{
			std::lock_guard<std::recursive_mutex> registerMutexLock(m_registerMutex);
			m_nCSR |= CSR_FINISH_EVENT;
		}
		break;
	}

#ifdef _DEBUG
	DisassembleWrite(nRegister, nData);
#endif
}

void CGSHandler::FeedImageDataImpl(void* pData, uint32 nLength)
{
	boost::scoped_array<uint8> dataPtr(reinterpret_cast<uint8*>(pData));

	if(m_TrxCtx.nSize == 0)
	{
#ifdef _DEBUG
		CLog::GetInstance().Print(LOG_NAME, "Warning. Received image data when no transfer was expected.\r\n");
#endif
		return;
	}

	if(m_TrxCtx.nSize < nLength)
	{
		nLength = m_TrxCtx.nSize;
		//assert(0);
		//return;
	}

	BITBLTBUF bltBuf;
	bltBuf <<= m_nReg[GS_REG_BITBLTBUF];

	m_TrxCtx.nDirty |= ((this)->*(m_pTransferHandler[bltBuf.nDstPsm]))(pData, nLength);

	m_TrxCtx.nSize -= nLength;

	if(m_TrxCtx.nSize == 0)
	{
		TRXREG trxReg;
		trxReg <<= m_nReg[GS_REG_TRXREG];
		uint32 nSize = (bltBuf.GetDstWidth() * trxReg.nRRH * GetPsmPixelSize(bltBuf.nDstPsm)) / 8;
		ProcessImageTransfer(bltBuf.GetDstPtr(), nSize, m_TrxCtx.nDirty);

#ifdef _DEBUG
		CLog::GetInstance().Print(LOG_NAME, "Completed image transfer at 0x%0.8X (dirty = %d).\r\n", bltBuf.GetDstPtr(), m_TrxCtx.nDirty);
#endif
	}
}

void CGSHandler::WriteRegisterMassivelyImpl(const RegisterWrite* writeList, unsigned int count)
{
	const RegisterWrite* writeIterator = writeList;
	for(unsigned int i = 0; i < count; i++)
	{
		WriteRegisterImpl(writeIterator->first, writeIterator->second);
		writeIterator++;
	}
	delete [] writeList;
}

void CGSHandler::BeginTransfer()
{
	uint32 trxDir = m_nReg[GS_REG_TRXDIR] & 0x03;
	if(trxDir == 0)
	{
		//From Host to Local
		BITBLTBUF bltBuf;
		TRXREG trxReg;

		bltBuf <<= m_nReg[GS_REG_BITBLTBUF];
		trxReg <<= m_nReg[GS_REG_TRXREG];

		unsigned int nPixelSize;

		//We need to figure out the pixel size of the source stream
		switch(bltBuf.nDstPsm)
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

		m_TrxCtx.nSize	= (trxReg.nRRW * trxReg.nRRH * nPixelSize) / 8;
		m_TrxCtx.nRRX	= 0;
		m_TrxCtx.nRRY	= 0;
		m_TrxCtx.nDirty	= false;
	}
	else if(trxDir == 2)
	{
		//Local to Local
		ProcessLocalToLocalTransfer();
	}
}

void CGSHandler::FetchImagePSMCT16(uint16* pDst, uint32 nBufPos, uint32 nBufWidth, uint32 nWidth, uint32 nHeight)
{
	CGsPixelFormats::CPixelIndexorPSMCT16 Indexor(m_pRAM, nBufPos, nBufWidth);

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
	CGsPixelFormats::CPixelIndexorPSMCT16S Indexor(m_pRAM, nBufPos, nBufWidth);

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
	CGsPixelFormats::CPixelIndexorPSMCT32 Indexor(m_pRAM, nBufPos, nBufWidth);

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
	bool nDirty = false;
	TRXPOS* pTrxPos = GetTrxPos();
	TRXREG* pTrxReg = GetTrxReg();
	BITBLTBUF* pTrxBuf = GetBitBltBuf();

	nLength /= sizeof(typename Storage::Unit);

	CGsPixelFormats::CPixelIndexor<Storage> Indexor(m_pRAM, pTrxBuf->GetDstPtr(), pTrxBuf->nDstWidth);

	auto pSrc = reinterpret_cast<typename Storage::Unit*>(pData);

	for(unsigned int i = 0; i < nLength; i++)
	{
		uint32 nX = (m_TrxCtx.nRRX + pTrxPos->nDSAX) % 2048;
		uint32 nY = (m_TrxCtx.nRRY + pTrxPos->nDSAY) % 2048;

		auto pPixel = Indexor.GetPixelAddress(nX, nY);

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
	TRXPOS* pTrxPos = GetTrxPos();
	TRXREG* pTrxReg = GetTrxReg();
	BITBLTBUF* pTrxBuf = GetBitBltBuf();

	CGsPixelFormats::CPixelIndexorPSMCT32 Indexor(m_pRAM, pTrxBuf->GetDstPtr(), pTrxBuf->nDstWidth);

	uint8* pSrc = (uint8*)pData;

	for(unsigned int i = 0; i < nLength; i += 3)
	{
		uint32 nX = (m_TrxCtx.nRRX + pTrxPos->nDSAX) % 2048;
		uint32 nY = (m_TrxCtx.nRRY + pTrxPos->nDSAY) % 2048;

		uint32* pDstPixel = Indexor.GetPixelAddress(nX, nY);
		uint32 nSrcPixel = (*(uint32*)&pSrc[i]) & 0x00FFFFFF;
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
	bool dirty = false;
	TRXPOS* pTrxPos = GetTrxPos();
	TRXREG* pTrxReg = GetTrxReg();
	BITBLTBUF* pTrxBuf = GetBitBltBuf();

	CGsPixelFormats::CPixelIndexorPSMT4 Indexor(m_pRAM, pTrxBuf->GetDstPtr(), pTrxBuf->nDstWidth);

	uint8* pSrc = (uint8*)pData;

	for(unsigned int i = 0; i < nLength; i++)
	{
		uint8 nPixel[2];

		nPixel[0] = (pSrc[i] >> 0) & 0x0F;
		nPixel[1] = (pSrc[i] >> 4) & 0x0F;

		for(unsigned int j = 0; j < 2; j++)
		{
			uint32 nX = (m_TrxCtx.nRRX + pTrxPos->nDSAX) % 2048;
			uint32 nY = (m_TrxCtx.nRRY + pTrxPos->nDSAY) % 2048;

			uint8 currentPixel = Indexor.GetPixel(nX, nY);
			if(currentPixel != nPixel[j])
			{
				Indexor.SetPixel(nX, nY, nPixel[j]);
				dirty = true;
			}

			m_TrxCtx.nRRX++;
			if(m_TrxCtx.nRRX == pTrxReg->nRRW)
			{
				m_TrxCtx.nRRX = 0;
				m_TrxCtx.nRRY++;
			}
		}
	}

	return dirty;
}

template <uint32 nShift, uint32 nMask>
bool CGSHandler::TrxHandlerPSMT4H(void* pData, uint32 nLength)
{
	TRXPOS* pTrxPos = GetTrxPos();
	TRXREG* pTrxReg = GetTrxReg();
	BITBLTBUF* pTrxBuf = GetBitBltBuf();

	CGsPixelFormats::CPixelIndexorPSMCT32 Indexor(m_pRAM, pTrxBuf->GetDstPtr(), pTrxBuf->nDstWidth);

	uint8* pSrc = reinterpret_cast<uint8*>(pData);

	for(unsigned int i = 0; i < nLength; i++)
	{
		//Pixel 1
		uint32 nX = (m_TrxCtx.nRRX + pTrxPos->nDSAX) % 2048;
		uint32 nY = (m_TrxCtx.nRRY + pTrxPos->nDSAY) % 2048;

		uint8 nSrcPixel = pSrc[i] & 0x0F;

		uint32* pDstPixel = Indexor.GetPixelAddress(nX, nY);
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
	TRXPOS* pTrxPos = GetTrxPos();
	TRXREG* pTrxReg = GetTrxReg();
	BITBLTBUF* pTrxBuf = GetBitBltBuf();

	CGsPixelFormats::CPixelIndexorPSMCT32 Indexor(m_pRAM, pTrxBuf->GetDstPtr(), pTrxBuf->nDstWidth);

	uint8* pSrc = reinterpret_cast<uint8*>(pData);

	for(unsigned int i = 0; i < nLength; i++)
	{
		uint32 nX = (m_TrxCtx.nRRX + pTrxPos->nDSAX) % 2048;
		uint32 nY = (m_TrxCtx.nRRY + pTrxPos->nDSAY) % 2048;

		uint8 nSrcPixel = pSrc[i];

		uint32* pDstPixel = Indexor.GetPixelAddress(nX, nY);
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
	m_nCrtMode = nMode;

	SMODE2 smode2;
	smode2 <<= 0;
	smode2.interlaced	= nIsInterlaced ? 1 : 0;
	smode2.ffmd			= nIsFrameMode ? 1 : 0;
	m_nSMODE2 = smode2;
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

unsigned int CGSHandler::GetCrtWidth() const
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

unsigned int CGSHandler::GetCrtHeight() const
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

bool CGSHandler::GetCrtIsInterlaced() const
{
	SMODE2 smode2;
	smode2 <<= m_nSMODE2;
	return smode2.interlaced;
}

bool CGSHandler::GetCrtIsFrameMode() const
{
	SMODE2 smode2;
	smode2 <<= m_nSMODE2;
	return smode2.ffmd;
}

void CGSHandler::SyncCLUT(const TEX0& tex0)
{
	//Sync clut
	if(tex0.nCLD != 0)
	{
		//assert(IsPsmIDTEX(tex0.nPsm));
		switch(tex0.nPsm)
		{
		case PSMT8:
		case PSMT8H:
			ReadCLUT8(tex0);
			break;
		case PSMT4:
		case PSMT4HH:
		case PSMT4HL:
			ReadCLUT4(tex0);
			break;
		}
	}
}

void CGSHandler::ReadCLUT4(const TEX0& tex0)
{
	bool updateNeeded = false;

	if(tex0.nCLD == 0)
	{
		//No changes to CLUT
	}
	else if(tex0.nCLD == 1)
	{
		updateNeeded = true;
	}
	else if(tex0.nCLD == 2)
	{
		m_nCBP0 = tex0.nCBP;
		updateNeeded = true;
	}
	else
	{
		updateNeeded = true;
		assert(0);
	}

	if(updateNeeded)
	{
		bool changed = false;
		uint32 clutOffset = tex0.nCSA * 16;

		if(tex0.nCSM == 0)
		{
			//CSM1 mode
			if(tex0.nCPSM == PSMCT32)
			{
				assert(tex0.nCSA < 16);

				CGsPixelFormats::CPixelIndexorPSMCT32 Indexor(m_pRAM, tex0.GetCLUTPtr(), 1);
				uint16* pDst = m_pCLUT + clutOffset;

				for(unsigned int j = 0; j < 2; j++)
				{
					for(unsigned int i = 0; i < 8; i++)
					{
						uint32 color = Indexor.GetPixel(i, j);
						uint16 colorLo = static_cast<uint16>(color & 0xFFFF);
						uint16 colorHi = static_cast<uint16>(color >> 16);

						if(
							(pDst[0x000] != colorLo) ||
							(pDst[0x100] != colorHi))
						{
							changed = true;
						}

						pDst[0x000] = colorLo;
						pDst[0x100] = colorHi;
						pDst++;
					}
				}
			}
			else if(tex0.nCPSM == PSMCT16)
			{
				assert(tex0.nCSA < 32);

				CGsPixelFormats::CPixelIndexorPSMCT16 Indexor(m_pRAM, tex0.GetCLUTPtr(), 1);
				uint16* pDst = m_pCLUT + clutOffset;

				for(unsigned int j = 0; j < 2; j++)
				{
					for(unsigned int i = 0; i < 8; i++)
					{
						uint16 color = Indexor.GetPixel(i, j);

						if(*pDst != color)
						{
							changed = true;
						}

						(*pDst++) = color;
					}
				}
			}
			else
			{
				assert(0);
			}
		}
		else
		{
			//CSM2 mode
			assert(tex0.nCPSM == PSMCT16);
			assert(tex0.nCSA == 0);

			TEXCLUT texClut;
			texClut <<= m_nReg[GS_REG_TEXCLUT];

			CGsPixelFormats::CPixelIndexorPSMCT16 Indexor(m_pRAM, tex0.GetCLUTPtr(), texClut.nCBW);
			unsigned int nOffsetX = texClut.GetOffsetU();
			unsigned int nOffsetY = texClut.GetOffsetV();
			uint16* pDst = m_pCLUT;

			for(unsigned int i = 0; i < 0x10; i++)
			{
				uint16 color = Indexor.GetPixel(nOffsetX + i, nOffsetY);

				if(*pDst != color)
				{
					changed = true;
				}

				(*pDst++) = color;
			}
		}

		if(changed)
		{
			ProcessClutTransfer(tex0.nCSA, 0);
		}
	}
}

void CGSHandler::ReadCLUT8(const TEX0& tex0)
{
	assert(tex0.nCSA == 0);
	assert(tex0.nCSM == 0);

	bool updateNeeded = false;

	if(tex0.nCLD == 0)
	{
		//No changes to CLUT
	}
	else if(tex0.nCLD == 1)
	{
		updateNeeded = true;
	}
	else if(tex0.nCLD == 2)
	{
		m_nCBP0 = tex0.nCBP;
		updateNeeded = true;
	}
	else if(tex0.nCLD == 3)
	{
		m_nCBP1 = tex0.nCBP;
		updateNeeded = true;
	}
	else if(tex0.nCLD == 4)
	{
		updateNeeded = m_nCBP0 != tex0.nCBP;
		m_nCBP0 = tex0.nCBP;
	}
	else
	{
		updateNeeded = true;
		assert(0);
	}

	if(updateNeeded)
	{
		bool changed = false;

		if(tex0.nCPSM == PSMCT32)
		{
			CGsPixelFormats::CPixelIndexorPSMCT32 Indexor(m_pRAM, tex0.GetCLUTPtr(), 1);

			for(unsigned int j = 0; j < 16; j++)
			{
				for(unsigned int i = 0; i < 16; i++)
				{
					uint32 color = Indexor.GetPixel(i, j);
					uint16 colorLo = static_cast<uint16>(color & 0xFFFF);
					uint16 colorHi = static_cast<uint16>(color >> 16);

					uint8 index = i + (j * 16);
					index = (index & ~0x18) | ((index & 0x08) << 1) | ((index & 0x10) >> 1);

					if(
						(m_pCLUT[index + 0x000] != colorLo) ||
						(m_pCLUT[index + 0x100] != colorHi))
					{
						changed = true;
					}

					m_pCLUT[index + 0x000] = colorLo;
					m_pCLUT[index + 0x100] = colorHi;
				}
			}
		}
		else if(tex0.nCPSM == PSMCT16)
		{
			CGsPixelFormats::CPixelIndexorPSMCT16 Indexor(m_pRAM, tex0.GetCLUTPtr(), 1);

			for(unsigned int j = 0; j < 16; j++)
			{
				for(unsigned int i = 0; i < 16; i++)
				{
					uint16 color = Indexor.GetPixel(i, j);
					
					uint8 index = i + (j * 16);
					index = (index & ~0x18) | ((index & 0x08) << 1) | ((index & 0x10) >> 1);

					if(m_pCLUT[index] != color)
					{
						changed = true;
					}

					m_pCLUT[index] = color;
				}
			}
		}
		else
		{
			assert(0);
		}

		if(changed)
		{
			ProcessClutTransfer(tex0.nCSA, 0);
		}
	}
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
	case 9:
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

bool CGSHandler::IsPsmIDTEX(unsigned int psm)
{
	return IsPsmIDTEX4(psm) || IsPsmIDTEX8(psm);
}

bool CGSHandler::IsPsmIDTEX4(unsigned int psm)
{
	return psm == PSMT4 || psm == PSMT4HH || psm == PSMT4HL;
}

bool CGSHandler::IsPsmIDTEX8(unsigned int psm)
{
	return psm == PSMT8 || psm == PSMT8H;
}

void CGSHandler::DisassembleWrite(uint8 nRegister, uint64 nData)
{
	//Filtering
	//if(!((nRegister == GS_REG_FRAME_1) || (nRegister == GS_REG_FRAME_2))) return;
	//if(!((nRegister == GS_REG_TEST_1) || (nRegister == GS_REG_TEST_2))) return;

	switch(nRegister)
	{
	case GS_REG_PRIM:
		{
			PRIM pr;
			pr <<= nData;
			CLog::GetInstance().Print(LOG_NAME, "PRIM(PRI: %i, IIP: %i, TME: %i, FGE: %i, ABE: %i, AA1: %i, FST: %i, CTXT: %i, FIX: %i);\r\n", \
				pr.nType, \
				pr.nShading, \
				pr.nTexture, \
				pr.nFog, \
				pr.nAlpha, \
				pr.nAntiAliasing, \
				pr.nUseUV, \
				pr.nContext, \
				pr.nUseFloat);
		}
		break;
	case GS_REG_RGBAQ:
		{
			RGBAQ rgbaq;
			rgbaq <<= nData;
			CLog::GetInstance().Print(LOG_NAME, "RGBAQ(R: 0x%0.2X, G: 0x%0.2X, B: 0x%0.2X, A: 0x%0.2X, Q: %f);\r\n", \
									  rgbaq.nR,
									  rgbaq.nG,
									  rgbaq.nB,
									  rgbaq.nA,
									  rgbaq.nQ);
		}
		break;
	case GS_REG_ST:
		{
			ST st;
			st <<= nData;
			CLog::GetInstance().Print(LOG_NAME, "ST(S: %f, T: %f);\r\n", \
				st.nS, \
				st.nT);
		}
		break;
	case GS_REG_UV:
		{
			UV uv;
			uv <<= nData;
			CLog::GetInstance().Print(LOG_NAME, "UV(U: %f, V: %f);\r\n", \
				uv.GetU(), \
				uv.GetV());
		}
		break;
	case GS_REG_XYZ2:
	case GS_REG_XYZ3:
		{
			XYZ xyz;
			xyz <<= nData;
			CLog::GetInstance().Print(LOG_NAME, "%s(%f, %f, %f);\r\n",
				(nRegister == GS_REG_XYZ2) ? "XYZ2" : "XYZ3",
				xyz.GetX(),
				xyz.GetY(),
				xyz.GetZ());
		}
		break;
	case GS_REG_XYZF2:
	case GS_REG_XYZF3:
		{
			XYZF xyzf;
			xyzf = *reinterpret_cast<XYZF*>(&nData);
			CLog::GetInstance().Print(LOG_NAME, "%s(%f, %f, %i, %i);\r\n",
				(nRegister == GS_REG_XYZF2) ? "XYZF2" : "XYZF3",
				xyzf.GetX(),
				xyzf.GetY(),
				xyzf.nZ,
				xyzf.nF);
		}
		break;
	case GS_REG_TEX0_1:
	case GS_REG_TEX0_2:
		{
			TEX0 tex;
			tex <<= nData;
			CLog::GetInstance().Print(LOG_NAME, "TEX0_%i(TBP: 0x%0.8X, TBW: %i, PSM: %i, TW: %i, TH: %i, TCC: %i, TFX: %i, CBP: 0x%0.8X, CPSM: %i, CSM: %i, CSA: %i, CLD: %i);\r\n", \
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
			clamp <<= nData;
			CLog::GetInstance().Print(LOG_NAME, "CLAMP_%i(WMS: %i, WMT: %i, MINU: %i, MAXU: %i, MINV: %i, MAXV: %i);\r\n", \
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
			TEX1 tex1;
			tex1 <<= nData;
			CLog::GetInstance().Print(LOG_NAME, "TEX1_%i(LCM: %i, MXL: %i, MMAG: %i, MMIN: %i, MTBA: %i, L: %i, K: %i);\r\n", \
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
		CLog::GetInstance().Print(LOG_NAME, "TEX2_%i(PSM: %i, CBP: 0x%0.8X, CPSM: %i, CSM: %i, CSA: %i, CLD: %i);\r\n", \
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
		CLog::GetInstance().Print(LOG_NAME, "XYOFFSET_%i(%i, %i);\r\n", \
			nRegister == GS_REG_XYOFFSET_1 ? 1 : 2, \
			(uint32)((nData >>  0) & 0xFFFFFFFF), \
			(uint32)((nData >> 32) & 0xFFFFFFFF));
		break;
	case GS_REG_PRMODECONT:
		CLog::GetInstance().Print(LOG_NAME, "PRMODECONT(AC = %i);\r\n", \
			nData & 1);
		break;
	case GS_REG_PRMODE:
		{
			PRMODE prm;
			prm <<= nData;
			CLog::GetInstance().Print(LOG_NAME, "PRMODE(IIP: %i, TME: %i, FGE: %i, ABE: %i, AA1: %i, FST: %i, CTXT: %i, FIX: %i);\r\n", \
									  prm.nShading, \
									  prm.nTexture, \
									  prm.nFog, \
									  prm.nAlpha, \
									  prm.nAntiAliasing, \
									  prm.nUseUV, \
									  prm.nContext, \
									  prm.nUseFloat);
		}
		break;
	case GS_REG_TEXCLUT:
		TEXCLUT clut;
		clut = *(TEXCLUT*)&nData;
		CLog::GetInstance().Print(LOG_NAME, "TEXCLUT(CBW: %i, COU: %i, COV: %i);\r\n", \
			clut.nCBW,
			clut.nCOU,
			clut.nCOV);
		break;
	case GS_REG_FOGCOL:
		FOGCOL fogcol;
		fogcol = *(FOGCOL*)&nData;
		CLog::GetInstance().Print(LOG_NAME, "FOGCOL(R: 0x%0.2X, G: 0x%0.2X, B: 0x%0.2X);\r\n", \
			fogcol.nFCR,
			fogcol.nFCG,
			fogcol.nFCB);
		break;
	case GS_REG_TEXA:
		{
			TEXA TexA;
			TexA <<= nData;
			CLog::GetInstance().Print(LOG_NAME, "TEXA(TA0: 0x%0.2X, AEM: %i, TA1: 0x%0.2X);\r\n", \
									  TexA.nTA0,
									  TexA.nAEM,
									  TexA.nTA1);
		}
		break;
	case GS_REG_TEXFLUSH:
		CLog::GetInstance().Print(LOG_NAME, "TEXFLUSH();\r\n");
		break;
	case GS_REG_ALPHA_1:
	case GS_REG_ALPHA_2:
		{
			ALPHA alpha;
			alpha <<= nData;
			CLog::GetInstance().Print(LOG_NAME, "ALPHA_%i(A: %i, B: %i, C: %i, D: %i, FIX: 0x%0.2X);\r\n", \
									  nRegister == GS_REG_ALPHA_1 ? 1 : 2, \
									  alpha.nA, \
									  alpha.nB, \
									  alpha.nC, \
									  alpha.nD, \
									  alpha.nFix);
		}
		break;
	case GS_REG_SCISSOR_1:
	case GS_REG_SCISSOR_2:
		{
			SCISSOR scissor;
			scissor <<= nData;
			CLog::GetInstance().Print(LOG_NAME, "SCISSOR_%i(scax0: %i, scax1: %i, scay0: %i, scay1: %i);\r\n", 
				nRegister == GS_REG_SCISSOR_1 ? 1 : 2,
				scissor.scax0,
				scissor.scax1,
				scissor.scay0,
				scissor.scay1);
			break;
		}
	case GS_REG_TEST_1:
	case GS_REG_TEST_2:
		{
			TEST tst;
			tst <<= nData;
			CLog::GetInstance().Print(LOG_NAME, "TEST_%i(ATE: %i, ATST: %i, AREF: 0x%0.2X, AFAIL: %i, DATE: %i, DATM: %i, ZTE: %i, ZTST: %i);\r\n", \
									  nRegister == GS_REG_TEST_1 ? 1 : 2, \
									  tst.nAlphaEnabled, \
									  tst.nAlphaMethod, \
									  tst.nAlphaRef, \
									  tst.nAlphaFail, \
									  tst.nDestAlphaEnabled, \
									  tst.nDestAlphaMode, \
									  tst.nDepthEnabled, \
									  tst.nDepthMethod);
		}
		break;
	case GS_REG_FRAME_1:
	case GS_REG_FRAME_2:
		{
			FRAME fr;
			fr <<= nData;
			CLog::GetInstance().Print(LOG_NAME, "FRAME_%i(FBP: 0x%0.8X, FBW: %d, PSM: %d, FBMSK: 0x%0.8X);\r\n", \
				nRegister == GS_REG_FRAME_1 ? 1 : 2, \
				fr.GetBasePtr(), \
				fr.GetWidth(), \
				fr.nPsm, \
				fr.nMask);
		}
		break;
	case GS_REG_ZBUF_1:
	case GS_REG_ZBUF_2:
		{
			ZBUF zbuf;
			zbuf <<= nData;
			CLog::GetInstance().Print(LOG_NAME, "ZBUF_%i(ZBP: 0x%0.8X, PSM: %i, ZMSK: %i);\r\n", \
									  nRegister == GS_REG_ZBUF_1 ? 1 : 2, \
									  zbuf.GetBasePtr(), \
									  zbuf.nPsm, \
									  zbuf.nMask);
		}
		break;
	case GS_REG_BITBLTBUF:
		{
			BITBLTBUF buf;
			buf <<= nData;
			CLog::GetInstance().Print(LOG_NAME, "BITBLTBUF(SBP: 0x%0.8X, SBW: %i, SPSM: %i, DBP: 0x%0.8X, DBW: %i, DPSM: %i);\r\n",
				buf.GetSrcPtr(),
				buf.GetSrcWidth(),
				buf.nSrcPsm,
				buf.GetDstPtr(),
				buf.GetDstWidth(),
				buf.nDstPsm);
		}
		break;
	case GS_REG_TRXPOS:
		{
			TRXPOS trxPos;
			trxPos <<= nData;
			CLog::GetInstance().Print(LOG_NAME, "TRXPOS(SSAX: %i, SSAY: %i, DSAX: %i, DSAY: %i, DIR: %i);\r\n",
				trxPos.nSSAX, trxPos.nSSAY, trxPos.nDSAX, trxPos.nDSAY, trxPos.nDIR);
		}
		break;
	case GS_REG_TRXREG:
		{
			TRXREG trxReg;
			trxReg <<= nData;
			CLog::GetInstance().Print(LOG_NAME, "TRXREG(RRW: %i, RRH: %i);\r\n",
				trxReg.nRRW, trxReg.nRRH);
		}
		break;
	case GS_REG_TRXDIR:
		CLog::GetInstance().Print(LOG_NAME, "TRXDIR(XDIR: %i);\r\n", nData & 0x03);
		break;
	case GS_REG_FINISH:
		CLog::GetInstance().Print(LOG_NAME, "FINISH();\r\n");
		break;
	default:
		CLog::GetInstance().Print(LOG_NAME, "Unknown command (0x%X).\r\n", nRegister); 
		break;
	}
}

void CGSHandler::DisassemblePrivWrite(uint32 address)
{
	assert((address & 0x04) != 0);

	uint32 regAddress = address & ~0x0F;
	switch(regAddress)
	{
	case GS_PMODE:
		CLog::GetInstance().Print(LOG_NAME, "PMODE(0x%0.8X);\r\n", m_nPMODE);
		break;
	case GS_SMODE2:
		{
			SMODE2 smode2;
			smode2 <<= m_nSMODE2;
			CLog::GetInstance().Print(LOG_NAME, "SMODE2(inter = %d, ffmd = %d, dpms = %d);\r\n",
				smode2.interlaced,
				smode2.ffmd,
				smode2.dpms);
		}
		break;
	case GS_DISPFB1:
	case GS_DISPFB2:
		{
			DISPFB dispfb;
			dispfb <<= (regAddress == GS_DISPFB1) ? m_nDISPFB1.value.q : m_nDISPFB2.value.q;
			CLog::GetInstance().Print(LOG_NAME, "DISPFB%d(FBP: 0x%0.8X, FBW: %d, PSM: %d, DBX: %d, DBY: %d);\r\n",
				(regAddress == GS_DISPFB1) ? 1 : 2,
				dispfb.GetBufPtr(),
				dispfb.GetBufWidth(),
				dispfb.nPSM,
				dispfb.nX,
				dispfb.nY);
		}
		break;
	case GS_DISPLAY1:
	case GS_DISPLAY2:
		{
			DISPLAY display;
			display <<= (regAddress == GS_DISPLAY1) ? m_nDISPLAY1.value.q : m_nDISPLAY2.value.q;
			CLog::GetInstance().Print(LOG_NAME, "DISPLAY%d(DX: %d, DY: %d, MAGH: %d, MAGV: %d, DW: %d, DH: %d);\r\n",
				(regAddress == GS_DISPLAY1) ? 1 : 2,
				display.nX,
				display.nY,
				display.nMagX,
				display.nMagY,
				display.nW,
				display.nH);
		}
		break;
	case GS_CSR:
		//CSR
		break;
	case GS_IMR:
		//IMR
		break;
	}
}

void CGSHandler::LoadSettings()
{
	m_flipMode = static_cast<FLIP_MODE>(CAppConfig::GetInstance().GetPreferenceInteger(PREF_CGSHANDLER_FLIPMODE));
}

void CGSHandler::WriteToDelayedRegister(uint32 address, uint32 value, DELAYED_REGISTER& delayedRegister)
{
	if(address & 0x04)
	{
		std::lock_guard<std::recursive_mutex> registerMutexLock(m_registerMutex);
		delayedRegister.value.d0 = delayedRegister.heldValue;
		delayedRegister.value.d1 = value;
	}
	else
	{
		delayedRegister.heldValue = value;
	}
}

void CGSHandler::ThreadProc()
{
	while(!m_threadDone)
	{
		m_mailBox.WaitForCall(100);
		while(m_mailBox.IsPending())
		{
			m_mailBox.ReceiveCall();
		}
	}
}
