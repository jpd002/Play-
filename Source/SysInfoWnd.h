#ifndef _SYSINFOWND_H_
#define _SYSINFOWND_H_

#include "win32/Window.h"
#include "win32/Static.h"
#include "win32/ListBox.h"
#include "VerticalLayout.h"

class CSysInfoWnd : public Framework::CWindow
{
public:
									CSysInfoWnd(HWND);
									~CSysInfoWnd();

protected:
	long							OnTimer();
	long							OnSysCommand(unsigned int, LPARAM);

private:
	void							UpdateSchedulerInfo();
	void							UpdateProcessorFeatures();
	void							UpdateProcessor();

	void							RefreshLayout();

	static unsigned long WINAPI		ThreadRDTSC(void*);
	static const xchar*				m_sFeature[32];

	HANDLE							m_nRDTSCThread;

	Framework::CStatic*				m_pProcessor;
	Framework::CStatic*				m_pProcesses;
	Framework::CStatic*				m_pThreads;
	Framework::CListBox*			m_pFeatures;

	Framework::CVerticalLayout*		m_pLayout;
};

#endif
