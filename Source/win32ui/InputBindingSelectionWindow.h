#ifndef _INPUTBINDINGSELECTIONWINDOW_H_
#define _INPUTBINDINGSELECTIONWINDOW_H_

#include "win32/ModalWindow.h"
#include "win32/Static.h"
#include "../ControllerInfo.h"
#include "directinput/Manager.h"
#include "win32/Layouts.h"
#include "InputConfig.h"

class CInputBindingSelectionWindow : public Framework::Win32::CModalWindow
{
public:
                                    CInputBindingSelectionWindow(HWND, DirectInput::CManager*, PS2::CControllerInfo::BUTTON);
    virtual                         ~CInputBindingSelectionWindow();

protected:
    long                            OnTimer(WPARAM);
    long                            OnActivate(unsigned int, bool, HWND);

private:
    void                            RefreshLayout();
    void                            ProcessEvent(const GUID&, uint32, uint32);

    Framework::Win32::CStatic*      m_currentBindingLabel;
    DirectInput::CManager*          m_directInputManager;
    PS2::CControllerInfo::BUTTON    m_button;

    GUID                            m_selectedDevice;
    uint32                          m_selectedId;
    bool                            m_selected;

    Framework::LayoutObjectPtr      m_layout;

    const CInputConfig::CBinding*   m_binding;
    bool                            m_isActive;
};

#endif
