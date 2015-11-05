#include "VirtualPadButton.h"

CVirtualPadButton::~CVirtualPadButton()
{

}

void CVirtualPadButton::Draw(Gdiplus::Graphics& graphics)
{
	if(m_pressed)
	{
		Gdiplus::SolidBrush brush(Gdiplus::Color(255, 255, 0, 255));
		graphics.FillRectangle(&brush, m_bounds.Left(), m_bounds.Top(), m_bounds.Width(), m_bounds.Height());
	}
	else
	{
		Gdiplus::SolidBrush brush(Gdiplus::Color(255, 0, 0, 255));
		graphics.FillRectangle(&brush, m_bounds.Left(), m_bounds.Top(), m_bounds.Width(), m_bounds.Height());
	}

	if(!m_caption.empty())
	{
		Gdiplus::SolidBrush fontBrush(Gdiplus::Color(255, 255, 255, 255));
		Gdiplus::Font font(&Gdiplus::FontFamily(L"Courier New"), 10);
		graphics.DrawString(m_caption.c_str(), m_caption.size(), &font, 
			Gdiplus::PointF(m_bounds.Left(), m_bounds.Top()), &fontBrush);
	}
}

void CVirtualPadButton::OnMouseDown(int x, int y)
{
	m_pressed = true;
}

void CVirtualPadButton::OnMouseUp()
{
	m_pressed = false;
}

void CVirtualPadButton::SetCaption(const std::wstring& caption)
{
	m_caption = caption;
}
