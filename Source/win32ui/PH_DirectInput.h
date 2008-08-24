#ifndef _PH_DIRECTINPUT_H_
#define _PH_DIRECTINPUT_H_

#include "directinput/Manager.h"
#include "Types.h"
#include "SettingsDialogProvider.h"
#include "../PadHandler.h"
#include "../ControllerInfo.h"

class CPH_DirectInput : public CPadHandler, public CSettingsDialogProvider
{
public:
							                    CPH_DirectInput(HWND);
    virtual                                     ~CPH_DirectInput();

    void					                    Update(uint8*);
    DirectInput::CManager*                      GetManager() const;

    virtual Framework::Win32::CModalWindow*     CreateSettingsDialog(HWND);
	virtual void                                OnSettingsDialogDestroyed();

    static FactoryFunction                      GetFactoryFunction(HWND);

private:
	static CPadHandler*		                    PadHandlerFactory(HWND);
	void					                    Initialize();
    void                                        ProcessEvents(PS2::CControllerInfo::BUTTON, uint32, uint8*);

    DirectInput::CManager*                      m_manager;

	HWND					                    m_hWnd;
};

#endif
