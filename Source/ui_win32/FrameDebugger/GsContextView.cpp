#include "GsContextView.h"
#include "GsStateUtils.h"
#include "../../gs/GsPixelFormats.h"
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

	m_bufferSelectionTab = std::make_unique<Framework::Win32::CTab>(*m_mainSplitter, Framework::Win32::CRect(0, 0, 1, 1), TCS_BOTTOM);
	m_bufferSelectionTab->InsertTab(_T("Framebuffer"));
	m_bufferSelectionTab->InsertTab(_T("Texture"));

	m_bufferView = std::make_unique<CPixelBufferView>(*m_bufferSelectionTab, Framework::Win32::CRect(0, 0, 1, 1));

	m_stateView = std::make_unique<CGsContextStateView>(*m_mainSplitter, GetClientRect(), m_contextId);
	m_stateView->Show(SW_SHOW);

	m_mainSplitter->SetChild(0, *m_bufferSelectionTab);
	m_mainSplitter->SetChild(1, *m_stateView);
	m_mainSplitter->SetMasterChild(1);
	m_mainSplitter->SetEdgePosition(0.5f);
}

CGsContextView::~CGsContextView()
{

}

void CGsContextView::SetFbDisplayMode(FB_DISPLAY_MODE fbDisplayMode)
{
	m_fbDisplayMode = fbDisplayMode;
	UpdateBufferView();
}

void CGsContextView::UpdateState(CGSHandler* gs, CGsPacketMetadata*, DRAWINGKICK_INFO* drawingKick)
{
	assert(gs == m_gs);
	m_drawingKick = (*drawingKick);
	UpdateBufferView();
	m_stateView->UpdateState(m_gs);
}

void CGsContextView::UpdateBufferView()
{
	if(m_bufferSelectionTab->GetSelection() == 0)
	{
		uint64 frameReg = m_gs->GetRegisters()[GS_REG_FRAME_1 + m_contextId];
		auto framebuffer = static_cast<CGSH_Direct3D9*>(m_gs)->GetFramebuffer(frameReg);
		if(!framebuffer.IsEmpty())
		{
			RenderDrawKick(framebuffer);
			if(m_fbDisplayMode == FB_DISPLAY_MODE_448P)
			{
				framebuffer = framebuffer.ResizeCanvas(640, 448);
			}
			else if(m_fbDisplayMode == FB_DISPLAY_MODE_448I)
			{
				framebuffer = framebuffer.ResizeCanvas(640, 224);
				framebuffer = framebuffer.Resize(640, 448);
			}
		}
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

void CGsContextView::RenderDrawKick(Framework::CBitmap& bitmap)
{
	if(m_drawingKick.primType == CGSHandler::PRIM_INVALID) return;
	if(m_drawingKick.context != m_contextId) return;

	auto primHighlightColor = Framework::CColor(0, 0xFF, 0, 0xFF);

	switch(m_drawingKick.primType)
	{
	case CGSHandler::PRIM_TRIANGLE:
	case CGSHandler::PRIM_TRIANGLESTRIP:
	case CGSHandler::PRIM_TRIANGLEFAN:
		{
			int x1 = static_cast<int16>(m_drawingKick.vertex[0].x) / 16;
			int y1 = static_cast<int16>(m_drawingKick.vertex[0].y) / 16;
			int x2 = static_cast<int16>(m_drawingKick.vertex[1].x) / 16;
			int y2 = static_cast<int16>(m_drawingKick.vertex[1].y) / 16;
			int x3 = static_cast<int16>(m_drawingKick.vertex[2].x) / 16;
			int y3 = static_cast<int16>(m_drawingKick.vertex[2].y) / 16;
			bitmap.DrawLine(x1, y1, x2, y2, primHighlightColor);
			bitmap.DrawLine(x1, y1, x3, y3, primHighlightColor);
			bitmap.DrawLine(x2, y2, x3, y3, primHighlightColor);
		}
		break;
	case CGSHandler::PRIM_SPRITE:
		{
			int x1 = static_cast<int16>(m_drawingKick.vertex[0].x) / 16;
			int y1 = static_cast<int16>(m_drawingKick.vertex[0].y) / 16;
			int x2 = static_cast<int16>(m_drawingKick.vertex[1].x) / 16;
			int y2 = static_cast<int16>(m_drawingKick.vertex[1].y) / 16;
			bitmap.DrawLine(x1, y1, x1, y2, primHighlightColor);
			bitmap.DrawLine(x1, y2, x2, y2, primHighlightColor);
			bitmap.DrawLine(x2, y2, x2, y1, primHighlightColor);
			bitmap.DrawLine(x2, y1, x1, y1, primHighlightColor);
		}
		break;
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
