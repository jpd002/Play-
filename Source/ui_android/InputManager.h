#pragma once

#include "Singleton.h"
#include "Types.h"

class CInputManager : public CSingleton<CInputManager>
{
public:
	void SetButtonState(int, bool);
	void SetAxisState(int, float);
};
