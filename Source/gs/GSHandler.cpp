#include <stdio.h>
#include <string.h>
#include <functional>
#include <boost/scoped_array.hpp>
#include "../AppConfig.h"
#include "../Log.h"
#include "../MemoryStateFile.h"
#include "../RegisterStateFile.h"
#include "../FrameDump.h"
#include "GSHandler.h"
#include "GsPixelFormats.h"
#include "string_format.h"

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

struct MASSIVEWRITE_INFO
{
#ifdef DEBUGGER_INCLUDED
	CGsPacketMetadata				metadata;
#endif
	unsigned int					count;
	CGSHandler::RegisterWrite		writes[1];
};

CGSHandler::CGSHandler()
: m_threadDone(false)
, m_drawCallCount(0)
, m_pCLUT(nullptr)
, m_pRAM(nullptr)
, m_frameDump(nullptr)
, m_loggingEnabled(true)
{
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
	m_nCBP0 = 0;
	m_nCBP1 = 0;
	m_transferCount = 0;
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
	archive.InsertFile(new CMemoryStateFile(STATE_REGS,		m_nReg,		sizeof(uint64) * CGSHandler::REGISTER_MAX));
	archive.InsertFile(new CMemoryStateFile(STATE_TRXCTX,	&m_trxCtx,	sizeof(TRXCONTEXT)));

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
	archive.BeginReadFile(STATE_TRXCTX	)->Read(&m_trxCtx,	sizeof(TRXCONTEXT));

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

void CGSHandler::SetFrameDump(CFrameDump* frameDump)
{
	m_frameDump = frameDump;
}

bool CGSHandler::GetDrawEnabled() const
{
	return m_drawEnabled;
}

void CGSHandler::SetDrawEnabled(bool drawEnabled)
{
	m_drawEnabled = drawEnabled;
}

void CGSHandler::SetVBlank()
{
	{
		Flip();
	}

	std::lock_guard<std::recursive_mutex> registerMutexLock(m_registerMutex);
	m_nCSR |= CSR_VSYNC_INT;
}

void CGSHandler::ResetVBlank()
{
	std::lock_guard<std::recursive_mutex> registerMutexLock(m_registerMutex);

	//Alternate current field
	m_nCSR ^= CSR_FIELD;
}

int CGSHandler::GetPendingTransferCount() const
{
	return m_transferCount;
}

bool CGSHandler::IsInterruptPending()
{
	uint32 mask = (~m_nIMR >> 8) & 0x1F;
	return (m_nCSR & mask) != 0;
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
		W_REG(nAddress, nData, m_nSMODE2);
		break;
	case GS_DISPFB1:
		WriteToDelayedRegister(nAddress, nData, m_nDISPFB1);
		break;
	case GS_DISPLAY1:
		WriteToDelayedRegister(nAddress, nData, m_nDISPLAY1);
		break;
	case GS_DISPFB2:
		WriteToDelayedRegister(nAddress, nData, m_nDISPFB2);
		break;
	case GS_DISPLAY2:
		WriteToDelayedRegister(nAddress, nData, m_nDISPLAY2);
		break;
	case GS_CSR:
		{
			if(!(nAddress & 0x04))
			{
				std::lock_guard<std::recursive_mutex> registerMutexLock(m_registerMutex);
				if(nData & CSR_SIGNAL_EVENT)
				{
					m_nCSR &= ~CSR_SIGNAL_EVENT;
				}
				if(nData & CSR_FINISH_EVENT)
				{
					m_nCSR &= ~CSR_FINISH_EVENT;
				}
				if(nData & CSR_VSYNC_INT)
				{
					m_nCSR &= ~CSR_VSYNC_INT;
				}
				if(nData & CSR_RESET)
				{
					m_nCSR |= CSR_RESET;
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
		LogPrivateWrite(nAddress);
	}
#endif
}

void CGSHandler::Initialize()
{
	m_mailBox.SendCall(std::bind(&CGSHandler::InitializeImpl, this), true);
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
	m_mailBox.SendCall(std::bind(&CGSHandler::FlipImpl, this), true);
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

uint8* CGSHandler::GetRam()
{
	return m_pRAM;
}

uint64* CGSHandler::GetRegisters()
{
	return m_nReg;
}

uint64 CGSHandler::GetSMODE2() const
{
	return m_nSMODE2;
}

void CGSHandler::SetSMODE2(uint64 value)
{
	m_nSMODE2 = value;
}

void CGSHandler::WriteRegister(uint8 registerId, uint64 value)
{
	m_mailBox.SendCall(std::bind(&CGSHandler::WriteRegisterImpl, this, registerId, value));
}

void CGSHandler::FeedImageData(const void* data, uint32 length)
{
	m_transferCount++;

	uint8* buffer = new uint8[length];
	memcpy(buffer, data, length);
	m_mailBox.SendCall(std::bind(&CGSHandler::FeedImageDataImpl, this, buffer, length));
}

void CGSHandler::WriteRegisterMassively(const RegisterWrite* writeList, unsigned int count, const CGsPacketMetadata* metadata)
{
	m_transferCount++;

	auto massiveWrite = reinterpret_cast<MASSIVEWRITE_INFO*>(malloc(sizeof(MASSIVEWRITE_INFO) + (count * sizeof(RegisterWrite))));
	memcpy(massiveWrite->writes, writeList, sizeof(CGSHandler::RegisterWrite) * count);
	massiveWrite->count = count;
#ifdef DEBUGGER_INCLUDED
	if(metadata != nullptr)
	{
		memcpy(&massiveWrite->metadata, metadata, sizeof(CGsPacketMetadata));
	}
	else
	{
		massiveWrite->metadata = CGsPacketMetadata();
	}
#endif
	//Bind is used here because using a lambda seems to cause problems on Android/clang
	m_mailBox.SendCall(std::bind(&CGSHandler::WriteRegisterMassivelyImpl, this, massiveWrite));
}

void CGSHandler::WriteRegisterImpl(uint8 nRegister, uint64 nData)
{
	assert(nRegister < REGISTER_MAX);
	if(nRegister < REGISTER_MAX)
	{
		m_nReg[nRegister] = nData;
	}

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

	case GS_REG_SIGNAL:
		{
			std::lock_guard<std::recursive_mutex> registerMutexLock(m_registerMutex);
			//TODO: Update SIGLBLID
			m_nCSR |= CSR_SIGNAL_EVENT;
		}
		break;

	case GS_REG_FINISH:
		{
			std::lock_guard<std::recursive_mutex> registerMutexLock(m_registerMutex);
			m_nCSR |= CSR_FINISH_EVENT;
		}
		break;
	}

#ifdef _DEBUG
	LogWrite(nRegister, nData);
#endif
}

void CGSHandler::FeedImageDataImpl(void* pData, uint32 nLength)
{
	boost::scoped_array<uint8> dataPtr(reinterpret_cast<uint8*>(pData));

	if(m_trxCtx.nSize == 0)
	{
#ifdef _DEBUG
		CLog::GetInstance().Print(LOG_NAME, "Warning. Received image data when no transfer was expected.\r\n");
#endif
	}
	else
	{
		if(m_trxCtx.nSize < nLength)
		{
			nLength = m_trxCtx.nSize;
			//assert(0);
			//return;
		}

		auto bltBuf = make_convertible<BITBLTBUF>(m_nReg[GS_REG_BITBLTBUF]);

		m_trxCtx.nDirty |= ((this)->*(m_pTransferHandler[bltBuf.nDstPsm]))(pData, nLength);

		m_trxCtx.nSize -= nLength;

		if(m_trxCtx.nSize == 0)
		{
			auto trxReg = make_convertible<TRXREG>(m_nReg[GS_REG_TRXREG]);
			assert(m_trxCtx.nRRY == trxReg.nRRH);
			ProcessImageTransfer();

#ifdef _DEBUG
			CLog::GetInstance().Print(LOG_NAME, "Completed image transfer at 0x%0.8X (dirty = %d).\r\n", bltBuf.GetDstPtr(), m_trxCtx.nDirty);
#endif
		}
	}

	assert(m_transferCount != 0);
	m_transferCount--;
}

void CGSHandler::WriteRegisterMassivelyImpl(MASSIVEWRITE_INFO* massiveWrite)
{
#ifdef DEBUGGER_INCLUDED
	if(m_frameDump)
	{
		m_frameDump->AddPacket(massiveWrite->writes, massiveWrite->count, &massiveWrite->metadata);
	}
#endif

	const RegisterWrite* writeIterator = massiveWrite->writes;
	for(unsigned int i = 0; i < massiveWrite->count; i++)
	{
		WriteRegisterImpl(writeIterator->first, writeIterator->second);
		writeIterator++;
	}
	free(massiveWrite);

	assert(m_transferCount != 0);
	m_transferCount--;
}

void CGSHandler::BeginTransfer()
{
	uint32 trxDir = m_nReg[GS_REG_TRXDIR] & 0x03;
	if(trxDir == 0)
	{
		//From Host to Local
		auto bltBuf = make_convertible<BITBLTBUF>(m_nReg[GS_REG_BITBLTBUF]);
		auto trxReg = make_convertible<TRXREG>(m_nReg[GS_REG_TRXREG]);
		unsigned int nPixelSize = 0;

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

		m_trxCtx.nSize	= (trxReg.nRRW * trxReg.nRRH * nPixelSize) / 8;
		m_trxCtx.nRealSize = m_trxCtx.nSize;
		m_trxCtx.nRRX	= 0;
		m_trxCtx.nRRY	= 0;
		m_trxCtx.nDirty	= false;
	}
	else if(trxDir == 2)
	{
		//Local to Local
		ProcessLocalToLocalTransfer();
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
	auto trxPos = make_convertible<TRXPOS>(m_nReg[GS_REG_TRXPOS]);
	auto trxReg = make_convertible<TRXREG>(m_nReg[GS_REG_TRXREG]);
	auto trxBuf = make_convertible<BITBLTBUF>(m_nReg[GS_REG_BITBLTBUF]);

	nLength /= sizeof(typename Storage::Unit);

	CGsPixelFormats::CPixelIndexor<Storage> Indexor(m_pRAM, trxBuf.GetDstPtr(), trxBuf.nDstWidth);

	auto pSrc = reinterpret_cast<typename Storage::Unit*>(pData);

	for(unsigned int i = 0; i < nLength; i++)
	{
		uint32 nX = (m_trxCtx.nRRX + trxPos.nDSAX) % 2048;
		uint32 nY = (m_trxCtx.nRRY + trxPos.nDSAY) % 2048;

		auto pPixel = Indexor.GetPixelAddress(nX, nY);

		if((*pPixel) != pSrc[i])
		{
			(*pPixel) = pSrc[i];
			nDirty = true;
		}

		m_trxCtx.nRRX++;
		if(m_trxCtx.nRRX == trxReg.nRRW)
		{
			m_trxCtx.nRRX = 0;
			m_trxCtx.nRRY++;
		}
	}

	return nDirty;
}

bool CGSHandler::TrxHandlerPSMCT24(void* pData, uint32 nLength)
{
	auto trxPos = make_convertible<TRXPOS>(m_nReg[GS_REG_TRXPOS]);
	auto trxReg = make_convertible<TRXREG>(m_nReg[GS_REG_TRXREG]);
	auto trxBuf = make_convertible<BITBLTBUF>(m_nReg[GS_REG_BITBLTBUF]);

	CGsPixelFormats::CPixelIndexorPSMCT32 Indexor(m_pRAM, trxBuf.GetDstPtr(), trxBuf.nDstWidth);

	uint8* pSrc = (uint8*)pData;

	for(unsigned int i = 0; i < nLength; i += 3)
	{
		uint32 nX = (m_trxCtx.nRRX + trxPos.nDSAX) % 2048;
		uint32 nY = (m_trxCtx.nRRY + trxPos.nDSAY) % 2048;

		uint32* pDstPixel = Indexor.GetPixelAddress(nX, nY);
		uint32 nSrcPixel = (*(uint32*)&pSrc[i]) & 0x00FFFFFF;
		(*pDstPixel) &= 0xFF000000;
		(*pDstPixel) |= nSrcPixel;

		m_trxCtx.nRRX++;
		if(m_trxCtx.nRRX == trxReg.nRRW)
		{
			m_trxCtx.nRRX = 0;
			m_trxCtx.nRRY++;
		}
	}

	return true;
}

bool CGSHandler::TrxHandlerPSMT4(void* pData, uint32 nLength)
{
	bool dirty = false;
	auto trxPos = make_convertible<TRXPOS>(m_nReg[GS_REG_TRXPOS]);
	auto trxReg = make_convertible<TRXREG>(m_nReg[GS_REG_TRXREG]);
	auto trxBuf = make_convertible<BITBLTBUF>(m_nReg[GS_REG_BITBLTBUF]);

	CGsPixelFormats::CPixelIndexorPSMT4 Indexor(m_pRAM, trxBuf.GetDstPtr(), trxBuf.nDstWidth);

	uint8* pSrc = (uint8*)pData;

	for(unsigned int i = 0; i < nLength; i++)
	{
		uint8 nPixel[2];

		nPixel[0] = (pSrc[i] >> 0) & 0x0F;
		nPixel[1] = (pSrc[i] >> 4) & 0x0F;

		for(unsigned int j = 0; j < 2; j++)
		{
			uint32 nX = (m_trxCtx.nRRX + trxPos.nDSAX) % 2048;
			uint32 nY = (m_trxCtx.nRRY + trxPos.nDSAY) % 2048;

			uint8 currentPixel = Indexor.GetPixel(nX, nY);
			if(currentPixel != nPixel[j])
			{
				Indexor.SetPixel(nX, nY, nPixel[j]);
				dirty = true;
			}

			m_trxCtx.nRRX++;
			if(m_trxCtx.nRRX == trxReg.nRRW)
			{
				m_trxCtx.nRRX = 0;
				m_trxCtx.nRRY++;
			}
		}
	}

	return dirty;
}

template <uint32 nShift, uint32 nMask>
bool CGSHandler::TrxHandlerPSMT4H(void* pData, uint32 nLength)
{
	auto trxPos = make_convertible<TRXPOS>(m_nReg[GS_REG_TRXPOS]);
	auto trxReg = make_convertible<TRXREG>(m_nReg[GS_REG_TRXREG]);
	auto trxBuf = make_convertible<BITBLTBUF>(m_nReg[GS_REG_BITBLTBUF]);

	CGsPixelFormats::CPixelIndexorPSMCT32 Indexor(m_pRAM, trxBuf.GetDstPtr(), trxBuf.nDstWidth);

	uint8* pSrc = reinterpret_cast<uint8*>(pData);

	for(unsigned int i = 0; i < nLength; i++)
	{
		//Pixel 1
		uint32 nX = (m_trxCtx.nRRX + trxPos.nDSAX) % 2048;
		uint32 nY = (m_trxCtx.nRRY + trxPos.nDSAY) % 2048;

		uint8 nSrcPixel = pSrc[i] & 0x0F;

		uint32* pDstPixel = Indexor.GetPixelAddress(nX, nY);
		(*pDstPixel) &= ~nMask;
		(*pDstPixel) |= (nSrcPixel << nShift);

		m_trxCtx.nRRX++;
		if(m_trxCtx.nRRX == trxReg.nRRW)
		{
			m_trxCtx.nRRX = 0;
			m_trxCtx.nRRY++;
		}

		//Pixel 2
		nX = (m_trxCtx.nRRX + trxPos.nDSAX) % 2048;
		nY = (m_trxCtx.nRRY + trxPos.nDSAY) % 2048;

		nSrcPixel = (pSrc[i] & 0xF0);

		pDstPixel = Indexor.GetPixelAddress(nX, nY);
		(*pDstPixel) &= ~nMask;
		(*pDstPixel) |= (nSrcPixel << (nShift - 4));

		m_trxCtx.nRRX++;
		if(m_trxCtx.nRRX == trxReg.nRRW)
		{
			m_trxCtx.nRRX = 0;
			m_trxCtx.nRRY++;
		}
	}

	return true;
}

bool CGSHandler::TrxHandlerPSMT8H(void* pData, uint32 nLength)
{
	auto trxPos = make_convertible<TRXPOS>(m_nReg[GS_REG_TRXPOS]);
	auto trxReg = make_convertible<TRXREG>(m_nReg[GS_REG_TRXREG]);
	auto trxBuf = make_convertible<BITBLTBUF>(m_nReg[GS_REG_BITBLTBUF]);

	CGsPixelFormats::CPixelIndexorPSMCT32 Indexor(m_pRAM, trxBuf.GetDstPtr(), trxBuf.nDstWidth);

	uint8* pSrc = reinterpret_cast<uint8*>(pData);

	for(unsigned int i = 0; i < nLength; i++)
	{
		uint32 nX = (m_trxCtx.nRRX + trxPos.nDSAX) % 2048;
		uint32 nY = (m_trxCtx.nRRY + trxPos.nDSAY) % 2048;

		uint8 nSrcPixel = pSrc[i];

		uint32* pDstPixel = Indexor.GetPixelAddress(nX, nY);
		(*pDstPixel) &= ~0xFF000000;
		(*pDstPixel) |= (nSrcPixel << 24);

		m_trxCtx.nRRX++;
		if(m_trxCtx.nRRX == trxReg.nRRW)
		{
			m_trxCtx.nRRX = 0;
			m_trxCtx.nRRY++;
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
	else if(tex0.nCLD == 4)
	{
		updateNeeded = (m_nCBP0 != tex0.nCBP);
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
		uint32 clutOffset = tex0.nCSA * 16;

		if(tex0.nCSM == 0)
		{
			//CSM1 mode
			if(tex0.nCPSM == PSMCT32 || tex0.nCPSM == PSMCT24)
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

		if(tex0.nCPSM == PSMCT32 || tex0.nCPSM == PSMCT24)
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

void CGSHandler::SetLoggingEnabled(bool loggingEnabled)
{
	m_loggingEnabled = loggingEnabled;
}

std::string CGSHandler::DisassembleWrite(uint8 registerId, uint64 data)
{
	std::string result;

	switch(registerId)
	{
	case GS_REG_PRIM:
		{
			auto pr = make_convertible<PRIM>(data);
			result = string_format("PRIM(PRI: %i, IIP: %i, TME: %i, FGE: %i, ABE: %i, AA1: %i, FST: %i, CTXT: %i, FIX: %i)",
				pr.nType, pr.nShading, pr.nTexture, pr.nFog, pr.nAlpha, pr.nAntiAliasing, pr.nUseUV, pr.nContext, pr.nUseFloat);
		}
		break;
	case GS_REG_RGBAQ:
		{
			auto rgbaq = make_convertible<RGBAQ>(data);
			result = string_format("RGBAQ(R: 0x%0.2X, G: 0x%0.2X, B: 0x%0.2X, A: 0x%0.2X, Q: %f)",
				rgbaq.nR, rgbaq.nG, rgbaq.nB, rgbaq.nA, rgbaq.nQ);
		}
		break;
	case GS_REG_ST:
		{
			auto st = make_convertible<ST>(data);
			result = string_format("ST(S: %f, T: %f)",
				st.nS, st.nT);
		}
		break;
	case GS_REG_UV:
		{
			auto uv = make_convertible<UV>(data);
			result = string_format("UV(U: %f, V: %f)",
				uv.GetU(), uv.GetV());
		}
		break;
	case GS_REG_XYZ2:
	case GS_REG_XYZ3:
		{
			auto xyz = make_convertible<XYZ>(data);
			result = string_format("%s(%f, %f, %f)",
				(registerId == GS_REG_XYZ2) ? "XYZ2" : "XYZ3", xyz.GetX(), xyz.GetY(), xyz.GetZ());
		}
		break;
	case GS_REG_XYZF2:
	case GS_REG_XYZF3:
		{
			auto xyzf = make_convertible<XYZF>(data);
			result = string_format("%s(%f, %f, %i, %i)",
				(registerId == GS_REG_XYZF2) ? "XYZF2" : "XYZF3", xyzf.GetX(), xyzf.GetY(), xyzf.nZ, xyzf.nF);
		}
		break;
	case GS_REG_TEX0_1:
	case GS_REG_TEX0_2:
		{
			auto tex0 = make_convertible<TEX0>(data);
			result = string_format("TEX0_%i(TBP: 0x%0.8X, TBW: %i, PSM: %i, TW: %i, TH: %i, TCC: %i, TFX: %i, CBP: 0x%0.8X, CPSM: %i, CSM: %i, CSA: %i, CLD: %i)",
				(registerId == GS_REG_TEX0_1) ? 1 : 2, tex0.GetBufPtr(), tex0.GetBufWidth(), tex0.nPsm, tex0.GetWidth(), tex0.GetHeight(), tex0.nColorComp,
				tex0.nFunction, tex0.GetCLUTPtr(), tex0.nCPSM, tex0.nCSM, tex0.nCSA, tex0.nCLD);
		}
		break;
	case GS_REG_CLAMP_1:
	case GS_REG_CLAMP_2:
		{
			auto clamp = make_convertible<CLAMP>(data);
			result = string_format("CLAMP_%i(WMS: %i, WMT: %i, MINU: %i, MAXU: %i, MINV: %i, MAXV: %i)",
				(registerId == GS_REG_CLAMP_1 ? 1 : 2), clamp.nWMS, clamp.nWMT, clamp.GetMinU(), clamp.GetMaxU(), clamp.GetMinV(), clamp.GetMaxV());
		}
		break;
	case GS_REG_TEX1_1:
	case GS_REG_TEX1_2:
		{
			auto tex1 = make_convertible<TEX1>(data);
			result = string_format("TEX1_%i(LCM: %i, MXL: %i, MMAG: %i, MMIN: %i, MTBA: %i, L: %i, K: %i)",
				(registerId == GS_REG_TEX1_1) ? 1 : 2, tex1.nLODMethod, tex1.nMaxMip, tex1.nMagFilter, tex1.nMinFilter, tex1.nMipBaseAddr, tex1.nLODL, tex1.nLODK);
		}
		break;
	case GS_REG_TEX2_1:
	case GS_REG_TEX2_2:
		{
			auto tex2 = make_convertible<TEX2>(data);
			result = string_format("TEX2_%i(PSM: %i, CBP: 0x%0.8X, CPSM: %i, CSM: %i, CSA: %i, CLD: %i)",
				(registerId == GS_REG_TEX2_1) ? 1 : 2, tex2.nPsm, tex2.GetCLUTPtr(), tex2.nCPSM, tex2.nCSM, tex2.nCSA, tex2.nCLD);
		}
		break;
	case GS_REG_XYOFFSET_1:
	case GS_REG_XYOFFSET_2:
		result = string_format("XYOFFSET_%i(%i, %i)",
			(registerId == GS_REG_XYOFFSET_1) ? 1 : 2, static_cast<uint32>(data >> 0), static_cast<uint32>(data >> 32));
		break;
	case GS_REG_PRMODECONT:
		result = string_format("PRMODECONT(AC: %i)", data & 1);
		break;
	case GS_REG_PRMODE:
		{
			auto prm = make_convertible<PRMODE>(data);
			result = string_format("PRMODE(IIP: %i, TME: %i, FGE: %i, ABE: %i, AA1: %i, FST: %i, CTXT: %i, FIX: %i)",
				prm.nShading, prm.nTexture, prm.nFog, prm.nAlpha, prm.nAntiAliasing, prm.nUseUV, prm.nContext, prm.nUseFloat);
		}
		break;
	case GS_REG_TEXCLUT:
		{
			auto clut = make_convertible<TEXCLUT>(data);
			result = string_format("TEXCLUT(CBW: %i, COU: %i, COV: %i)",
				clut.nCBW, clut.nCOU, clut.nCOV);
		}
		break;
	case GS_REG_FOGCOL:
		{
			auto fogcol = make_convertible<FOGCOL>(data);
			result = string_format("FOGCOL(R: 0x%0.2X, G: 0x%0.2X, B: 0x%0.2X)",
				fogcol.nFCR, fogcol.nFCG, fogcol.nFCB);
		}
		break;
	case GS_REG_TEXA:
		{
			auto texa = make_convertible<TEXA>(data);
			result = string_format("TEXA(TA0: 0x%0.2X, AEM: %i, TA1: 0x%0.2X)",
				texa.nTA0, texa.nAEM, texa.nTA1);
		}
		break;
	case GS_REG_TEXFLUSH:
		result = "TEXFLUSH()";
		break;
	case GS_REG_ALPHA_1:
	case GS_REG_ALPHA_2:
		{
			auto alpha = make_convertible<ALPHA>(data);
			result = string_format("ALPHA_%i(A: %i, B: %i, C: %i, D: %i, FIX: 0x%0.2X)",
				(registerId == GS_REG_ALPHA_1) ? 1 : 2, alpha.nA, alpha.nB, alpha.nC, alpha.nD, alpha.nFix);
		}
		break;
	case GS_REG_SCISSOR_1:
	case GS_REG_SCISSOR_2:
		{
			auto scissor = make_convertible<SCISSOR>(data);
			result = string_format("SCISSOR_%i(SCAX0: %i, SCAX1: %i, SCAY0: %i, SCAY1: %i)",
				(registerId == GS_REG_SCISSOR_1) ? 1 : 2, scissor.scax0, scissor.scax1, scissor.scay0, scissor.scay1);
		}
		break;
	case GS_REG_TEST_1:
	case GS_REG_TEST_2:
		{
			auto tst = make_convertible<TEST>(data);
			result = string_format("TEST_%i(ATE: %i, ATST: %i, AREF: 0x%0.2X, AFAIL: %i, DATE: %i, DATM: %i, ZTE: %i, ZTST: %i)",
				(registerId == GS_REG_TEST_1) ? 1 : 2, tst.nAlphaEnabled, tst.nAlphaMethod, tst.nAlphaRef, tst.nAlphaFail,
				tst.nDestAlphaEnabled, tst.nDestAlphaMode, tst.nDepthEnabled, tst.nDepthMethod);
		}
		break;
	case GS_REG_PABE:
		{
			auto value = static_cast<uint8>(data & 1);
			result = string_format("PABE(PABE: %d)", value);
		}
		break;
	case GS_REG_FBA_1:
	case GS_REG_FBA_2:
		{
			auto value = static_cast<uint8>(data & 1);
			result = string_format("FBA_%d(FBA: %d)", (registerId == GS_REG_FBA_1) ? 1 : 2, value);
		}
		break;
	case GS_REG_FRAME_1:
	case GS_REG_FRAME_2:
		{
			auto fr = make_convertible<FRAME>(data);
			result = string_format("FRAME_%i(FBP: 0x%0.8X, FBW: %d, PSM: %d, FBMSK: 0x%0.8X)",
				(registerId == GS_REG_FRAME_1) ? 1 : 2, fr.GetBasePtr(), fr.GetWidth(), fr.nPsm, fr.nMask);
		}
		break;
	case GS_REG_ZBUF_1:
	case GS_REG_ZBUF_2:
		{
			auto zbuf = make_convertible<ZBUF>(data);
			result = string_format("ZBUF_%i(ZBP: 0x%0.8X, PSM: %i, ZMSK: %i)",
				(registerId == GS_REG_ZBUF_1) ? 1 : 2, zbuf.GetBasePtr(), zbuf.nPsm, zbuf.nMask);
		}
		break;
	case GS_REG_BITBLTBUF:
		{
			auto buf = make_convertible<BITBLTBUF>(data);
			result = string_format("BITBLTBUF(SBP: 0x%0.8X, SBW: %i, SPSM: %i, DBP: 0x%0.8X, DBW: %i, DPSM: %i)",
				buf.GetSrcPtr(), buf.GetSrcWidth(), buf.nSrcPsm, buf.GetDstPtr(), buf.GetDstWidth(), buf.nDstPsm);
		}
		break;
	case GS_REG_TRXPOS:
		{
			auto trxPos = make_convertible<TRXPOS>(data);
			result = string_format("TRXPOS(SSAX: %i, SSAY: %i, DSAX: %i, DSAY: %i, DIR: %i)",
				trxPos.nSSAX, trxPos.nSSAY, trxPos.nDSAX, trxPos.nDSAY, trxPos.nDIR);
		}
		break;
	case GS_REG_TRXREG:
		{
			auto trxReg = make_convertible<TRXREG>(data);
			result = string_format("TRXREG(RRW: %i, RRH: %i)",
				trxReg.nRRW, trxReg.nRRH);
		}
		break;
	case GS_REG_TRXDIR:
		result = string_format("TRXDIR(XDIR: %i)", data & 0x03);
		break;
	case GS_REG_SIGNAL:
		result = string_format("SIGNAL(IDMSK: 0x%0.8X, ID: 0x%0.8X)",
			static_cast<uint32>(data >> 32), static_cast<uint32>(data));
		break;
	case GS_REG_FINISH:
		result = "FINISH()";
		break;
	default:
		result = string_format("(Unknown register: 0x%0.2X)", registerId);
		break;
	}

	return result;
}

void CGSHandler::LogWrite(uint8 registerId, uint64 data)
{
	//Filtering
	//if(!((registerId == GS_REG_FRAME_1) || (registerId == GS_REG_FRAME_2))) return;
	//if(!((registerId == GS_REG_TEST_1) || (registerId == GS_REG_TEST_2))) return;

	if(!m_loggingEnabled) return;
	auto disassembledWrite = DisassembleWrite(registerId, data);
	CLog::GetInstance().Print(LOG_NAME, "%s\r\n", disassembledWrite.c_str());
}

void CGSHandler::LogPrivateWrite(uint32 address)
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
