#include <algorithm>
#include "VirtualPadStick.h"

CVirtualPadStick::~CVirtualPadStick()
{

}

void CVirtualPadStick::Draw(Gdiplus::Graphics& graphics)
{
	auto offsetRect = m_bounds;
	OffsetRect(offsetRect, m_offset.x, m_offset.y);
	graphics.DrawImage(m_image, offsetRect.Left(), offsetRect.Top(), offsetRect.Width(), offsetRect.Height());
}

void CVirtualPadStick::OnMouseDown(int x, int y)
{
	m_pressPosition.x = x;
	m_pressPosition.y = y;
	m_offset.x = 0;
	m_offset.y = 0;
}

void CVirtualPadStick::OnMouseMove(int x, int y)
{
	int radius = m_bounds.Width();
	int offsetX = x - m_pressPosition.x;
	int offsetY = y - m_pressPosition.y;
	offsetX = std::min<int>(offsetX,  radius);
	offsetX = std::max<int>(offsetX, -radius);
	offsetY = std::min<int>(offsetY,  radius);
	offsetY = std::max<int>(offsetY, -radius);
	m_offset.x = offsetX;
	m_offset.y = offsetY;
}

void CVirtualPadStick::OnMouseUp()
{
	m_offset.x = 0;
	m_offset.y = 0;
}
