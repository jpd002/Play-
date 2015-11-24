#include "VirtualPadItem.h"

CVirtualPadItem::~CVirtualPadItem()
{

}

void CVirtualPadItem::OnMouseDown(int, int)
{

}

void CVirtualPadItem::OnMouseMove(int, int)
{

}

void CVirtualPadItem::OnMouseUp()
{

}

void CVirtualPadItem::SetPadHandler(CPH_Generic* padHandler)
{
	m_padHandler = padHandler;
}

Gdiplus::RectF CVirtualPadItem::GetBounds() const
{
	return m_bounds;
}

void CVirtualPadItem::SetBounds(const Gdiplus::RectF& bounds)
{
	m_bounds = bounds;
}

void CVirtualPadItem::SetImage(Gdiplus::Bitmap* image)
{
	m_image = image;
}

uint32 CVirtualPadItem::GetPointerId() const
{
	return m_pointerId;
}

void CVirtualPadItem::SetPointerId(uint32 pointerId)
{
	m_pointerId = pointerId;
}
