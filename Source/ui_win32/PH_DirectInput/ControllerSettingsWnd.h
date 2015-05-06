#ifndef _CONTROLLERSETTINGSWND_H_
#define _CONTROLLERSETTINGSWND_H_

#include "win32/ModalWindow.h"
#include "win32/Button.h"
#include "win32/ListView.h"
#include "../../ControllerInfo.h"
#include "win32/Layouts.h"
#include "InputManager.h"

namespace PH_DirectInput
{
	class CControllerSettingsWnd : public Framework::Win32::CModalWindow
	{
	public:
											CControllerSettingsWnd(HWND, CInputManager&);
		virtual								~CControllerSettingsWnd();

	protected:
		long								OnCommand(unsigned short, unsigned short, HWND);
		long								OnNotify(WPARAM, NMHDR*);
		long								OnTimer(WPARAM);

	private:
		void								RefreshLayout();
		void								AutoConfigKeyboard();
		void								AutoConfigJoystick();
		void								UpdateBindings();
		void								UpdateBindingValues();
		void								PopulateList();
		void								OnListItemDblClick();

		Framework::LayoutObjectPtr			m_layout;
		Framework::Win32::CListView*		m_bindingList;
		Framework::Win32::CButton*			m_ok;
		Framework::Win32::CButton*			m_cancel;
		Framework::Win32::CButton*			m_autoConfigButton;
		CInputManager&						m_inputManager;

		bool								m_valuesCached;
		uint32								m_cachedValues[PS2::CControllerInfo::MAX_BUTTONS];
	};
}

#endif
