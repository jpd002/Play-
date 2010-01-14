#ifndef _CONTROLLERSETTINGSWND_H_
#define _CONTROLLERSETTINGSWND_H_

#include "win32/ModalWindow.h"
#include "win32/Button.h"
#include "win32/ListView.h"
#include "directinput/Manager.h"
#include "../ControllerInfo.h"
#include "win32/Layouts.h"

class CControllerSettingsWnd : public Framework::Win32::CModalWindow
{
public:
                                    CControllerSettingsWnd(HWND, DirectInput::CManager*);
    virtual                         ~CControllerSettingsWnd();

protected:
    long                            OnTimer(WPARAM);
    long                            OnCommand(unsigned short, unsigned short, HWND);
    long                            OnNotify(WPARAM, NMHDR*);

private:
    void                            RefreshLayout();
    void                            AutoConfigKeyboard();
    void                            AutoConfigJoystick();
    void                            InputEventHandler(PS2::CControllerInfo::BUTTON, uint32);
    void                            UpdateButtonValue(PS2::CControllerInfo::BUTTON, uint32);
    void                            UpdateBindings();
    void                            PopulateList();
    void                            OnListItemDblClick();

    Framework::LayoutObjectPtr      m_layout;
    Framework::Win32::CListView*    m_bindingList;
    Framework::Win32::CButton*      m_ok;
    Framework::Win32::CButton*      m_cancel;
    Framework::Win32::CButton*      m_autoConfigButton;
    DirectInput::CManager*          m_directInputManager;
    bool                            m_samplingEnabled;
};

#endif
