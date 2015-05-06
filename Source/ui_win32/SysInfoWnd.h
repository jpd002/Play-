#ifndef _SYSINFOWND_H_
#define _SYSINFOWND_H_

#include "win32/Window.h"
#include "win32/Static.h"
#include "win32/ListBox.h"
#include "layout/VerticalLayout.h"
#include "win32/ModalWindow.h"

class CSysInfoWnd : public Framework::Win32::CModalWindow
{
public:
									CSysInfoWnd(HWND);
									~CSysInfoWnd();

protected:
	long							OnTimer(WPARAM);

private:
	void							UpdateSchedulerInfo();
	void							UpdateProcessorFeatures();
	void							UpdateProcessor();

	void							RefreshLayout();

	static unsigned long WINAPI		ThreadRDTSC(void*);
	static const TCHAR*				m_sFeature[32];

	HANDLE							m_nRDTSCThread;

	Framework::Win32::CStatic*		m_pProcessor;
	Framework::Win32::CStatic*		m_pProcesses;
	Framework::Win32::CStatic*		m_pThreads;
	Framework::Win32::CListBox*		m_pFeatures;

	Framework::FlatLayoutPtr		m_pLayout;
};

#endif
