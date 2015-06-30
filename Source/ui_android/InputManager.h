#pragma once

#include "Types.h"
#include "Singleton.h"

class CInputManager : public CSingleton<CInputManager>
{
public:
	void		SetButtonState(int, bool);
	void		SetAxisState(int, float);
};
