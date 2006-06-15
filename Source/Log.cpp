#include <stdio.h>
#include <io.h>
#include "Log.h"
#include "PtrMacro.h"

#define CLSNAME		_X("CLog")
#define BUFFERSIZE	0x1000
#define WM_FINISH	(WM_USER + 1)

using namespace Framework;

CLog::CLog(HWND hParent)
{
	unsigned long nTID;

	m_nWindowCreatedEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

	m_hParent = hParent;
	m_nWndThread = CreateThread(NULL, NULL, WndThread, this, NULL, &nTID);
	WaitForWindowCreation();
}

CLog::~CLog()
{
	m_nKillThread = 1;
	printf("\r\n");
	WaitForSingleObject(m_nPipeThread, INFINITE);
	DELETEPTR(m_pEdit);

//	SendMessage(m_hWnd, WM_FINISH, 0, 0);
//	WaitForSingleObject(m_nWndThread, INFINITE);
}

void CLog::WaitForWindowCreation()
{
	unsigned long nStatus;
	MSG wmmsg;

	while(1)
	{
		nStatus = MsgWaitForMultipleObjects(1, &m_nWindowCreatedEvent, FALSE, INFINITE, QS_SENDMESSAGE);
		if(nStatus == WAIT_OBJECT_0) break;
		//Process the message in queue to prevent a possible deadlock
		if(PeekMessage(&wmmsg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&wmmsg);
			DispatchMessage(&wmmsg);
		}
	}
	
	CloseHandle(m_nWindowCreatedEvent);
}

void CLog::RefreshLayout()
{
	RECT rc;

	if(m_pEdit == NULL) return;

	GetClientRect(&rc);

	m_pEdit->SetSize(rc.right, rc.bottom);
}

long CLog::OnSize(unsigned int nType, unsigned int nCX, unsigned int nCY)
{
	RefreshLayout();
	return TRUE;
}

long CLog::OnSetFocus()
{
	if(m_pEdit != NULL)
	{
		m_pEdit->SetFocus();
	}
	return FALSE;
}

long CLog::OnSysCommand(unsigned int nCmd, LPARAM lParam)
{
	switch(nCmd)
	{
	case SC_CLOSE:
		Show(SW_HIDE);
		return FALSE;
	}
	return TRUE;
}

long CLog::OnWndProc(unsigned int nMsg, WPARAM wParam, LPARAM lParam)
{
	switch(nMsg)
	{
	case WM_FINISH:
		DestroyWindow(m_hWnd);
		break;
	}
	return CMDIChild::OnWndProc(nMsg, wParam, lParam);
}

unsigned long CLog::WndThreadProc()
{
	unsigned long nTID;
	HFONT nFont;
	RECT rc;

	if(!DoesWindowClassExist(CLSNAME))
	{
		WNDCLASSEX wc;
		memset(&wc, 0, sizeof(WNDCLASSEX));
		wc.cbSize			= sizeof(WNDCLASSEX);
		wc.hCursor			= LoadCursor(NULL, IDC_ARROW);
		wc.hbrBackground	= (HBRUSH)(COLOR_WINDOW); 
		wc.hInstance		= GetModuleHandle(NULL);
		wc.lpszClassName	= CLSNAME;
		wc.lpfnWndProc		= CWindow::WndProc;
		RegisterClassEx(&wc);
	}
	
	SetRect(&rc, 0, 0, 320, 240);

	Create(NULL, CLSNAME, _X("Log"), WS_CLIPCHILDREN | WS_THICKFRAME | WS_CAPTION | WS_SYSMENU | WS_CHILD | WS_MAXIMIZEBOX, &rc, m_hParent, NULL);
	SetClassPtr();

	SetEvent(m_nWindowCreatedEvent);

	SetRect(&rc, 0, 0, 1, 1);
	m_pEdit = new CEdit(m_hWnd, &rc, _X(""), ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL | WS_VSCROLL | WS_HSCROLL);

	nFont = CreateFont(-11, 0, 0, 0, 400, 0, 0, 0, 0, 3, 2, 1, 49, _X("Courier New"));
	m_pEdit->SetFont(nFont);

	RefreshLayout();
	m_pEdit->SetFocus();
	m_pEdit->SetTextLimit(-1);

	CreatePipe(&m_nStdoutRd, &m_nStdoutWr, NULL, 0);
	*stdout = *_fdopen(_open_osfhandle((intptr_t)m_nStdoutWr, 0x4000), "w");
	setvbuf(stdout, NULL, _IONBF, 0);

	m_nKillThread = 0;
	m_nPipeThread = CreateThread(NULL, NULL, PipeThread, this, NULL, &nTID);

	CWindow::StdMsgLoop(this);

	return 0xDEADBEEF;
}

unsigned long CLog::PipeThreadProc()
{
	unsigned long nRead;
	unsigned int nLenght;
	unsigned int nBufferPtr;
	char* pBuffer;

	pBuffer = (char*)malloc(BUFFERSIZE + 1);
	nBufferPtr = 0;

	while(!m_nKillThread)
	{

		ReadFile(m_nStdoutRd, pBuffer + nBufferPtr, BUFFERSIZE - nBufferPtr, &nRead, NULL);

		nBufferPtr += nRead;
		pBuffer[nBufferPtr] = 0x00;

		if(strchr(pBuffer, '\n') != NULL)
		{
			//Flush the buffer
			nLenght = m_pEdit->GetTextLength();
			m_pEdit->SetSelection(nLenght, nLenght);
			m_pEdit->ReplaceSelectionA(false, pBuffer);
			nBufferPtr = 0;
		}

	}

	FREEPTR(pBuffer);

	return 0xDEADBEEF;
}

unsigned long WINAPI CLog::WndThread(void* pParam)
{
	return ((CLog*)pParam)->WndThreadProc();
}

unsigned long WINAPI CLog::PipeThread(void* pParam)
{
	return ((CLog*)pParam)->PipeThreadProc();
}
