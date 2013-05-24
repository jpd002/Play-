#include "GsContextView.h"
#include "GsStateUtils.h"
#include "../../GsPixelFormats.h"
#include "../GSH_Direct3D9.h"
#include "win32/VerticalSplitter.h"

void TexUploader_Psm8(CGSHandler* gs, Framework::CBitmap& dst, const CGSHandler::TEX0& tex0, const CGSHandler::TEXA& texA)
{
	uint32 nPointer			= tex0.GetBufPtr();
	unsigned int nWidth		= tex0.GetWidth();
	unsigned int nHeight	= tex0.GetHeight();
	unsigned int nDstPitch	= dst.GetPitch() / 4;
	uint32* dstBuf			= reinterpret_cast<uint32*>(dst.GetPixels());

	CGsPixelFormats::CPixelIndexorPSMT8 Indexor(gs->GetRam(), nPointer, tex0.nBufWidth);

	for(unsigned int j = 0; j < nHeight; j++)
	{
		for(unsigned int i = 0; i < nWidth; i++)
		{
			uint8 pixel = static_cast<uint8>(Indexor.GetPixel(i, j));
			dstBuf[i] = 
				(static_cast<uint8>(pixel) <<  0) | 
				(static_cast<uint8>(pixel) <<  8) |
				(static_cast<uint8>(pixel) << 16) |
				(0xFF << 24);
		}

		dstBuf += nDstPitch;
	}
}

void TexUploader_Psm8H(CGSHandler* gs, Framework::CBitmap& dst, const CGSHandler::TEX0& tex0, const CGSHandler::TEXA& texA)
{
	uint32 nPointer			= tex0.GetBufPtr();
	unsigned int nWidth		= tex0.GetWidth();
	unsigned int nHeight	= tex0.GetHeight();
	unsigned int nDstPitch	= dst.GetPitch() / 4;
	uint32* dstBuf			= reinterpret_cast<uint32*>(dst.GetPixels());

	CGsPixelFormats::CPixelIndexorPSMCT32 indexor(gs->GetRam(), nPointer, tex0.nBufWidth);

	for(unsigned int j = 0; j < nHeight; j++)
	{
		for(unsigned int i = 0; i < nWidth; i++)
		{
			uint8 pixel = static_cast<uint8>(indexor.GetPixel(i, j) >> 24);
			dstBuf[i] = 
				(static_cast<uint8>(pixel) <<  0) | 
				(static_cast<uint8>(pixel) <<  8) |
				(static_cast<uint8>(pixel) << 16) |
				(0xFF << 24);
		}

		dstBuf += nDstPitch;
	}
}

#define WNDSTYLE (WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN)

CGsContextView::CGsContextView(HWND parent, const RECT& rect, CGSHandler* gs)
: m_contextId(0)
, m_gs(gs)
{
	Create(0, Framework::Win32::CDefaultWndClass::GetName(), NULL, WNDSTYLE, rect, parent, NULL);
	SetClassPtr();

	m_mainSplitter = std::make_unique<Framework::Win32::CVerticalSplitter>(m_hWnd, GetClientRect());
	m_mainSplitter->SetEdgePosition(0.5f);

	m_bufferSelectionTab = std::make_unique<Framework::Win32::CTab>(*m_mainSplitter, Framework::Win32::CRect(0, 0, 1, 1), TCS_BOTTOM);
	m_bufferSelectionTab->InsertTab(_T("Framebuffer"));
	m_bufferSelectionTab->InsertTab(_T("Texture"));

	m_bufferView = std::make_unique<CPixelBufferView>(*m_bufferSelectionTab, Framework::Win32::CRect(0, 0, 1, 1));

	m_stateView = std::make_unique<CGsContextStateView>(*m_mainSplitter, GetClientRect(), m_contextId);
	m_stateView->Show(SW_SHOW);

	m_mainSplitter->SetChild(0, *m_bufferSelectionTab);
	m_mainSplitter->SetChild(1, *m_stateView);
}

CGsContextView::~CGsContextView()
{

}

void CGsContextView::UpdateState()
{
	UpdateBufferView();
	m_stateView->UpdateState(m_gs);
}

void CGsContextView::UpdateBufferView()
{
	if(m_bufferSelectionTab->GetSelection() == 0)
	{
		uint64 frameReg = m_gs->GetRegisters()[GS_REG_FRAME_1 + m_contextId];
		auto framebuffer = static_cast<CGSH_Direct3D9*>(m_gs)->GetFramebuffer(frameReg);
		m_bufferView->SetBitmap(framebuffer);
	}
	else if(m_bufferSelectionTab->GetSelection() == 1)
	{
		CGSHandler::TEX0 tex0;
		CGSHandler::TEXA texA;
		tex0 <<= m_gs->GetRegisters()[GS_REG_TEX0_1 + m_contextId];
		texA <<= m_gs->GetRegisters()[GS_REG_TEXA];

		auto texture = Framework::CBitmap(tex0.GetWidth(), tex0.GetHeight(), 32);

		switch(tex0.nPsm)
		{
		case CGSHandler::PSMT8:
			TexUploader_Psm8(m_gs, texture, tex0, texA);
			break;
		case CGSHandler::PSMT8H:
			TexUploader_Psm8H(m_gs, texture, tex0, texA);
			break;
		default:
			for(unsigned int i = 0; i < texture.GetPixelsSize() / 4; i++)
			{
				reinterpret_cast<uint32*>(texture.GetPixels())[i] = (0xFF << 24);
			}
			break;
		}

		m_bufferView->SetBitmap(texture);
	}
}

long CGsContextView::OnSize(unsigned int, unsigned int, unsigned int)
{
	m_mainSplitter->SetSizePosition(GetClientRect());
	return TRUE;
}

long CGsContextView::OnCommand(unsigned short, unsigned short, HWND hwndFrom)
{
	if(CWindow::IsCommandSource(m_mainSplitter.get(), hwndFrom))
	{
		m_bufferView->SetSizePosition(m_bufferSelectionTab->GetDisplayAreaRect());
	}
	return TRUE;
}

long CGsContextView::OnNotify(WPARAM wParam, NMHDR* hdr)
{
	if(CWindow::IsNotifySource(m_bufferSelectionTab.get(), hdr))
	{
		if(hdr->code == TCN_SELCHANGE)
		{
			UpdateBufferView();
			m_bufferView->FitBitmap();
		}
	}
	return FALSE;
}
