#include "VirtualPadButton.h"

CVirtualPadButton::~CVirtualPadButton()
{

}

void CVirtualPadButton::Draw(Gdiplus::Graphics& graphics)
{
	if(m_image != nullptr)
	{
		Gdiplus::RectF dstRect(m_bounds.Left(), m_bounds.Top(), m_bounds.Width(), m_bounds.Height());

		if(m_pressed)
		{
			Gdiplus::ColorMatrix matrix;
			memset(&matrix, 0, sizeof(Gdiplus::ColorMatrix));
			matrix.m[0][0] = 0.75;
			matrix.m[1][1] = 0.75;
			matrix.m[2][2] = 0.75;
			matrix.m[3][3] = 1;
			matrix.m[4][4] = 1;

			Gdiplus::ImageAttributes attr;
			attr.SetColorMatrix(&matrix);

			Gdiplus::RectF imageSize;
			Gdiplus::Unit imageUnit;
			m_image->GetBounds(&imageSize, &imageUnit);
			graphics.DrawImage(m_image, dstRect, imageSize.X, imageSize.Y, imageSize.Width, imageSize.Height, imageUnit, &attr);
		}
		else
		{
			graphics.DrawImage(m_image, dstRect);
		}
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
