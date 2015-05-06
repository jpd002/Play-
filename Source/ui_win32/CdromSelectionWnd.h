#ifndef _CDROMSELECTIONWND_H_
#define _CDROMSELECTIONWND_H_

#include "win32/ModalWindow.h"
#include "win32/Button.h"
#include "win32/Edit.h"
#include "win32/ComboBox.h"
#include "layout/LayoutObject.h"

class CCdromSelectionWnd : public Framework::Win32::CModalWindow
{
public:
    enum BINDINGTYPE
    {
        BINDING_IMAGE = 1,
        BINDING_PHYSICAL = 2,
    };

    struct CDROMBINDING
    {
        BINDINGTYPE		nType;
        const char*		sImagePath;
        unsigned int	nPhysicalDevice;
    };

						            CCdromSelectionWnd(HWND, const TCHAR*, CDROMBINDING*);
						            ~CCdromSelectionWnd();
    bool							WasConfirmed();
    void							GetBindingInfo(CDROMBINDING*);

protected:
    long							OnCommand(unsigned short, unsigned short, HWND);

private:
    void							RefreshLayout();
    void							UpdateControls();
    void							PopulateDeviceList();
    void							SelectImage();

    bool							m_nConfirmed;
    BINDINGTYPE						m_nType;
    std::string     				m_sImagePath;
    unsigned int					m_nPhysicalDevice;

    Framework::LayoutObjectPtr      m_pLayout;
    Framework::Win32::CButton*      m_pImageRadio;
    Framework::Win32::CButton*      m_pDeviceRadio;
    Framework::Win32::CButton*      m_pImageBrowse;
    Framework::Win32::CEdit*        m_pImageEdit;
    Framework::Win32::CComboBox*    m_pDeviceCombo;
    Framework::Win32::CButton*      m_pOk;
    Framework::Win32::CButton*      m_pCancel;
};

#endif
