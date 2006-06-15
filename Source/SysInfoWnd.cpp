#include <windows.h>
#include <tlhelp32.h>
#include "SysInfoWnd.h"
#include "PtrMacro.h"
#include "win32/LayoutWindow.h"

#define CLSNAME			_X("SysInfoWnd")
#define WNDSTYLE		(WS_CAPTION | WS_POPUP | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_SYSMENU)
#define WNDSTYLEEX		(WS_EX_DLGMODALFRAME)
#define CPUFREQDELAY	(250)

using namespace Framework;

const xchar* CSysInfoWnd::m_sFeature[32] =
{
	_X("FPU"),
	_X("VME"),
	_X("DE"),
	_X("PSE"),
	_X("TSC"),
	_X("MSR"),
	_X("PAE"),
	_X("MCE"),
	_X("CX8"),
	_X("APIC"),
	_X("*RESERVED*"),
	_X("SEP"),
	_X("MTRR"),
	_X("PGE"),
	_X("MCA"),
	_X("CMOV"),
	_X("PAT"),
	_X("PSE36"),
	_X("PSN"),
	_X("CLFL"),
	_X("*RESERVED*"),
	_X("DTES"),
	_X("ACPI"),
	_X("MMX"),
	_X("FXSR"),
	_X("SSE"),
	_X("SSE2"),
	_X("SS"),
	_X("HTT"),
	_X("TM"),
	_X("IA-64"),
	_X("PBE")
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

	Create(WNDSTYLEEX, CLSNAME, _X("System Information"), WNDSTYLE, &rc, hParent, NULL);
	SetClassPtr();

	SetRect(&rc, 0, 0, 1, 1);

	m_pProcessor	= new CStatic(m_hWnd, _X(""));
	m_pProcesses	= new CStatic(m_hWnd, _X(""));
	m_pThreads		= new CStatic(m_hWnd, _X(""));
	m_pFeatures		= new CListBox(m_hWnd, &rc, WS_VSCROLL | LBS_SORT);

	m_pLayout = new CVerticalLayout;
	m_pLayout->InsertObject(CLayoutWindow::CreateTextBoxBehavior(100, 20, m_pProcesses));
	m_pLayout->InsertObject(CLayoutWindow::CreateTextBoxBehavior(100, 20, m_pThreads));
	m_pLayout->InsertObject(CLayoutWindow::CreateTextBoxBehavior(100, 20, new CStatic(m_hWnd, _X("Processor:"))));
	m_pLayout->InsertObject(CLayoutWindow::CreateTextBoxBehavior(100, 20, m_pProcessor));
	m_pLayout->InsertObject(CLayoutWindow::CreateTextBoxBehavior(100, 20, new CStatic(m_hWnd, _X("Processor Features:"))));
	m_pLayout->InsertObject(new CLayoutWindow(200, 200, 1, 1, m_pFeatures));

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
	xchar sTemp[256];

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

	xsnprintf(sTemp, countof(sTemp), _X("%i processes running."), nProcesses);
	m_pProcesses->SetText(sTemp);

	xsnprintf(sTemp, countof(sTemp), _X("%i threads running."), nThreads);
	m_pThreads->SetText(sTemp);
}

void CSysInfoWnd::UpdateProcessorFeatures()
{
	uint32 nFeatures;
	int i;

	__asm
	{
		mov eax, 0x00000001;
		cpuid;
		mov dword ptr[nFeatures], edx
	}

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
	xchar sConvert[13];
	xchar sTemp[256];

	hThread = GetCurrentThread();
	SetThreadPriority(hThread, THREAD_PRIORITY_TIME_CRITICAL);

	QueryPerformanceFrequency(&nTime);
	nDeltaTime = (nTime.QuadPart * CPUFREQDELAY) / 1000;

	__asm
	{
		rdtsc;
		mov dword ptr[nStamp1 + 0x00], eax;
		mov dword ptr[nStamp1 + 0x04], edx;
	}

	QueryPerformanceCounter(&nTime);
	nDone = nTime.QuadPart + nDeltaTime;
	while(1)
	{
		QueryPerformanceCounter(&nTime);
		if((uint64)nTime.QuadPart >= nDone) break;
	}

	__asm
	{
		rdtsc;
		mov dword ptr[nStamp2 + 0x00], eax;
		mov dword ptr[nStamp2 + 0x04], edx;
	}

	nDeltaStamp = 1000 * (nStamp2 - nStamp1) / CPUFREQDELAY;

	SetThreadPriority(hThread, THREAD_PRIORITY_NORMAL);

	__asm
	{
		mov eax, 0x00000000;
		cpuid;
		mov dword ptr[sCpu + 0], ebx;
		mov dword ptr[sCpu + 4], edx;
		mov dword ptr[sCpu + 8], ecx;
	}
	sCpu[12] = '\0';
	xconvert(sConvert, sCpu, 13);

	pWnd = (CSysInfoWnd*)pParam;

	xsnprintf(sTemp, countof(sTemp), _X("%s @ ~%i MHz"), sConvert, nDeltaStamp / 1000000);
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
