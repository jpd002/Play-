#include "AcceleratorTable.h"

using namespace Framework::Win32;

CAcceleratorTable::CAcceleratorTable(HACCEL accel)
    : m_accel(accel)
{
}

CAcceleratorTable::~CAcceleratorTable()
{
	DestroyAcceleratorTable(m_accel);
}

CAcceleratorTable::operator HACCEL() const
{
	return m_accel;
}
