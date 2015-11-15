#ifndef _PH_DIRECTINPUT_H_
#define _PH_DIRECTINPUT_H_

#include "Types.h"
#include "SettingsDialogProvider.h"
#include "../PadHandler.h"
#include "../ControllerInfo.h"
#include "PH_DirectInput/InputManager.h"

class CPH_DirectInput : public CPadHandler, public CSettingsDialogProvider
{
public:
												CPH_DirectInput(HWND);
	virtual										~CPH_DirectInput();

	void										Update(uint8*);

	Framework::Win32::CWindow*					CreateSettingsDialog(HWND) override;
	void										OnSettingsDialogDestroyed() override;

	static FactoryFunction						GetFactoryFunction(HWND);

private:
	static CPadHandler*							PadHandlerFactory(HWND);

	PH_DirectInput::CInputManager				m_inputManager;
	HWND										m_hWnd;
};

#endif
