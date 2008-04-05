#include <windows.h>
#include <tlhelp32.h>
#include <boost/lexical_cast.hpp>
#include "Types.h"
#include "string_cast.h"
#include "SysInfoWnd.h"
#include "PtrMacro.h"
#include "win32/LayoutWindow.h"
#ifdef AMD64
#include "../amd64/CPUID.h"
#endif

#define CLSNAME			_T("SysInfoWnd")
#define WNDSTYLE		(WS_CAPTION | WS_POPUP | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_SYSMENU)
#define WNDSTYLEEX		(WS_EX_DLGMODALFRAME)
#define CPUFREQDELAY	(250)

using namespace Framework;
using namespace boost;
using namespace std;

const TCHAR* CSysInfoWnd::m_sFeature[32] =
{
	_T("FPU"),
	_T("VME"),
	_T("DE"),
	_T("PSE"),
	_T("TSC"),
	_T("MSR"),
	_T("PAE"),
	_T("MCE"),
	_T("CX8"),
	_T("APIC"),
	_T("*RESERVED*"),
	_T("SEP"),
	_T("MTRR"),
	_T("PGE"),
	_T("MCA"),
	_T("CMOV"),
	_T("PAT"),
	_T("PSE36"),
	_T("PSN"),
	_T("CLFL"),
	_T("*RESERVED*"),
	_T("DTES"),
	_T("ACPI"),
	_T("MMX"),
	_T("FXSR"),
	_T("SSE"),
	_T("SSE2"),
	_T("SS"),
	_T("HTT"),
	_T("TM"),
	_T("IA-64"),
	_T("PBE")
};

CSysInfoWnd::CSysInfoWnd(HWND hParent)
{
	RECT rc;

	if(!DoesWindowClassExist(CLSNAME))
	{
		WNDCLASSEX w;
		memset(&w, 0, sizeof(WNDCLASSEX));
		w.cbSize		= sizeof(WNDCLASSEX);
		w.lpfnWndProc	= CWindow::WndProc;
		w.lpszClassName	= CLSNAME;
		w.hbrBackground	= (HBRUSH)GetSysColorBrush(COLOR_BTNFACE);
		w.hInstance		= GetModuleHandle(NULL);
		w.hCursor		= LoadCursor(NULL, IDC_ARROW);
		RegisterClassEx(&w);
	}

	SetRect(&rc, 0, 0, 200, 270);

	if(hParent != NULL)
	{
		EnableWindow(hParent, FALSE);
	}

	Create(WNDSTYLEEX, CLSNAME, _T("System Information"), WNDSTYLE, &rc, hParent, NULL);
	SetClassPtr();

	SetRect(&rc, 0, 0, 1, 1);

	m_pProcessor	= new Win32::CStatic(m_hWnd, _T(""));
	m_pProcesses	= new Win32::CStatic(m_hWnd, _T(""));
	m_pThreads		= new Win32::CStatic(m_hWnd, _T(""));
	m_pFeatures		= new Win32::CListBox(m_hWnd, &rc, WS_VSCROLL | LBS_SORT);

	m_pLayout = new CVerticalLayout;
	m_pLayout->InsertObject(Win32::CLayoutWindow::CreateTextBoxBehavior(100, 20, m_pProcesses));
	m_pLayout->InsertObject(Win32::CLayoutWindow::CreateTextBoxBehavior(100, 20, m_pThreads));
	m_pLayout->InsertObject(Win32::CLayoutWindow::CreateTextBoxBehavior(100, 20, new Win32::CStatic(m_hWnd, _T("Processor:"))));
	m_pLayout->InsertObject(Win32::CLayoutWindow::CreateTextBoxBehavior(100, 20, m_pProcessor));
	m_pLayout->InsertObject(Win32::CLayoutWindow::CreateTextBoxBehavior(100, 20, new Win32::CStatic(m_hWnd, _T("Processor Features:"))));
	m_pLayout->InsertObject(new Win32::CLayoutWindow(200, 200, 1, 1, m_pFeatures));

	m_nRDTSCThread = NULL;

	UpdateSchedulerInfo();
	UpdateProcessorFeatures();
	UpdateProcessor();

	SetTimer(m_hWnd, 0, 2000, NULL);

	RefreshLayout();
}

CSysInfoWnd::~CSysInfoWnd()
{
	WaitForSingleObject(m_nRDTSCThread, INFINITE);
	DELETEPTR(m_pLayout);
}

long CSysInfoWnd::OnTimer()
{
	UpdateSchedulerInfo();
	//UpdateProcessor();
	return TRUE;
}

long CSysInfoWnd::OnSysCommand(unsigned int nCmd, LPARAM lParam)
{
	switch(nCmd)
	{
	case SC_CLOSE:
		if(GetParent() != NULL)
		{
			EnableWindow(GetParent(), TRUE);
			SetForegroundWindow(GetParent());
		}
		break;
	}
	return TRUE;
}

void CSysInfoWnd::UpdateSchedulerInfo()
{
	int nThreads, nProcesses;
	HANDLE hSnapshot;
	THREADENTRY32 ti;
	PROCESSENTRY32 pi;

	nThreads = 0;
	nProcesses = 0;

	hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS | TH32CS_SNAPTHREAD, 0);

	pi.dwSize = sizeof(PROCESSENTRY32);
	if(Process32First(hSnapshot, &pi))
	{
		nProcesses++;
		while(Process32Next(hSnapshot, &pi))
		{
			nProcesses++;
		}
	}

	ti.dwSize = sizeof(THREADENTRY32);
	if(Thread32First(hSnapshot, &ti))
	{
		nThreads++;
		while(Thread32Next(hSnapshot, &ti))
		{
			nThreads++;
		}
	}

	m_pProcesses->SetText((lexical_cast<tstring>(nProcesses) + _T(" processes running.")).c_str());
	m_pThreads->SetText((lexical_cast<tstring>(nThreads) + _T(" threads running.")).c_str());
}

void CSysInfoWnd::UpdateProcessorFeatures()
{
	uint32 nFeatures;
	int i;

#ifdef AMD64

	nFeatures = _cpuid_GetCpuFeatures();

#else

	__asm
	{
		mov eax, 0x00000001;
		cpuid;
		mov dword ptr[nFeatures], edx
	}

#endif

	m_pFeatures->ResetContent();

	for(i = 0; i < 32; i++)
	{
		//Reserved features
		if(i == 10) continue;
		if(i == 20) continue;

		if((nFeatures & (1 << i)) != 0)
		{
			m_pFeatures->AddString(m_sFeature[i]);
		}
	}
}

void CSysInfoWnd::UpdateProcessor()
{
	unsigned long nTID;
	if(WaitForSingleObject(m_nRDTSCThread, 0) == WAIT_TIMEOUT)
	{
		return;
	}
	m_nRDTSCThread = CreateThread(NULL, NULL, ThreadRDTSC, this, NULL, &nTID);
}

unsigned long WINAPI CSysInfoWnd::ThreadRDTSC(void* pParam)
{
	CSysInfoWnd* pWnd;
	HANDLE hThread;
	LARGE_INTEGER nTime;
	uint64 nStamp1, nStamp2, nDeltaStamp, nDeltaTime, nDone;
	char sCpu[13];
	TCHAR sTemp[256];

	hThread = GetCurrentThread();
	SetThreadPriority(hThread, THREAD_PRIORITY_TIME_CRITICAL);

	QueryPerformanceFrequency(&nTime);
	nDeltaTime = (nTime.QuadPart * CPUFREQDELAY) / 1000;

#ifdef AMD64

	nStamp1 = 0;

#else

	__asm
	{
		rdtsc;
		mov dword ptr[nStamp1 + 0x00], eax;
		mov dword ptr[nStamp1 + 0x04], edx;
	}

#endif

	QueryPerformanceCounter(&nTime);
	nDone = nTime.QuadPart + nDeltaTime;
	while(1)
	{
		QueryPerformanceCounter(&nTime);
		if((uint64)nTime.QuadPart >= nDone) break;
	}

#ifdef AMD64

	nStamp2 = 0;

#else

	__asm
	{
		rdtsc;
		mov dword ptr[nStamp2 + 0x00], eax;
		mov dword ptr[nStamp2 + 0x04], edx;
	}

#endif

	nDeltaStamp = 1000 * (nStamp2 - nStamp1) / CPUFREQDELAY;

	SetThreadPriority(hThread, THREAD_PRIORITY_NORMAL);

#ifdef AMD64

	_cpuid_GetCpuIdString(sCpu);

#else

	__asm
	{
		mov eax, 0x00000000;
		cpuid;
		mov dword ptr[sCpu + 0], ebx;
		mov dword ptr[sCpu + 4], edx;
		mov dword ptr[sCpu + 8], ecx;
	}

#endif

	sCpu[12] = '\0';

	pWnd = (CSysInfoWnd*)pParam;

	_sntprintf(sTemp, countof(sTemp), _T("%s @ ~%i MHz"), string_cast<tstring>(sCpu).c_str(), nDeltaStamp / 1000000);
	pWnd->m_pProcessor->SetText(sTemp);

	return 0xDEADBEEF;
}

void CSysInfoWnd::RefreshLayout()
{
	RECT rc;

	GetClientRect(&rc);

	SetRect(&rc, rc.left + 10, rc.top + 10, rc.right - 10, rc.bottom - 10);

	m_pLayout->SetRect(rc.left, rc.top, rc.right, rc.bottom);
	m_pLayout->RefreshGeometry();

	Redraw();
}
