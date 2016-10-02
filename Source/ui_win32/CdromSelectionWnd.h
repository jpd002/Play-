#pragma once

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
		BINDINGTYPE     nType;
		const char*     sImagePath;
		unsigned int    nPhysicalDevice;
	};

	CCdromSelectionWnd(HWND, const TCHAR*, CDROMBINDING*);

	bool    WasConfirmed() const;
	void    GetBindingInfo(CDROMBINDING*) const;

protected:
	long    OnCommand(unsigned short, unsigned short, HWND) override;

private:
	void    RefreshLayout();
	void    UpdateControls();
	void    PopulateDeviceList();
	void    SelectImage();

	bool            m_nConfirmed = false;
	BINDINGTYPE     m_nType = BINDING_IMAGE;
	std::string     m_sImagePath;
	unsigned int    m_nPhysicalDevice = 0;

	Framework::LayoutObjectPtr      m_pLayout;
	Framework::Win32::CButton*      m_pImageRadio = nullptr;
	Framework::Win32::CButton*      m_pDeviceRadio = nullptr;
	Framework::Win32::CButton*      m_pImageBrowse = nullptr;
	Framework::Win32::CEdit*        m_pImageEdit = nullptr;
	Framework::Win32::CComboBox*    m_pDeviceCombo = nullptr;
	Framework::Win32::CButton*      m_pOk = nullptr;
	Framework::Win32::CButton*      m_pCancel = nullptr;
};
