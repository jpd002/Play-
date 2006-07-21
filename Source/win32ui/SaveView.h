#ifndef _SAVEVIEW_H_
#define _SAVEVIEW_H_

#include <boost/thread.hpp>
#include "win32/Window.h"
#include "win32/Edit.h"
#include "win32/Button.h"
#include "win32/Layouts.h"
#include "../saves/Save.h"
#include "../ThreadMsg.h"

class CSaveView : public Framework::CWindow
{
public:
								CSaveView(HWND);
								~CSaveView();

	void						SetSave(const CSave*);

protected:
	long						OnSize(unsigned int, unsigned int, unsigned int);
	long						OnCommand(unsigned short, unsigned short, HWND);

private:
	class CIconView : public Framework::CWindow
	{
	public:
										CIconView(HWND, RECT*);
										~CIconView();

	private:
		void							ThreadProc();
		void							Render(HDC);

		HGLRC							m_hRC;
		boost::thread*					m_pThread;
		static PIXELFORMATDESCRIPTOR	m_PFD;
		CThreadMsg						m_MailSlot;

		enum THREADMSG
		{
			THREAD_END,
			THREAD_SETSAVE,
		};
	};

	void						RefreshLayout();
	void						OpenSaveFolder();

	const CSave*				m_pSave;
	Framework::CVerticalLayout*	m_pLayout;
	Framework::Win32::CEdit*	m_pNameLine1;
	Framework::Win32::CEdit*	m_pNameLine2;
	Framework::Win32::CEdit*	m_pSize;
	Framework::Win32::CEdit*	m_pId;
	Framework::Win32::CEdit*	m_pLastModified;
	Framework::Win32::CButton*	m_pOpenFolder;
	Framework::Win32::CButton*	m_pExport;
	Framework::Win32::CButton*	m_pDelete;
	Framework::Win32::CButton*	m_pNormalIcon;
	Framework::Win32::CButton*	m_pCopyingIcon;
	Framework::Win32::CButton*	m_pDeletingIcon;
	CIconView*					m_pIconView;
};

#endif
