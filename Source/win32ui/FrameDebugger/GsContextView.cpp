#include "GsContextView.h"
#include "GsStateUtils.h"
#include "../../GsPixelFormats.h"
#include "../GSH_Direct3D9.h"
#include "win32/VerticalSplitter.h"

#define WNDSTYLE (WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN)

CGsContextView::CGsContextView(HWND parent, const RECT& rect, CGSHandler* gs, unsigned int contextId)
: m_contextId(contextId)
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

void CGsContextView::UpdateState(CGSHandler* gs, CGsPacketMetadata*)
{
	assert(gs == m_gs);
	UpdateBufferView();
	m_stateView->UpdateState(m_gs);
}

void CGsContextView::UpdateBufferView()
{
	if(m_bufferSelectionTab->GetSelection() == 0)
	{
		uint64 frameReg = m_gs->GetRegisters()[GS_REG_FRAME_1 + m_contextId];
		auto framebuffer = static_cast<CGSH_Direct3D9*>(m_gs)->GetFramebuffer(frameReg);
		framebuffer = framebuffer.ResizeCanvas(640, 200);
		framebuffer = framebuffer.Resize(640, 400);
		m_bufferView->SetBitmap(framebuffer);
	}
	else if(m_bufferSelectionTab->GetSelection() == 1)
	{
		uint64 tex0Reg = m_gs->GetRegisters()[GS_REG_TEX0_1 + m_contextId];
		uint64 tex1Reg = m_gs->GetRegisters()[GS_REG_TEX1_1 + m_contextId];
		uint64 clampReg = m_gs->GetRegisters()[GS_REG_CLAMP_1 + m_contextId];

		auto texture = static_cast<CGSH_Direct3D9*>(m_gs)->GetTexture(tex0Reg, tex1Reg, clampReg);

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
