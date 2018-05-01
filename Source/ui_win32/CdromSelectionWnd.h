#pragma once

#include "win32/Dialog.h"
#include "win32/Button.h"
#include "win32/Edit.h"
#include "win32/ComboBox.h"

class CCdromSelectionWnd : public Framework::Win32::CDialog
{
public:
	enum BINDINGTYPE
	{
		BINDING_IMAGE = 1,
		BINDING_PHYSICAL = 2,
	};

	struct CDROMBINDING
	{
		BINDINGTYPE type = BINDING_IMAGE;
		std::tstring imagePath;
		unsigned int physicalDevice = 0;
	};

	CCdromSelectionWnd(HWND, const TCHAR*);

	bool WasConfirmed() const;

	CDROMBINDING GetBindingInfo() const;
	void SetBindingInfo(const CDROMBINDING&);

protected:
	long OnCommand(unsigned short, unsigned short, HWND) override;

private:
	void UpdateControls();
	void PopulateDeviceList();
	void SelectImage();

	bool m_confirmed = false;
	CDROMBINDING m_binding;

	Framework::Win32::CButton m_imageRadio;
	Framework::Win32::CButton m_imageBrowse;
	Framework::Win32::CEdit m_imageEdit;
	Framework::Win32::CButton m_deviceRadio;
	Framework::Win32::CComboBox m_deviceCombo;
};
