#include <assert.h>
#include "PadListener.h"

using namespace PS2;

uint32 CPadListener::GetButtonMask(CControllerInfo::BUTTON button)
{
	static uint32 buttonMask[CControllerInfo::MAX_BUTTONS] =
	{
		static_cast<uint32>(-1),
		static_cast<uint32>(-1),
		static_cast<uint32>(-1),
		static_cast<uint32>(-1),
		0x1000,
		0x4000,
		0x8000,
		0x2000,
		0x0100,
		0x0800,
		0x0080,
		0x0010,
		0x0020,
		0x0040,
		0x0004,
		0x0001,
		0x0008,
		0x0002,
	};

	uint32 result = buttonMask[button];
	assert(result != -1);
	return result;
}
