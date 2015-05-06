#ifndef _INPUTBINDINGSELECTIONWINDOW_H_
#define _INPUTBINDINGSELECTIONWINDOW_H_

#include "win32/ModalWindow.h"
#include "win32/Static.h"
#include "../../ControllerInfo.h"
#include "win32/Layouts.h"
#include "InputManager.h"

namespace PH_DirectInput
{
	class CInputBindingSelectionWindow : public Framework::Win32::CModalWindow
	{
	public:
											CInputBindingSelectionWindow(HWND, CInputManager&, PS2::CControllerInfo::BUTTON);
		virtual								~CInputBindingSelectionWindow();

	protected:
		long								OnActivate(unsigned int, bool, HWND);
		long								OnTimer(WPARAM);

	private:
		void								RefreshLayout();
		void								ProcessEvent(const GUID&, uint32, uint32);

		CInputManager&						m_inputManager;
		uint32								m_directInputManagerHandlerId;

		Framework::Win32::CStatic*			m_currentBindingLabel;
		PS2::CControllerInfo::BUTTON		m_button;

		GUID								m_selectedDevice;
		uint32								m_selectedId;
		uint32								m_selectedValue;
		bool								m_selected;

		Framework::LayoutObjectPtr			m_layout;

		bool								m_isActive;
	};
}

#endif
