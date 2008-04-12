#ifndef _SAVEVIEW_H_
#define _SAVEVIEW_H_

#include <boost/thread.hpp>
#include "win32/Window.h"
#include "win32/Edit.h"
#include "win32/Button.h"
#include "win32/Layouts.h"
#include "IconView.h"
#include "CommandSink.h"
#include "EventEx.h"
#include "../saves/Save.h"
#include "../ThreadMsg.h"

class CSaveView : public Framework::Win32::CWindow
{
public:
										CSaveView(HWND);
										~CSaveView();

	void								SetSave(const CSave*);

	Framework::CEventEx<const CSave*>	m_OnDeleteClicked;

protected:
	long								OnSize(unsigned int, unsigned int, unsigned int);
	long								OnCommand(unsigned short, unsigned short, HWND);

private:
	enum ICONTYPE
	{
		ICON_NORMAL,
		ICON_DELETING,
		ICON_COPYING,
	};

	class CIconViewWnd : public Framework::Win32::CWindow
	{
	public:
										CIconViewWnd(HWND, RECT*);
										~CIconViewWnd();
		void							SetSave(const CSave*);
		void							SetIconType(ICONTYPE);

	protected:
		long							OnLeftButtonDown(int, int);
		long							OnLeftButtonUp(int, int);
		long							OnMouseWheel(short);
		long							OnMouseMove(WPARAM, int, int);
		long							OnSetCursor(HWND, unsigned int, unsigned int);

	private:
		void							ThreadProc();
		void							ThreadSetSave(const CSave*);
		void							ThreadSetIconType(ICONTYPE);
		void							LoadIcon();
		void							ChangeCursor();
		void							Render(HDC);
		void							DrawBackground();

		HGLRC							m_hRC;
		boost::thread*					m_pThread;
		static PIXELFORMATDESCRIPTOR	m_PFD;
		CThreadMsg						m_MailSlot;
		const CSave*					m_pSave;
		CIconView*						m_pIconView;
		ICONTYPE						m_nIconType;
		
		double							m_nRotationX;
		double							m_nRotationY;

		bool							m_nGrabbing;
		int								m_nGrabPosX;
		int								m_nGrabPosY;
		int								m_nGrabDistX;
		int								m_nGrabDistY;
		double							m_nGrabRotX;
		double							m_nGrabRotY;
		double							m_nZoom;

		enum THREADMSG
		{
			THREAD_END,
			THREAD_SETSAVE,
			THREAD_SETICONTYPE,
		};
	};

	void								RefreshLayout();

	long								SetIconType(ICONTYPE);
	long								OpenSaveFolder();
	long								Export();
	long								Delete();

	CCommandSink						m_CommandSink;

	const CSave*						m_pSave;
    Framework::FlatLayoutPtr			m_pLayout;
	Framework::Win32::CEdit*			m_pNameLine1;
	Framework::Win32::CEdit*			m_pNameLine2;
	Framework::Win32::CEdit*			m_pSize;
	Framework::Win32::CEdit*			m_pId;
	Framework::Win32::CEdit*			m_pLastModified;
	Framework::Win32::CButton*			m_pOpenFolder;
	Framework::Win32::CButton*			m_pExport;
	Framework::Win32::CButton*			m_pDelete;
	Framework::Win32::CButton*			m_pNormalIcon;
	Framework::Win32::CButton*			m_pCopyingIcon;
	Framework::Win32::CButton*			m_pDeletingIcon;
	CIconViewWnd*						m_pIconViewWnd;
	ICONTYPE							m_nIconType;
};

#endif
