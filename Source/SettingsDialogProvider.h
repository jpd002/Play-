#ifndef _SETTINGSDIALOGPROVIDER_H_
#define _SETTINGSDIALOGPROVIDER_H_

#include "ModalWindow.h"

class CSettingsDialogProvider
{
public:
	virtual CModalWindow*		CreateSettingsDialog(HWND) = 0;
	virtual void				OnSettingsDialogDestroyed() = 0;
};

#endif
