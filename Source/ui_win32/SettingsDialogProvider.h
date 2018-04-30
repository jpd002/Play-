#pragma once

#include "win32/Window.h"

class CSettingsDialogProvider
{
public:
	virtual ~CSettingsDialogProvider()
	{
	}
	virtual Framework::Win32::CWindow* CreateSettingsDialog(HWND) = 0;
	virtual void OnSettingsDialogDestroyed() = 0;
};
