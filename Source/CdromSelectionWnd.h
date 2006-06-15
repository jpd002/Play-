#ifndef _CDROMSELECTIONWND_H_
#define _CDROMSELECTIONWND_H_

#include "ModalWindow.h"
#include "win32/Button.h"
#include "win32/Edit.h"
#include "win32/ComboBox.h"
#include "VerticalLayout.h"
#include "Str.h"

class CCdromSelectionWnd : public CModalWindow
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

									CCdromSelectionWnd(HWND, const xchar*, CDROMBINDING*);
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
	Framework::CStrA				m_sImagePath;
	unsigned int					m_nPhysicalDevice;

	Framework::CVerticalLayout*		m_pLayout;
	Framework::CButton*				m_pImageRadio;
	Framework::CButton*				m_pDeviceRadio;
	Framework::CButton*				m_pImageBrowse;
	Framework::CEdit*				m_pImageEdit;
	Framework::CComboBox*			m_pDeviceCombo;
	Framework::CButton*				m_pOk;
	Framework::CButton*				m_pCancel;
};

#endif
