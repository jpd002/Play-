#include <stdio.h>
#include <string.h>
#include <functional>
#include "../AppConfig.h"
#include "../Log.h"
#include "../states/MemoryStateFile.h"
#include "../states/RegisterStateFile.h"
#include "../FrameDump.h"
#include "../ee/INTC.h"
#include "GSHandler.h"
#include "GsPixelFormats.h"
#include "string_format.h"

//Shadow Hearts 2 looks for this specific value
#define GS_REVISION (7)

#define R_REG(a, v, r)                \
	if((a)&0x4)                       \
	{                                 \
		v = (uint32)(r >> 32);        \
	}                                 \
	else                              \
	{                                 \
		v = (uint32)(r & 0xFFFFFFFF); \
	}

#define W_REG(a, v, r)                \
	if((a)&0x4)                       \
	{                                 \
		(r) &= 0x00000000FFFFFFFFULL; \
		(r) |= (uint64)(v) << 32;     \
	}                                 \
	else                              \
	{                                 \
		(r) &= 0xFFFFFFFF00000000ULL; \
		(r) |= (v);                   \
	}

#define STATE_RAM ("gs/ram")
#define STATE_REGS ("gs/regs")
#define STATE_TRXCTX ("gs/trxcontext")
#define STATE_PRIVREGS ("gs/privregs.xml")

#define STATE_PRIVREGS_PMODE ("PMODE")
#define STATE_PRIVREGS_SMODE2 ("SMODE2")
#define STATE_PRIVREGS_DISPFB1 ("DISPFB1")
#define STATE_PRIVREGS_DISPLAY1 ("DISPLAY1")
#define STATE_PRIVREGS_DISPFB2 ("DISPFB2")
#define STATE_PRIVREGS_DISPLAY2 ("DISPLAY2")
#define STATE_PRIVREGS_CSR ("CSR")
#define STATE_PRIVREGS_IMR ("IMR")
#define STATE_PRIVREGS_SIGLBLID ("SIGLBLID")
#define STATE_PRIVREGS_CRTMODE ("CrtMode")

#define STATE_REG_CBP0 ("cbp0")
#define STATE_REG_CBP1 ("cbp1")

#define LOG_NAME ("gs")

CGSHandler::CGSHandler(bool gsThreaded)
    : m_threadDone(false)
    , m_drawCallCount(0)
    , m_pCLUT(nullptr)
    , m_pRAM(nullptr)
    , m_frameDump(nullptr)
    , m_loggingEnabled(true)
    , m_gsThreaded(gsThreaded)
{
	RegisterPreferences();

	m_presentationParams.mode = static_cast<PRESENTATION_MODE>(CAppConfig::GetInstance().GetPreferenceInteger(PREF_CGSHANDLER_PRESENTATION_MODE));
	m_presentationParams.windowWidth = 512;
	m_presentationParams.windowHeight = 384;

	m_pRAM = new uint8[RAMSIZE];
	m_pCLUT = new uint16[CLUTENTRYCOUNT];
	m_writeBuffer = new RegisterWrite[REGISTERWRITEBUFFER_SIZE];

	for(int i = 0; i < PSM_MAX; i++)
	{
		m_transferWriteHandlers[i] = &CGSHandler::TransferWriteHandlerInvalid;
		m_transferReadHandlers[i] = &CGSHandler::TransferReadHandlerInvalid;
	}

	m_transferWriteHandlers[PSMCT32] = &CGSHandler::TransferWriteHandlerGeneric<CGsPixelFormats::STORAGEPSMCT32>;
	m_transferWriteHandlers[PSMCT24] = &CGSHandler::TransferWriteHandlerPSMCT24;
	m_transferWriteHandlers[PSMCT16] = &CGSHandler::TransferWriteHandlerGeneric<CGsPixelFormats::STORAGEPSMCT16>;
	m_transferWriteHandlers[PSMCT16S] = &CGSHandler::TransferWriteHandlerGeneric<CGsPixelFormats::STORAGEPSMCT16S>;
	m_transferWriteHandlers[PSMT8] = &CGSHandler::TransferWriteHandlerGeneric<CGsPixelFormats::STORAGEPSMT8>;
	m_transferWriteHandlers[PSMT4] = &CGSHandler::TransferWriteHandlerPSMT4;
	m_transferWriteHandlers[PSMT8H] = &CGSHandler::TransferWriteHandlerPSMT8H;
	m_transferWriteHandlers[PSMT4HL] = &CGSHandler::TransferWriteHandlerPSMT4H<24, 0x0F000000>;
	m_transferWriteHandlers[PSMT4HH] = &CGSHandler::TransferWriteHandlerPSMT4H<28, 0xF0000000>;

	m_transferReadHandlers[PSMCT32] = &CGSHandler::TransferReadHandlerGeneric<CGsPixelFormats::STORAGEPSMCT32>;
	m_transferReadHandlers[PSMCT24] = &CGSHandler::TransferReadHandlerPSMCT24;
	m_transferReadHandlers[PSMCT16] = &CGSHandler::TransferReadHandlerGeneric<CGsPixelFormats::STORAGEPSMCT16>;
	m_transferReadHandlers[PSMT8] = &CGSHandler::TransferReadHandlerGeneric<CGsPixelFormats::STORAGEPSMT8>;

	ResetBase();

	if(m_gsThreaded)
	{
		m_thread = std::thread([&]() { ThreadProc(); });
	}
}

CGSHandler::~CGSHandler()
{
	if(m_gsThreaded)
	{
		SendGSCall([this]() { m_threadDone = true; });
		m_thread.join();
	}
	delete[] m_pRAM;
	delete[] m_pCLUT;
	delete[] m_writeBuffer;
}

void CGSHandler::RegisterPreferences()
{
	CAppConfig::GetInstance().RegisterPreferenceInteger(PREF_CGSHANDLER_PRESENTATION_MODE, CGSHandler::PRESENTATION_MODE_FIT);
}

void CGSHandler::NotifyPreferencesChanged()
{
	SendGSCall([this]() { NotifyPreferencesChangedImpl(); });
}

void CGSHandler::SetIntc(CINTC* intc)
{
	m_intc = intc;
}

void CGSHandler::Reset()
{
	ResetBase();
	SendGSCall(std::bind(&CGSHandler::ResetImpl, this), true);
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
	m_nCSR = CSR_FIFO_EMPTY | (GS_REVISION << 16);
	m_nIMR = ~0;
	m_nSIGLBLID = 0;
	m_nCrtMode = 2;
	m_nCBP0 = 0;
	m_nCBP1 = 0;
	m_transferCount = 0;
}

void CGSHandler::ResetImpl()
{
}

void CGSHandler::NotifyPreferencesChangedImpl()
{
}

void CGSHandler::SetPresentationParams(const PRESENTATION_PARAMS& presentationParams)
{
	m_presentationParams = presentationParams;
}

CGSHandler::PRESENTATION_VIEWPORT CGSHandler::GetPresentationViewport() const
{
	PRESENTATION_VIEWPORT viewport;
	unsigned int sourceWidth = GetCrtWidth();
	unsigned int sourceHeight = GetCrtHeight();
	switch(m_presentationParams.mode)
	{
	case PRESENTATION_MODE_FILL:
		viewport.offsetX = 0;
		viewport.offsetY = 0;
		viewport.width = m_presentationParams.windowWidth;
		viewport.height = m_presentationParams.windowHeight;
		break;
	case PRESENTATION_MODE_FIT:
	{
		int viewportWidth[2];
		int viewportHeight[2];
		{
			viewportWidth[0] = m_presentationParams.windowWidth;
			viewportHeight[0] = (sourceWidth != 0) ? (m_presentationParams.windowWidth * sourceHeight) / sourceWidth : 0;
		}
		{
			viewportWidth[1] = (sourceHeight != 0) ? (m_presentationParams.windowHeight * sourceWidth) / sourceHeight : 0;
			viewportHeight[1] = m_presentationParams.windowHeight;
		}
		int selectedViewport = 0;
		if(
		    (viewportWidth[0] > static_cast<int>(m_presentationParams.windowWidth)) ||
		    (viewportHeight[0] > static_cast<int>(m_presentationParams.windowHeight)))
		{
			selectedViewport = 1;
			assert(
			    viewportWidth[1] <= static_cast<int>(m_presentationParams.windowWidth) &&
			    viewportHeight[1] <= static_cast<int>(m_presentationParams.windowHeight));
		}
		int offsetX = static_cast<int>(m_presentationParams.windowWidth - viewportWidth[selectedViewport]) / 2;
		int offsetY = static_cast<int>(m_presentationParams.windowHeight - viewportHeight[selectedViewport]) / 2;
		viewport.offsetX = offsetX;
		viewport.offsetY = offsetY;
		viewport.width = viewportWidth[selectedViewport];
		viewport.height = viewportHeight[selectedViewport];
	}
	break;
	case PRESENTATION_MODE_ORIGINAL:
	{
		int offsetX = static_cast<int>(m_presentationParams.windowWidth - sourceWidth) / 2;
		int offsetY = static_cast<int>(m_presentationParams.windowHeight - sourceHeight) / 2;
		viewport.offsetX = offsetX;
		viewport.offsetY = offsetY;
		viewport.width = sourceWidth;
		viewport.height = sourceHeight;
	}
	break;
	}
	return viewport;
}

void CGSHandler::SaveState(Framework::CZipArchiveWriter& archive)
{
	archive.InsertFile(new CMemoryStateFile(STATE_RAM, GetRam(), RAMSIZE));
	archive.InsertFile(new CMemoryStateFile(STATE_REGS, m_nReg, sizeof(uint64) * CGSHandler::REGISTER_MAX));
	archive.InsertFile(new CMemoryStateFile(STATE_TRXCTX, &m_trxCtx, sizeof(TRXCONTEXT)));

	{
		CRegisterStateFile* registerFile = new CRegisterStateFile(STATE_PRIVREGS);

		registerFile->SetRegister64(STATE_PRIVREGS_PMODE, m_nPMODE);
		registerFile->SetRegister64(STATE_PRIVREGS_SMODE2, m_nSMODE2);
		registerFile->SetRegister64(STATE_PRIVREGS_DISPFB1, m_nDISPFB1.value.q);
		registerFile->SetRegister64(STATE_PRIVREGS_DISPLAY1, m_nDISPLAY1.value.q);
		registerFile->SetRegister64(STATE_PRIVREGS_DISPFB2, m_nDISPFB2.value.q);
		registerFile->SetRegister64(STATE_PRIVREGS_DISPLAY2, m_nDISPLAY2.value.q);
		registerFile->SetRegister64(STATE_PRIVREGS_CSR, m_nCSR);
		registerFile->SetRegister64(STATE_PRIVREGS_IMR, m_nIMR);
		registerFile->SetRegister64(STATE_PRIVREGS_SIGLBLID, m_nSIGLBLID);
		registerFile->SetRegister32(STATE_PRIVREGS_CRTMODE, m_nCrtMode);
		registerFile->SetRegister32(STATE_REG_CBP0, m_nCBP0);
		registerFile->SetRegister32(STATE_REG_CBP1, m_nCBP1);

		archive.InsertFile(registerFile);
	}
}

void CGSHandler::LoadState(Framework::CZipArchiveReader& archive)
{
	archive.BeginReadFile(STATE_RAM)->Read(GetRam(), RAMSIZE);
	archive.BeginReadFile(STATE_REGS)->Read(m_nReg, sizeof(uint64) * CGSHandler::REGISTER_MAX);
	archive.BeginReadFile(STATE_TRXCTX)->Read(&m_trxCtx, sizeof(TRXCONTEXT));

	{
		CRegisterStateFile registerFile(*archive.BeginReadFile(STATE_PRIVREGS));
		m_nPMODE = registerFile.GetRegister64(STATE_PRIVREGS_PMODE);
		m_nSMODE2 = registerFile.GetRegister64(STATE_PRIVREGS_SMODE2);
		m_nDISPFB1.value.q = registerFile.GetRegister64(STATE_PRIVREGS_DISPFB1);
		m_nDISPLAY1.value.q = registerFile.GetRegister64(STATE_PRIVREGS_DISPLAY1);
		m_nDISPFB2.value.q = registerFile.GetRegister64(STATE_PRIVREGS_DISPFB2);
		m_nDISPLAY2.value.q = registerFile.GetRegister64(STATE_PRIVREGS_DISPLAY2);
		m_nCSR = registerFile.GetRegister64(STATE_PRIVREGS_CSR);
		m_nIMR = registerFile.GetRegister64(STATE_PRIVREGS_IMR);
		m_nSIGLBLID = registerFile.GetRegister64(STATE_PRIVREGS_SIGLBLID);
		m_nCrtMode = registerFile.GetRegister32(STATE_PRIVREGS_CRTMODE);
		m_nCBP0 = registerFile.GetRegister32(STATE_REG_CBP0);
		m_nCBP1 = registerFile.GetRegister32(STATE_REG_CBP1);
	}
}

void CGSHandler::Copy(const CGSHandler* gs)
{
	memcpy(GetRam(), gs->GetRam(), RAMSIZE);
	memcpy(m_nReg, gs->m_nReg, sizeof(uint64) * CGSHandler::REGISTER_MAX);
	m_trxCtx = gs->m_trxCtx;

	{
		m_nPMODE = gs->m_nPMODE;
		m_nSMODE2 = gs->m_nSMODE2;
		m_nDISPFB1.value.q = gs->m_nDISPFB1.value.q;
		m_nDISPLAY1.value.q = gs->m_nDISPLAY1.value.q;
		m_nDISPFB2.value.q = gs->m_nDISPFB2.value.q;
		m_nDISPLAY2.value.q = gs->m_nDISPLAY2.value.q;
		m_nCSR = gs->m_nCSR;
		m_nIMR = gs->m_nIMR;
		m_nSIGLBLID = gs->m_nSIGLBLID;
		m_nCrtMode = gs->m_nCrtMode;
		m_nCBP0 = gs->m_nCBP0;
		m_nCBP1 = gs->m_nCBP1;
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
		Finish();
	}

	std::lock_guard<std::recursive_mutex> registerMutexLock(m_registerMutex);
	m_nCSR |= CSR_VSYNC_INT;
	NotifyEvent(CSR_VSYNC_INT);
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

void CGSHandler::NotifyEvent(uint32 eventBit)
{
	uint32 mask = (~m_nIMR >> 8) & 0x1F;
	bool hasPendingInterrupt = (eventBit & mask) != 0;
	if(m_intc && hasPendingInterrupt)
	{
		m_intc->AssertLine(CINTC::INTC_LINE_GS);
	}
}

uint32 CGSHandler::ReadPrivRegister(uint32 nAddress)
{
	uint32 nData = 0;
	switch(nAddress & ~0x0F)
	{
	case GS_CSR:
	case GS_CSR_ALT:
		//Force CSR to have the H-Blank bit set.
		{
			std::lock_guard<std::recursive_mutex> registerMutexLock(m_registerMutex);
			m_nCSR |= CSR_HSYNC_INT;
			NotifyEvent(CSR_HSYNC_INT);
			R_REG(nAddress, nData, m_nCSR);
		}
		break;
	case GS_IMR:
		R_REG(nAddress, nData, m_nIMR);
		break;
	case GS_SIGLBLID:
		R_REG(nAddress, nData, m_nSIGLBLID);
		break;
	default:
		CLog::GetInstance().Warn(LOG_NAME, "Read an unhandled priviledged register (0x%08X).\r\n", nAddress);
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
	case GS_CSR_ALT:
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
		if(!(nAddress & 0x04))
		{
			//Some games (Soul Calibur 2, Crash Nitro Kart) will unmask interrupts
			//right after starting a transfer that sets a SIGNAL... Let the
			//interrupt go through even though it's not supposed to work that way
			NotifyEvent(m_nCSR & 0x1F);
		}
		break;
	case GS_SIGLBLID:
		W_REG(nAddress, nData, m_nSIGLBLID);
		break;
	default:
		CLog::GetInstance().Warn(LOG_NAME, "Wrote to an unhandled priviledged register (0x%08X, 0x%08X).\r\n", nAddress, nData);
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
	SendGSCall(std::bind(&CGSHandler::InitializeImpl, this), true);
}

void CGSHandler::Release()
{
	SendGSCall(std::bind(&CGSHandler::ReleaseImpl, this), true);
}

void CGSHandler::Finish()
{
	FlushWriteBuffer();
	SendGSCall(std::bind(&CGSHandler::MarkNewFrame, this));
	Flip(true);
}

void CGSHandler::Flip(bool waitForCompletion)
{
	SendGSCall(std::bind(&CGSHandler::FlipImpl, this), waitForCompletion, waitForCompletion);
}

void CGSHandler::FlipImpl()
{
	OnFlipComplete();
	m_flipped = true;
}

void CGSHandler::MarkNewFrame()
{
	OnNewFrame(m_drawCallCount);
	m_drawCallCount = 0;
#ifdef _DEBUG
	CLog::GetInstance().Print(LOG_NAME, "Frame Done.\r\n---------------------------------------------------------------------------------\r\n");
#endif
}

uint8* CGSHandler::GetRam() const
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

void CGSHandler::FeedImageData(const void* data, uint32 length)
{
	assert(m_writeBufferProcessIndex == m_writeBufferSize);
	SubmitWriteBuffer();

	m_transferCount++;

	//Allocate 0x10 more bytes to allow transfer handlers
	//to read beyond the actual length of the buffer (ie.: PSMCT24)

	std::vector<uint8> imageData(length + 0x10);
	memcpy(imageData.data(), data, length);
#ifdef DEBUGGER_INCLUDED
	if(m_frameDump)
	{
		m_frameDump->AddImagePacket(imageData.data(), length);
	}
#endif
	SendGSCall(
	    [this, imageData = std::move(imageData), length]() {
		    FeedImageDataImpl(imageData.data(), length);
	    });
}

void CGSHandler::ReadImageData(void* data, uint32 length)
{
	assert(m_writeBufferProcessIndex == m_writeBufferSize);
	SubmitWriteBuffer();
	SendGSCall([this, data, length]() { ReadImageDataImpl(data, length); }, true);
}

void CGSHandler::ProcessWriteBuffer(const CGsPacketMetadata* metadata)
{
	assert(m_writeBufferProcessIndex <= m_writeBufferSize);
	assert(m_writeBufferSubmitIndex <= m_writeBufferProcessIndex);
#ifdef DEBUGGER_INCLUDED
	if(m_frameDump)
	{
		uint32 packetSize = m_writeBufferSize - m_writeBufferProcessIndex;
		if(packetSize != 0)
		{
			m_frameDump->AddRegisterPacket(m_writeBuffer + m_writeBufferProcessIndex, packetSize, metadata);
		}
	}
#endif
	for(uint32 writeIndex = m_writeBufferProcessIndex; writeIndex < m_writeBufferSize; writeIndex++)
	{
		const auto& write = m_writeBuffer[writeIndex];
		switch(write.first)
		{
		case GS_REG_SIGNAL:
		{
			auto signal = make_convertible<SIGNAL>(write.second);
			auto siglblid = make_convertible<SIGLBLID>(m_nSIGLBLID);
			siglblid.sigid &= ~signal.idmsk;
			siglblid.sigid |= signal.id;
			m_nSIGLBLID = siglblid;
			assert((m_nCSR & CSR_SIGNAL_EVENT) == 0);
			m_nCSR |= CSR_SIGNAL_EVENT;
			NotifyEvent(CSR_SIGNAL_EVENT);
		}
		break;
		case GS_REG_FINISH:
			m_nCSR |= CSR_FINISH_EVENT;
			NotifyEvent(CSR_FINISH_EVENT);
			break;
		case GS_REG_LABEL:
		{
			auto label = make_convertible<LABEL>(write.second);
			auto siglblid = make_convertible<SIGLBLID>(m_nSIGLBLID);
			siglblid.lblid &= ~label.idmsk;
			siglblid.lblid |= label.id;
			m_nSIGLBLID = siglblid;
		}
		break;
		}
	}
	m_writeBufferProcessIndex = m_writeBufferSize;
	uint32 submitPending = m_writeBufferProcessIndex - m_writeBufferSubmitIndex;
	if(submitPending >= REGISTERWRITEBUFFER_SUBMIT_THRESHOLD)
	{
		SubmitWriteBuffer();
	}
}

void CGSHandler::SubmitWriteBuffer()
{
	assert(m_writeBufferSubmitIndex <= m_writeBufferSize);
	if(m_writeBufferSubmitIndex == m_writeBufferSize) return;

	m_transferCount++;
	uint32 bufferStartIndex = m_writeBufferSubmitIndex;
	uint32 bufferEndIndex = m_writeBufferSize;

	SendGSCall(
	    [this, bufferStartIndex, bufferEndIndex]() {
		    SubmitWriteBufferImpl(bufferStartIndex, bufferEndIndex);
	    });

	m_writeBufferSubmitIndex = m_writeBufferSize;
}

void CGSHandler::FlushWriteBuffer()
{
	//Everything should be processed at this point
	assert(m_writeBufferProcessIndex == m_writeBufferSize);
	//Make sure everything is submitted
	SubmitWriteBuffer();
	m_writeBufferSize = 0;
	m_writeBufferProcessIndex = 0;
	m_writeBufferSubmitIndex = 0;
	//Nothing should be written to the buffer after that
}

void CGSHandler::WriteRegisterImpl(uint8 nRegister, uint64 nData)
{
	nRegister &= REGISTER_MAX - 1;
	m_nReg[nRegister] = nData;

	switch(nRegister)
	{
	case GS_REG_TEX0_1:
	case GS_REG_TEX0_2:
	{
		unsigned int nContext = nRegister - GS_REG_TEX0_1;
		assert(nContext == 0 || nContext == 1);
		auto tex0 = make_convertible<TEX0>(m_nReg[GS_REG_TEX0_1 + nContext]);
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

		auto tex0 = make_convertible<TEX0>(m_nReg[GS_REG_TEX0_1 + nContext]);
		SyncCLUT(tex0);
	}
	break;

	case GS_REG_TRXDIR:
		BeginTransfer();
		break;

	case GS_REG_HWREG:
		m_transferCount++;
		FeedImageDataImpl(reinterpret_cast<const uint8*>(&nData), 8);
		break;
	}

#ifdef _DEBUG
	LogWrite(nRegister, nData);
#endif
}

void CGSHandler::FeedImageDataImpl(const uint8* imageData, uint32 length)
{
	if(m_trxCtx.nSize == 0)
	{
#ifdef _DEBUG
		CLog::GetInstance().Warn(LOG_NAME, "Warning. Received image data when no transfer was expected.\r\n");
#endif
	}
	else
	{
		if(m_trxCtx.nSize < length)
		{
			length = m_trxCtx.nSize;
			//assert(0);
			//return;
		}

		TransferWrite(imageData, length);
		m_trxCtx.nSize -= length;

		if(m_trxCtx.nSize == 0)
		{
			auto trxReg = make_convertible<TRXREG>(m_nReg[GS_REG_TRXREG]);
			//assert(m_trxCtx.nRRY == trxReg.nRRH);
			ProcessHostToLocalTransfer();

#ifdef _DEBUG
			auto bltBuf = make_convertible<BITBLTBUF>(m_nReg[GS_REG_BITBLTBUF]);
			CLog::GetInstance().Print(LOG_NAME, "Completed image transfer at 0x%08X (dirty = %d).\r\n", bltBuf.GetDstPtr(), m_trxCtx.nDirty);
#endif
		}
	}

	assert(m_transferCount != 0);
	m_transferCount--;
}

void CGSHandler::ReadImageDataImpl(void* ptr, uint32 size)
{
	assert(m_trxCtx.nSize != 0);
	assert(m_trxCtx.nSize == size);
	auto bltBuf = make_convertible<BITBLTBUF>(m_nReg[GS_REG_BITBLTBUF]);
	auto trxPos = make_convertible<TRXPOS>(m_nReg[GS_REG_TRXPOS]);

	assert(trxPos.nDIR == 0);
	((this)->*(m_transferReadHandlers[bltBuf.nSrcPsm]))(ptr, size);
}

void CGSHandler::SubmitWriteBufferImpl(uint32 bufferStartIndex, uint32 bufferEndIndex)
{
	for(uint32 i = bufferStartIndex; i < bufferEndIndex; i++)
	{
		const auto& write = m_writeBuffer[i];
		WriteRegisterImpl(write.first, write.second);
	}

	assert(m_transferCount != 0);
	m_transferCount--;
}

std::pair<uint32, uint32> CGSHandler::GetTransferInvalidationRange(const BITBLTBUF& bltBuf, const TRXREG& trxReg, const TRXPOS& trxPos)
{
	uint32 transferAddress = bltBuf.GetDstPtr();

	//Find the pages that are touched by this transfer
	auto transferPageSize = CGsPixelFormats::GetPsmPageSize(bltBuf.nDstPsm);

	// DBZ Budokai Tenkaichi 2 and 3 use invalid (empty) buffer sizes
	// Account for that, by assuming trxReg.nRRW.
	auto width = bltBuf.GetDstWidth();
	if(width == 0)
	{
		width = trxReg.nRRW;
	}

	//Espgaluda uses an offset into a big memory area. The Y offset is not necessarily
	//a multiple of the page height. We need to make sure to take this into account.
	uint32 intraPageOffsetY = trxPos.nDSAY % transferPageSize.second;

	uint32 pageCountX = (width + transferPageSize.first - 1) / transferPageSize.first;
	uint32 pageCountY = (intraPageOffsetY + trxReg.nRRH + transferPageSize.second - 1) / transferPageSize.second;

	uint32 pageCount = pageCountX * pageCountY;
	uint32 transferSize = pageCount * CGsPixelFormats::PAGESIZE;
	uint32 transferOffset = (trxPos.nDSAY / transferPageSize.second) * pageCountX * CGsPixelFormats::PAGESIZE;

	return std::make_pair(transferAddress + transferOffset, transferSize);
}

void CGSHandler::BeginTransfer()
{
	uint32 trxDir = m_nReg[GS_REG_TRXDIR] & 0x03;
	if(trxDir == 0 || trxDir == 1)
	{
		//"Host to Local" or "Local to Host"
		auto bltBuf = make_convertible<BITBLTBUF>(m_nReg[GS_REG_BITBLTBUF]);
		auto trxReg = make_convertible<TRXREG>(m_nReg[GS_REG_TRXREG]);
		auto psm = (trxDir == 0) ? bltBuf.nDstPsm : bltBuf.nSrcPsm;
		unsigned int nPixelSize = 0;

		//We need to figure out the pixel size of the stream
		switch(psm)
		{
		case PSMCT32:
		case PSMZ32:
			nPixelSize = 32;
			break;
		case PSMCT24:
		case PSMZ24:
			nPixelSize = 24;
			break;
		case PSMCT16:
		case PSMCT16S:
		case PSMZ16S:
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

		//Make sure transfer size is a multiple of 16. Some games (ex.: Gregory Horror Show)
		//specify transfer width/height that will give a transfer size that is not a multiple of 16
		//and will prevent proper handling of image transfer (texture cache will not be invalidated).
		m_trxCtx.nSize = ((trxReg.nRRW * trxReg.nRRH * nPixelSize) / 8) & ~0xF;
		m_trxCtx.nRealSize = m_trxCtx.nSize;
		m_trxCtx.nRRX = 0;
		m_trxCtx.nRRY = 0;

		if(trxDir == 0)
		{
			BeginTransferWrite();
			CLog::GetInstance().Print(LOG_NAME, "Starting transfer to 0x%08X, buffer size %d, psm: %d, size (%dx%d)\r\n",
			                          bltBuf.GetDstPtr(), bltBuf.GetDstWidth(), bltBuf.nDstPsm, trxReg.nRRW, trxReg.nRRH);
		}
		else if(trxDir == 1)
		{
			ProcessLocalToHostTransfer();
			CLog::GetInstance().Print(LOG_NAME, "Starting transfer from 0x%08X, buffer size %d, psm: %d, size (%dx%d)\r\n",
			                          bltBuf.GetSrcPtr(), bltBuf.GetSrcWidth(), bltBuf.nSrcPsm, trxReg.nRRW, trxReg.nRRH);
		}
	}
	else if(trxDir == 2)
	{
		//Local to Local
		ProcessLocalToLocalTransfer();
	}
}

void CGSHandler::BeginTransferWrite()
{
	m_trxCtx.nDirty = false;
}

void CGSHandler::TransferWrite(const uint8* imageData, uint32 length)
{
	auto bltBuf = make_convertible<BITBLTBUF>(m_nReg[GS_REG_BITBLTBUF]);
	m_trxCtx.nDirty |= ((this)->*(m_transferWriteHandlers[bltBuf.nDstPsm]))(imageData, length);
}

bool CGSHandler::TransferWriteHandlerInvalid(const void* pData, uint32 nLength)
{
	assert(0);
	return false;
}

template <typename Storage>
bool CGSHandler::TransferWriteHandlerGeneric(const void* pData, uint32 nLength)
{
	bool nDirty = false;
	auto trxPos = make_convertible<TRXPOS>(m_nReg[GS_REG_TRXPOS]);
	auto trxReg = make_convertible<TRXREG>(m_nReg[GS_REG_TRXREG]);
	auto trxBuf = make_convertible<BITBLTBUF>(m_nReg[GS_REG_BITBLTBUF]);

	nLength /= sizeof(typename Storage::Unit);

	CGsPixelFormats::CPixelIndexor<Storage> Indexor(m_pRAM, trxBuf.GetDstPtr(), trxBuf.nDstWidth);

	auto pSrc = reinterpret_cast<const typename Storage::Unit*>(pData);

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

bool CGSHandler::TransferWriteHandlerPSMCT24(const void* pData, uint32 nLength)
{
	auto trxPos = make_convertible<TRXPOS>(m_nReg[GS_REG_TRXPOS]);
	auto trxReg = make_convertible<TRXREG>(m_nReg[GS_REG_TRXREG]);
	auto trxBuf = make_convertible<BITBLTBUF>(m_nReg[GS_REG_BITBLTBUF]);

	CGsPixelFormats::CPixelIndexorPSMCT32 Indexor(m_pRAM, trxBuf.GetDstPtr(), trxBuf.nDstWidth);

	auto pSrc = reinterpret_cast<const uint8*>(pData);

	for(unsigned int i = 0; i < nLength; i += 3)
	{
		uint32 nX = (m_trxCtx.nRRX + trxPos.nDSAX) % 2048;
		uint32 nY = (m_trxCtx.nRRY + trxPos.nDSAY) % 2048;

		uint32* pDstPixel = Indexor.GetPixelAddress(nX, nY);
		uint32 nSrcPixel = *reinterpret_cast<const uint32*>(&pSrc[i]) & 0x00FFFFFF;
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

bool CGSHandler::TransferWriteHandlerPSMT4(const void* pData, uint32 nLength)
{
	bool dirty = false;
	auto trxPos = make_convertible<TRXPOS>(m_nReg[GS_REG_TRXPOS]);
	auto trxReg = make_convertible<TRXREG>(m_nReg[GS_REG_TRXREG]);
	auto trxBuf = make_convertible<BITBLTBUF>(m_nReg[GS_REG_BITBLTBUF]);

	CGsPixelFormats::CPixelIndexorPSMT4 Indexor(m_pRAM, trxBuf.GetDstPtr(), trxBuf.nDstWidth);

	auto pSrc = reinterpret_cast<const uint8*>(pData);

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
bool CGSHandler::TransferWriteHandlerPSMT4H(const void* pData, uint32 nLength)
{
	auto trxPos = make_convertible<TRXPOS>(m_nReg[GS_REG_TRXPOS]);
	auto trxReg = make_convertible<TRXREG>(m_nReg[GS_REG_TRXREG]);
	auto trxBuf = make_convertible<BITBLTBUF>(m_nReg[GS_REG_BITBLTBUF]);

	CGsPixelFormats::CPixelIndexorPSMCT32 Indexor(m_pRAM, trxBuf.GetDstPtr(), trxBuf.nDstWidth);

	auto pSrc = reinterpret_cast<const uint8*>(pData);

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

bool CGSHandler::TransferWriteHandlerPSMT8H(const void* pData, uint32 nLength)
{
	auto trxPos = make_convertible<TRXPOS>(m_nReg[GS_REG_TRXPOS]);
	auto trxReg = make_convertible<TRXREG>(m_nReg[GS_REG_TRXREG]);
	auto trxBuf = make_convertible<BITBLTBUF>(m_nReg[GS_REG_BITBLTBUF]);

	CGsPixelFormats::CPixelIndexorPSMCT32 Indexor(m_pRAM, trxBuf.GetDstPtr(), trxBuf.nDstWidth);

	auto pSrc = reinterpret_cast<const uint8*>(pData);

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

void CGSHandler::TransferReadHandlerInvalid(void*, uint32)
{
	assert(0);
}

template <typename Storage>
void CGSHandler::TransferReadHandlerGeneric(void* buffer, uint32 length)
{
	auto trxPos = make_convertible<TRXPOS>(m_nReg[GS_REG_TRXPOS]);
	auto trxReg = make_convertible<TRXREG>(m_nReg[GS_REG_TRXREG]);
	auto trxBuf = make_convertible<BITBLTBUF>(m_nReg[GS_REG_BITBLTBUF]);

	uint32 typedLength = length / sizeof(typename Storage::Unit);
	auto typedBuffer = reinterpret_cast<typename Storage::Unit*>(buffer);

	CGsPixelFormats::CPixelIndexor<Storage> indexor(GetRam(), trxBuf.GetSrcPtr(), trxBuf.nSrcWidth);
	for(uint32 i = 0; i < typedLength; i++)
	{
		uint32 x = (m_trxCtx.nRRX + trxPos.nSSAX) % 2048;
		uint32 y = (m_trxCtx.nRRY + trxPos.nSSAY) % 2048;
		auto pixel = indexor.GetPixel(x, y);
		typedBuffer[i] = pixel;
		m_trxCtx.nRRX++;
		if(m_trxCtx.nRRX == trxReg.nRRW)
		{
			m_trxCtx.nRRX = 0;
			m_trxCtx.nRRY++;
		}
	}
}

void CGSHandler::TransferReadHandlerPSMCT24(void* buffer, uint32 length)
{
	auto trxPos = make_convertible<TRXPOS>(m_nReg[GS_REG_TRXPOS]);
	auto trxReg = make_convertible<TRXREG>(m_nReg[GS_REG_TRXREG]);
	auto trxBuf = make_convertible<BITBLTBUF>(m_nReg[GS_REG_BITBLTBUF]);

	auto dst = reinterpret_cast<uint8*>(buffer);

	CGsPixelFormats::CPixelIndexorPSMCT32 indexor(GetRam(), trxBuf.GetSrcPtr(), trxBuf.nSrcWidth);
	for(uint32 i = 0; i < length; i += 3)
	{
		uint32 x = (m_trxCtx.nRRX + trxPos.nSSAX) % 2048;
		uint32 y = (m_trxCtx.nRRY + trxPos.nSSAY) % 2048;
		auto pixel = indexor.GetPixel(x, y);
		dst[i + 0] = (pixel >> 0) & 0xFF;
		dst[i + 1] = (pixel >> 8) & 0xFF;
		dst[i + 2] = (pixel >> 16) & 0xFF;
		m_trxCtx.nRRX++;
		if(m_trxCtx.nRRX == trxReg.nRRW)
		{
			m_trxCtx.nRRX = 0;
			m_trxCtx.nRRY++;
		}
	}
}

void CGSHandler::SetCrt(bool nIsInterlaced, unsigned int nMode, bool nIsFrameMode)
{
	m_nCrtMode = nMode;

	SMODE2 smode2;
	smode2 <<= 0;
	smode2.interlaced = nIsInterlaced ? 1 : 0;
	smode2.ffmd = nIsFrameMode ? 1 : 0;
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

std::pair<uint64, uint64> CGSHandler::GetCurrentDisplayInfo()
{
	std::lock_guard<std::recursive_mutex> registerMutexLock(m_registerMutex);
	unsigned int readCircuit = GetCurrentReadCircuit();
	switch(readCircuit)
	{
	default:
	case 0:
		return {m_nDISPFB1.value.q, m_nDISPLAY1.value.q};
	case 1:
		return {m_nDISPFB2.value.q, m_nDISPLAY2.value.q};
	}
}

unsigned int CGSHandler::GetCurrentReadCircuit()
{
	uint32 rcMode = m_nPMODE & 0x03;
	switch(rcMode)
	{
	default:
	case 0:
		//No read circuit enabled?
		return 0;
	case 1:
		return 0;
	case 2:
		return 1;
	case 3:
	{
		//Both are enabled... See if we can find out which one is good
		//This happens in Capcom Classics Collection Vol. 2
		std::lock_guard<std::recursive_mutex> registerMutexLock(m_registerMutex);
		bool fb1Null = (m_nDISPFB1.value.q == 0);
		bool fb2Null = (m_nDISPFB2.value.q == 0);
		if(!fb1Null && fb2Null)
		{
			return 0;
		}
		if(fb1Null && !fb2Null)
		{
			return 1;
		}
		return 0;
	}
	break;
	}
}

void CGSHandler::SyncCLUT(const TEX0& tex0)
{
	if(!ProcessCLD(tex0)) return;

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

bool CGSHandler::ProcessCLD(const TEX0& tex0)
{
	switch(tex0.nCLD)
	{
	case 0:
		//No changes to CLUT
		return false;
	default:
		assert(false);
	case 1:
		return true;
	case 2:
		m_nCBP0 = tex0.nCBP;
		return true;
	case 3:
		m_nCBP1 = tex0.nCBP;
		return true;
	case 4:
		if(m_nCBP0 == tex0.nCBP) return false;
		m_nCBP0 = tex0.nCBP;
		return true;
	case 5:
		if(m_nCBP1 == tex0.nCBP) return false;
		m_nCBP1 = tex0.nCBP;
		return true;
	}
}

template <typename Indexor>
bool CGSHandler::ReadCLUT4_16(const TEX0& tex0)
{
	bool changed = false;

	assert(tex0.nCSA < 32);

	Indexor indexor(m_pRAM, tex0.GetCLUTPtr(), 1);
	uint32 clutOffset = tex0.nCSA * 16;
	uint16* pDst = m_pCLUT + clutOffset;

	for(unsigned int j = 0; j < 2; j++)
	{
		for(unsigned int i = 0; i < 8; i++)
		{
			uint16 color = indexor.GetPixel(i, j);

			if(*pDst != color)
			{
				changed = true;
			}

			(*pDst++) = color;
		}
	}

	return changed;
}

template <typename Indexor>
bool CGSHandler::ReadCLUT8_16(const TEX0& tex0)
{
	bool changed = false;

	Indexor indexor(m_pRAM, tex0.GetCLUTPtr(), 1);

	for(unsigned int j = 0; j < 16; j++)
	{
		for(unsigned int i = 0; i < 16; i++)
		{
			uint16 color = indexor.GetPixel(i, j);

			uint8 index = i + (j * 16);
			index = (index & ~0x18) | ((index & 0x08) << 1) | ((index & 0x10) >> 1);

			if(m_pCLUT[index] != color)
			{
				changed = true;
			}

			m_pCLUT[index] = color;
		}
	}

	return changed;
}

void CGSHandler::ReadCLUT4(const TEX0& tex0)
{
	bool changed = false;

	if(tex0.nCSM == 0)
	{
		//CSM1 mode
		if(tex0.nCPSM == PSMCT32 || tex0.nCPSM == PSMCT24)
		{
			assert(tex0.nCSA < 16);

			CGsPixelFormats::CPixelIndexorPSMCT32 Indexor(m_pRAM, tex0.GetCLUTPtr(), 1);
			uint32 clutOffset = (tex0.nCSA & 0x0F) * 16;
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
			changed = ReadCLUT4_16<CGsPixelFormats::CPixelIndexorPSMCT16>(tex0);
		}
		else if(tex0.nCPSM == PSMCT16S)
		{
			changed = ReadCLUT4_16<CGsPixelFormats::CPixelIndexorPSMCT16S>(tex0);
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

		auto texClut = make_convertible<TEXCLUT>(m_nReg[GS_REG_TEXCLUT]);

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

void CGSHandler::ReadCLUT8(const TEX0& tex0)
{
	assert(tex0.nCSA == 0);

	bool changed = false;

	if(tex0.nCSM == 0)
	{
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
			changed = ReadCLUT8_16<CGsPixelFormats::CPixelIndexorPSMCT16>(tex0);
		}
		else if(tex0.nCPSM == PSMCT16S)
		{
			changed = ReadCLUT8_16<CGsPixelFormats::CPixelIndexorPSMCT16S>(tex0);
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

		auto texClut = make_convertible<TEXCLUT>(m_nReg[GS_REG_TEXCLUT]);

		CGsPixelFormats::CPixelIndexorPSMCT16 indexor(m_pRAM, tex0.GetCLUTPtr(), texClut.nCBW);
		unsigned int offsetX = texClut.GetOffsetU();
		unsigned int offsetY = texClut.GetOffsetV();
		uint16* dst = m_pCLUT;

		for(unsigned int i = 0; i < 0x100; i++)
		{
			uint16 color = indexor.GetPixel(offsetX + i, offsetY);

			if(*dst != color)
			{
				changed = true;
			}

			(*dst++) = color;
		}
	}

	if(changed)
	{
		ProcessClutTransfer(tex0.nCSA, 0);
	}
}

bool CGSHandler::IsCompatibleFramebufferPSM(unsigned int psmFb, unsigned int psmTex)
{
	if((psmTex == CGSHandler::PSMCT32) || (psmTex == CGSHandler::PSMCT24))
	{
		return (psmFb == CGSHandler::PSMCT32) || (psmFb == CGSHandler::PSMCT24);
	}
	else
	{
		return (psmFb == psmTex);
	}
}

void CGSHandler::MakeLinearCLUT(const TEX0& tex0, std::array<uint32, 256>& clut) const
{
	static const auto RGBA16ToRGBA32 =
	    [](uint16 color) {
		    return ((color & 0x8000) ? 0xFF000000 : 0) | ((color & 0x7C00) << 9) | ((color & 0x03E0) << 6) | ((color & 0x001F) << 3);
	    };

	unsigned int entryCount = CGsPixelFormats::IsPsmIDTEX4(tex0.nPsm) ? 16 : 256;

	if(CGsPixelFormats::IsPsmIDTEX4(tex0.nPsm))
	{
		if(tex0.nCPSM == PSMCT32 || tex0.nCPSM == PSMCT24)
		{
			assert(tex0.nCSA < 16);
			uint32 clutOffset = (tex0.nCSA & 0xF) * 16;

			for(unsigned int i = 0; i < 16; i++)
			{
				uint32 color =
				    (static_cast<uint16>(m_pCLUT[i + clutOffset + 0x000])) |
				    (static_cast<uint16>(m_pCLUT[i + clutOffset + 0x100]) << 16);
				clut[i] = color;
			}
		}
		else if(tex0.nCPSM == PSMCT16 || tex0.nCPSM == PSMCT16S)
		{
			//CSA is 5-bit, shouldn't go over 31
			assert(tex0.nCSA < 32);
			uint32 clutOffset = tex0.nCSA * 16;

			for(unsigned int i = 0; i < 16; i++)
			{
				clut[i] = RGBA16ToRGBA32(m_pCLUT[i + clutOffset]);
			}
		}
		else
		{
			assert(false);
		}
	}
	else if(CGsPixelFormats::IsPsmIDTEX8(tex0.nPsm))
	{
		if(tex0.nCPSM == PSMCT32 || tex0.nCPSM == PSMCT24)
		{
			for(unsigned int i = 0; i < 256; i++)
			{
				uint32 offset = ((tex0.nCSA * 16) + i) & 0xFF;
				uint32 color =
				    (static_cast<uint16>(m_pCLUT[offset + 0x000])) |
				    (static_cast<uint16>(m_pCLUT[offset + 0x100]) << 16);
				clut[i] = color;
			}
		}
		else if(tex0.nCPSM == PSMCT16 || tex0.nCPSM == PSMCT16S)
		{
			assert(tex0.nCSA == 0);
			for(unsigned int i = 0; i < 256; i++)
			{
				clut[i] = RGBA16ToRGBA32(m_pCLUT[i]);
			}
		}
		else
		{
			assert(false);
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
		result = string_format("RGBAQ(R: 0x%02X, G: 0x%02X, B: 0x%02X, A: 0x%02X, Q: %f)",
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
	case GS_REG_FOG:
	{
		auto fog = static_cast<uint8>(data >> 56);
		result = string_format("FOG(F: %d)", fog);
	}
	break;
	case GS_REG_TEX0_1:
	case GS_REG_TEX0_2:
	{
		auto tex0 = make_convertible<TEX0>(data);
		result = string_format("TEX0_%i(TBP: 0x%08X, TBW: %i, PSM: %i, TW: %i, TH: %i, TCC: %i, TFX: %i, CBP: 0x%08X, CPSM: %i, CSM: %i, CSA: %i, CLD: %i)",
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
		result = string_format("TEX2_%i(PSM: %i, CBP: 0x%08X, CPSM: %i, CSM: %i, CSA: %i, CLD: %i)",
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
	case GS_REG_MIPTBP1_1:
	case GS_REG_MIPTBP1_2:
	{
		auto miptbp1 = make_convertible<MIPTBP1>(data);
		result = string_format("MIPTBP1_%d(TBP1: 0x%08X, TBW1: %d, TBP2: 0x%08X, TBW2: %d, TBP3: 0x%08X, TBW3: %d)",
		                       (registerId == GS_REG_MIPTBP1_1) ? 1 : 2,
		                       miptbp1.GetTbp1(), miptbp1.GetTbw1(),
		                       miptbp1.GetTbp2(), miptbp1.GetTbw2(),
		                       miptbp1.GetTbp3(), miptbp1.GetTbw3());
	}
	break;
	case GS_REG_MIPTBP2_1:
	case GS_REG_MIPTBP2_2:
	{
		auto miptbp2 = make_convertible<MIPTBP2>(data);
		result = string_format("MIPTBP2_%d(TBP4: 0x%08X, TBW4: %d, TBP5: 0x%08X, TBW5: %d, TBP6: 0x%08X, TBW6: %d)",
		                       (registerId == GS_REG_MIPTBP2_1) ? 1 : 2,
		                       miptbp2.GetTbp4(), miptbp2.GetTbw4(),
		                       miptbp2.GetTbp5(), miptbp2.GetTbw5(),
		                       miptbp2.GetTbp6(), miptbp2.GetTbw6());
	}
	break;
	case GS_REG_TEXA:
	{
		auto texa = make_convertible<TEXA>(data);
		result = string_format("TEXA(TA0: 0x%02X, AEM: %i, TA1: 0x%02X)",
		                       texa.nTA0, texa.nAEM, texa.nTA1);
	}
	break;
	case GS_REG_FOGCOL:
	{
		auto fogcol = make_convertible<FOGCOL>(data);
		result = string_format("FOGCOL(R: 0x%02X, G: 0x%02X, B: 0x%02X)",
		                       fogcol.nFCR, fogcol.nFCG, fogcol.nFCB);
	}
	break;
	case GS_REG_TEXFLUSH:
		result = "TEXFLUSH()";
		break;
	case GS_REG_ALPHA_1:
	case GS_REG_ALPHA_2:
	{
		auto alpha = make_convertible<ALPHA>(data);
		result = string_format("ALPHA_%i(A: %i, B: %i, C: %i, D: %i, FIX: 0x%02X)",
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
	case GS_REG_COLCLAMP:
		result = string_format("COLCLAMP(CLAMP: %d)", data & 1);
		break;
	case GS_REG_TEST_1:
	case GS_REG_TEST_2:
	{
		auto tst = make_convertible<TEST>(data);
		result = string_format("TEST_%i(ATE: %i, ATST: %i, AREF: 0x%02X, AFAIL: %i, DATE: %i, DATM: %i, ZTE: %i, ZTST: %i)",
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
		result = string_format("FRAME_%i(FBP: 0x%08X, FBW: %d, PSM: %d, FBMSK: 0x%08X)",
		                       (registerId == GS_REG_FRAME_1) ? 1 : 2, fr.GetBasePtr(), fr.GetWidth(), fr.nPsm, fr.nMask);
	}
	break;
	case GS_REG_ZBUF_1:
	case GS_REG_ZBUF_2:
	{
		auto zbuf = make_convertible<ZBUF>(data);
		result = string_format("ZBUF_%i(ZBP: 0x%08X, PSM: %i, ZMSK: %i)",
		                       (registerId == GS_REG_ZBUF_1) ? 1 : 2, zbuf.GetBasePtr(), zbuf.nPsm, zbuf.nMask);
	}
	break;
	case GS_REG_BITBLTBUF:
	{
		auto buf = make_convertible<BITBLTBUF>(data);
		result = string_format("BITBLTBUF(SBP: 0x%08X, SBW: %i, SPSM: %i, DBP: 0x%08X, DBW: %i, DPSM: %i)",
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
	case GS_REG_HWREG:
		result = string_format("HWREG(DATA: 0x%016llX)", data);
		break;
	case GS_REG_SIGNAL:
	{
		auto signal = make_convertible<SIGNAL>(data);
		result = string_format("SIGNAL(IDMSK: 0x%08X, ID: 0x%08X)",
		                       signal.idmsk, signal.id);
	}
	break;
	case GS_REG_FINISH:
		result = "FINISH()";
		break;
	case GS_REG_LABEL:
	{
		auto label = make_convertible<LABEL>(data);
		result = string_format("LABEL(IDMSK: 0x%08X, ID: 0x%08X)",
		                       label.idmsk, label.id);
	}
	break;
	default:
		result = string_format("(Unknown register: 0x%02X)", registerId);
		break;
	}

	return result;
}

void CGSHandler::LogWrite(uint8 registerId, uint64 data)
{
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
		CLog::GetInstance().Print(LOG_NAME, "PMODE(0x%08X);\r\n", m_nPMODE);
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
		CLog::GetInstance().Print(LOG_NAME, "DISPFB%d(FBP: 0x%08X, FBW: %d, PSM: %d, DBX: %d, DBY: %d);\r\n",
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
	case GS_CSR_ALT:
		//CSR
		break;
	case GS_IMR:
		CLog::GetInstance().Print(LOG_NAME, "IMR(0x%08X);\r\n", m_nIMR);
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
		m_mailBox.WaitForCall();
		while(m_mailBox.IsPending())
		{
			m_mailBox.ReceiveCall();
		}
	}
}

void CGSHandler::SendGSCall(const CMailBox::FunctionType& function, bool waitForCompletion, bool forceWaitForCompletion)
{
	if(!m_gsThreaded)
	{
		waitForCompletion = false;
	}
	waitForCompletion |= forceWaitForCompletion;
	m_mailBox.SendCall(function, waitForCompletion);
}

void CGSHandler::SendGSCall(CMailBox::FunctionType&& function)
{
	m_mailBox.SendCall(std::move(function));
}

void CGSHandler::ProcessSingleFrame()
{
	assert(!m_gsThreaded);
	assert(!m_flipped);
	while(!m_flipped)
	{
		m_mailBox.WaitForCall();
		while(m_mailBox.IsPending() && !m_flipped)
		{
			m_mailBox.ReceiveCall();
		}
	}
	m_flipped = false;
}

Framework::CBitmap CGSHandler::GetScreenshot()
{
	throw std::runtime_error("Screenshot feature is not implemented in current backend.");
}
