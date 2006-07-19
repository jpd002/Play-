#ifndef _SAVEVIEW_H_
#define _SAVEVIEW_H_

#include "win32/Window.h"
#include "win32/Edit.h"
#include "win32/Button.h"
#include "GridLayout.h"
#include "../saves/Save.h"

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
	void						RefreshLayout();
	void						OpenSaveFolder();

	const CSave*				m_pSave;
	Framework::CGridLayout*		m_pLayout;
	Framework::Win32::CEdit*	m_pNameLine1;
	Framework::Win32::CEdit*	m_pNameLine2;
	Framework::Win32::CEdit*	m_pSize;
	Framework::Win32::CEdit*	m_pId;
	Framework::Win32::CButton*	m_pOpenFolder;
};

#endif
