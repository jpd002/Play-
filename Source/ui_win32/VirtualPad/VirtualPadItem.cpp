#include "VirtualPadItem.h"

CVirtualPadItem::~CVirtualPadItem()
{

}

void CVirtualPadItem::SetBounds(const Framework::Win32::CRect& bounds)
{
	m_bounds = bounds;
}
