#ifndef _LOG_H_
#define _LOG_H_

#include "win32/Edit.h"
#include "win32/MDIChild.h"

class CLog : public Framework::CMDIChild
{
public:
								CLog(HWND);
								~CLog();

protected:
	long						OnSize(unsigned int, unsigned int, unsigned int);
	long						OnSysCommand(unsigned int, LPARAM);
	long						OnSetFocus();
	long						OnWndProc(unsigned int, WPARAM, LPARAM);

private:
	void						WaitForWindowCreation();
	void						RefreshLayout();

	unsigned long				WndThreadProc();
	static unsigned long WINAPI	WndThread(void*);

	unsigned long				PipeThreadProc();
	static unsigned long WINAPI	PipeThread(void*);

	HANDLE						m_nStdoutWr;
	HANDLE						m_nStdoutRd;
	HANDLE						m_nPipeThread;

	HANDLE						m_nWindowCreatedEvent;
	HANDLE						m_nWndThread;

	HWND						m_hParent;

	unsigned long				m_nKillThread;
	Framework::CEdit*			m_pEdit;
};

#endif