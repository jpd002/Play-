#ifndef _PADLISTENER_H_
#define _PADLISTENER_H_

#include "Types.h"

class CPadListener
{
public:
	enum BUTTON
	{
		BUTTON_LEFT		= 0x8000,
		BUTTON_DOWN		= 0x4000,
		BUTTON_RIGHT	= 0x2000,
		BUTTON_UP		= 0x1000,
		BUTTON_START	= 0x0800,
		BUTTON_SELECT	= 0x0100,
		BUTTON_SQUARE	= 0x0080,
		BUTTON_CROSS	= 0x0040,
		BUTTON_CIRCLE	= 0x0020,
		BUTTON_TRIANGLE	= 0x0010,
	};

	virtual			~CPadListener() {}
	virtual void	SetButtonState(unsigned int, BUTTON, bool, uint8*) = 0;
};

#endif
