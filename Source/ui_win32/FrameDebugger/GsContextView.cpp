#include "GsContextView.h"
#include "GsStateUtils.h"
#include "../../gs/GsPixelFormats.h"
#include "../GSH_Direct3D9.h"
#include "win32/VerticalSplitter.h"

#define WNDSTYLE (WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN)

enum TAB_IDS
{
	TAB_ID_FRAMEBUFFER,
	TAB_ID_TEXTURE_BASE,
	TAB_ID_TEXTURE_MIP1,
	TAB_ID_TEXTURE_MIP2,
	TAB_ID_TEXTURE_MIP3,
	TAB_ID_TEXTURE_MIP4,
	TAB_ID_TEXTURE_MIP5,
	TAB_ID_TEXTURE_MIP6
};

CGsContextView::CGsContextView(HWND parent, const RECT& rect, CGSHandler* gs, unsigned int contextId)
: m_contextId(contextId)
, m_gs(gs)
{
	Create(0, Framework::Win32::CDefaultWndClass::GetName(), NULL, WNDSTYLE, rect, parent, NULL);
	SetClassPtr();

	m_mainSplitter = std::make_unique<Framework::Win32::CVerticalSplitter>(m_hWnd, GetClientRect());

	m_bufferSelectionTab = std::make_unique<Framework::Win32::CTab>(*m_mainSplitter, Framework::Win32::CRect(0, 0, 1, 1), TCS_BOTTOM);
	m_bufferSelectionTab->SetTabData(
		m_bufferSelectionTab->InsertTab(_T("Framebuffer")), TAB_ID_FRAMEBUFFER);
	m_bufferSelectionTab->SetTabData(
		m_bufferSelectionTab->InsertTab(_T("Texture (Base)")), TAB_ID_TEXTURE_BASE);
	m_bufferSelectionTab->SetTabData(
		m_bufferSelectionTab->InsertTab(_T("Texture (Mip 1)")), TAB_ID_TEXTURE_MIP1);
	m_bufferSelectionTab->SetTabData(
		m_bufferSelectionTab->InsertTab(_T("Texture (Mip 2)")), TAB_ID_TEXTURE_MIP2);
	m_bufferSelectionTab->SetTabData(
		m_bufferSelectionTab->InsertTab(_T("Texture (Mip 3)")), TAB_ID_TEXTURE_MIP3);
	m_bufferSelectionTab->SetTabData(
		m_bufferSelectionTab->InsertTab(_T("Texture (Mip 4)")), TAB_ID_TEXTURE_MIP4);
	m_bufferSelectionTab->SetTabData(
		m_bufferSelectionTab->InsertTab(_T("Texture (Mip 5)")), TAB_ID_TEXTURE_MIP5);
	m_bufferSelectionTab->SetTabData(
		m_bufferSelectionTab->InsertTab(_T("Texture (Mip 6)")), TAB_ID_TEXTURE_MIP6);

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
	int selectionIndex = m_bufferSelectionTab->GetSelection();
	uint32 selectedId = m_bufferSelectionTab->GetTabData(selectionIndex);
	switch(selectedId)
	{
	case TAB_ID_FRAMEBUFFER:
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
			CPixelBufferView::PixelBufferArray pixelBuffers;
			pixelBuffers.emplace_back("Raw", std::move(framebuffer));
			m_bufferView->SetPixelBuffers(std::move(pixelBuffers));
		}
		break;
	case TAB_ID_TEXTURE_BASE:
	case TAB_ID_TEXTURE_MIP1:
	case TAB_ID_TEXTURE_MIP2:
	case TAB_ID_TEXTURE_MIP3:
	case TAB_ID_TEXTURE_MIP4:
	case TAB_ID_TEXTURE_MIP5:
	case TAB_ID_TEXTURE_MIP6:
		{
			uint64 tex0Reg = m_gs->GetRegisters()[GS_REG_TEX0_1 + m_contextId];
			uint64 tex1Reg = m_gs->GetRegisters()[GS_REG_TEX1_1 + m_contextId];
			uint64 miptbp1Reg = m_gs->GetRegisters()[GS_REG_MIPTBP1_1 + m_contextId];
			uint64 miptbp2Reg = m_gs->GetRegisters()[GS_REG_MIPTBP2_1 + m_contextId];
			auto tex0 = make_convertible<CGSHandler::TEX0>(tex0Reg);
			auto tex1 = make_convertible<CGSHandler::TEX1>(tex1Reg);

			uint32 mipLevel = selectedId - TAB_ID_TEXTURE_BASE;
			Framework::CBitmap texture, clutTexture;

			if(mipLevel <= tex1.nMaxMip)
			{
				texture = static_cast<CGSH_Direct3D9*>(m_gs)->GetTexture(tex0Reg, tex1.nMaxMip, miptbp1Reg, miptbp2Reg, mipLevel);
			}

			if(!texture.IsEmpty() && CGsPixelFormats::IsPsmIDTEX(tex0.nPsm))
			{
				ColorArray convertedClut;
				m_gs->MakeLinearCLUT(tex0, convertedClut);
				clutTexture = LookupBitmap(texture, convertedClut);
			}

			if(!texture.IsEmpty() && CGsPixelFormats::IsPsmIDTEX4(tex0.nPsm))
			{
				//Too hard to see if pixel brightness is not boosted
				BrightenBitmap(texture);
			}

			CPixelBufferView::PixelBufferArray pixelBuffers;
			pixelBuffers.emplace_back("Raw", std::move(texture));
			if(!clutTexture.IsEmpty())
			{
				pixelBuffers.emplace_back("+ CLUT", std::move(clutTexture));
			}
			m_bufferView->SetPixelBuffers(std::move(pixelBuffers));
		}
		break;
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

void CGsContextView::BrightenBitmap(Framework::CBitmap& bitmap)
{
	assert(!bitmap.IsEmpty());
	assert(bitmap.GetBitsPerPixel() == 8);
	auto pixels = reinterpret_cast<uint8*>(bitmap.GetPixels());
	for(uint32 y = 0; y < bitmap.GetHeight(); y++)
	{
		for(uint32 x = 0; x < bitmap.GetWidth(); x++)
		{
			pixels[x] <<= 4;
		}
		pixels += bitmap.GetPitch();
	}
}

Framework::CBitmap CGsContextView::LookupBitmap(const Framework::CBitmap& srcBitmap, const ColorArray& clut)
{
	assert(!srcBitmap.IsEmpty());
	assert(srcBitmap.GetBitsPerPixel() == 8);
	auto dstBitmap = Framework::CBitmap(srcBitmap.GetWidth(), srcBitmap.GetHeight(), 32);
	auto srcPixels = reinterpret_cast<uint8*>(srcBitmap.GetPixels());
	auto dstPixels = reinterpret_cast<uint32*>(dstBitmap.GetPixels());
	for(uint32 y = 0; y < srcBitmap.GetHeight(); y++)
	{
		for(uint32 x = 0; x < srcBitmap.GetWidth(); x++)
		{
			uint8 index = srcPixels[x];
			uint32 color = clut[index];
			uint32 newColor = CGSH_Direct3D9::Color_Ps2ToDx9(color);
			dstPixels[x] = newColor;
		}
		srcPixels += srcBitmap.GetPitch();
		dstPixels += dstBitmap.GetPitch() / 4;
	}
	return std::move(dstBitmap);
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

LRESULT CGsContextView::OnNotify(WPARAM wParam, NMHDR* hdr)
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
