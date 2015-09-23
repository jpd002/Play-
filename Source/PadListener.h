#pragma once

#include "Types.h"
#include "ControllerInfo.h"

class CPadListener
{
public:
	virtual			~CPadListener() {}
	virtual void	SetButtonState(unsigned int, PS2::CControllerInfo::BUTTON, bool, uint8*) = 0;
	virtual void	SetAxisState(unsigned int, PS2::CControllerInfo::BUTTON, uint8, uint8*) = 0;
	static uint32	GetButtonMask(PS2::CControllerInfo::BUTTON);
};
