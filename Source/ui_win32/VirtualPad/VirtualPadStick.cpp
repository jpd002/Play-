#include <algorithm>
#include "VirtualPadStick.h"

CVirtualPadStick::~CVirtualPadStick()
{

}

void CVirtualPadStick::Draw(Gdiplus::Graphics& graphics)
{
	auto offsetRect = m_bounds;
	offsetRect.Offset(m_offset);
	graphics.DrawImage(m_image, offsetRect);
}

void CVirtualPadStick::OnMouseDown(int x, int y)
{
	m_pressPosition = Gdiplus::PointF(x, y);
	m_offset = Gdiplus::PointF(0, 0);
}

void CVirtualPadStick::OnMouseMove(int x, int y)
{
	auto radius = m_bounds.Width;
	auto offsetX = x - m_pressPosition.X;
	auto offsetY = y - m_pressPosition.Y;
	offsetX = std::min<float>(offsetX,  radius);
	offsetX = std::max<float>(offsetX, -radius);
	offsetY = std::min<float>(offsetY,  radius);
	offsetY = std::max<float>(offsetY, -radius);
	m_offset = Gdiplus::PointF(offsetX, offsetY);
	if(m_padHandler)
	{
		m_padHandler->SetAxisState(m_codeX, offsetX / radius);
		m_padHandler->SetAxisState(m_codeY, offsetY / radius);
	}
}

void CVirtualPadStick::OnMouseUp()
{
	m_offset = Gdiplus::PointF(0, 0);
	if(m_padHandler)
	{
		m_padHandler->SetAxisState(m_codeX, 0);
		m_padHandler->SetAxisState(m_codeY, 0);
	}
}

void CVirtualPadStick::SetCodeX(PS2::CControllerInfo::BUTTON codeX)
{
	m_codeX = codeX;
}

void CVirtualPadStick::SetCodeY(PS2::CControllerInfo::BUTTON codeY)
{
	m_codeY = codeY;
}
