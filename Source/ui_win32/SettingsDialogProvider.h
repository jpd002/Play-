#ifndef _SETTINGSDIALOGPROVIDER_H_
#define _SETTINGSDIALOGPROVIDER_H_

#include "win32/ModalWindow.h"

class CSettingsDialogProvider
{
public:
    virtual Framework::Win32::CModalWindow*		CreateSettingsDialog(HWND) = 0;
	virtual void				                OnSettingsDialogDestroyed() = 0;
};

#endif
