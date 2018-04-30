#include "VirtualPadButton.h"

CVirtualPadButton::CVirtualPadButton()
    : m_font(&Gdiplus::FontFamily(L"Arial"), 10)
{
}

CVirtualPadButton::~CVirtualPadButton()
{
}

void CVirtualPadButton::Draw(Gdiplus::Graphics& graphics)
{
	if(m_image != nullptr)
	{
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
			graphics.DrawImage(m_image, m_bounds, imageSize.X, imageSize.Y, imageSize.Width, imageSize.Height, imageUnit, &attr);
		}
		else
		{
			graphics.DrawImage(m_image, m_bounds);
		}
	}

	if(!m_caption.empty())
	{
		Gdiplus::SolidBrush fontBrush(Gdiplus::Color(255, 255, 255, 255));

		Gdiplus::RectF boundingBox(0, 0, 0, 0);
		graphics.MeasureString(m_caption.c_str(), m_caption.size(), &m_font,
		                       Gdiplus::PointF(0, 0), &boundingBox);
		Gdiplus::PointF textOrigin(
		    m_bounds.X + (m_bounds.Width - boundingBox.Width) / 2,
		    m_bounds.Y + (m_bounds.Height - boundingBox.Height) / 2);
		graphics.DrawString(m_caption.c_str(), m_caption.size(), &m_font, textOrigin, &fontBrush);
	}
}

void CVirtualPadButton::OnMouseDown(int x, int y)
{
	m_pressed = true;
	if(m_padHandler)
	{
		m_padHandler->SetButtonState(m_code, m_pressed);
	}
}

void CVirtualPadButton::OnMouseUp()
{
	m_pressed = false;
	if(m_padHandler)
	{
		m_padHandler->SetButtonState(m_code, m_pressed);
	}
}

void CVirtualPadButton::SetCode(PS2::CControllerInfo::BUTTON code)
{
	m_code = code;
}

void CVirtualPadButton::SetCaption(const std::wstring& caption)
{
	m_caption = caption;
}
