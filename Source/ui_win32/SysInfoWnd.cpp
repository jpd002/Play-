#include <windows.h>
#include <tlhelp32.h>
#include <boost/lexical_cast.hpp>
#include "Types.h"
#include "string_cast.h"
#include "SysInfoWnd.h"
#include "PtrMacro.h"
#include "win32/LayoutWindow.h"

#ifdef _M_X64

extern "C" void _SysInfo_CPUID(uint32, void*);

#endif

#pragma intrinsic(__rdtsc)

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

CSysInfoWnd::CSysInfoWnd(HWND hParent) :
Win32::CModalWindow(hParent)
{
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

	if(hParent != NULL)
	{
		EnableWindow(hParent, FALSE);
	}

	int ydpi = GetDeviceCaps(GetDC(NULL), LOGPIXELSY);
	int width = MulDiv(200, ydpi, 96);
	int height = MulDiv(285, ydpi, 96);

	Create(WNDSTYLEEX, CLSNAME, _T("System Information"), WNDSTYLE, Framework::Win32::CRect(0, 0, width, height), hParent, NULL);
	SetClassPtr();

	m_pProcessor	= new Win32::CStatic(m_hWnd, _T(""));
	m_pProcesses	= new Win32::CStatic(m_hWnd, _T(""));
	m_pThreads		= new Win32::CStatic(m_hWnd, _T(""));
	m_pFeatures		= new Win32::CListBox(m_hWnd, Framework::Win32::CRect(0, 0, 1, 1), WS_VSCROLL | LBS_SORT);

	int lineHeight = MulDiv(20, ydpi, 96);

	m_pLayout = CVerticalLayout::Create();
	m_pLayout->InsertObject(Win32::CLayoutWindow::CreateTextBoxBehavior(100, lineHeight, m_pProcesses));
	m_pLayout->InsertObject(Win32::CLayoutWindow::CreateTextBoxBehavior(100, lineHeight, m_pThreads));
	m_pLayout->InsertObject(Win32::CLayoutWindow::CreateTextBoxBehavior(100, lineHeight, new Win32::CStatic(m_hWnd, _T("Processor:"))));
	m_pLayout->InsertObject(Win32::CLayoutWindow::CreateTextBoxBehavior(100, lineHeight, m_pProcessor));
	m_pLayout->InsertObject(Win32::CLayoutWindow::CreateTextBoxBehavior(100, lineHeight, new Win32::CStatic(m_hWnd, _T("Processor Features:"))));
	m_pLayout->InsertObject(Win32::CLayoutWindow::CreateCustomBehavior(lineHeight * 10, lineHeight*10, 1, 1, m_pFeatures));

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
}

long CSysInfoWnd::OnTimer(WPARAM)
{
	UpdateSchedulerInfo();
	//UpdateProcessor();
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
	uint32 nFeatures = 0;

#ifdef _M_X64

	uint32 cpuIdResult[3];
	_SysInfo_CPUID(1, cpuIdResult);
	nFeatures = cpuIdResult[1];

#else

	__asm
	{
		mov eax, 0x00000001;
		cpuid;
		mov dword ptr[nFeatures], edx
	}

#endif

	m_pFeatures->ResetContent();

	for(int i = 0; i < 32; i++)
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
	LARGE_INTEGER nTime;
	char sCpu[13];
	TCHAR sTemp[256];

	HANDLE hThread = GetCurrentThread();
	SetThreadPriority(hThread, THREAD_PRIORITY_TIME_CRITICAL);

	QueryPerformanceFrequency(&nTime);
	uint64 nDeltaTime = (nTime.QuadPart * CPUFREQDELAY) / 1000;

	uint64 nStamp1 = __rdtsc();

	QueryPerformanceCounter(&nTime);
	uint64 nDone = nTime.QuadPart + nDeltaTime;
	while(1)
	{
		QueryPerformanceCounter(&nTime);
		if((uint64)nTime.QuadPart >= nDone) break;
	}

	uint64 nStamp2 = __rdtsc();

	uint64 nDeltaStamp = 1000 * (nStamp2 - nStamp1) / CPUFREQDELAY;

	SetThreadPriority(hThread, THREAD_PRIORITY_NORMAL);

#ifdef _M_X64

	_SysInfo_CPUID(0, sCpu);

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

	CSysInfoWnd* pWnd = reinterpret_cast<CSysInfoWnd*>(pParam);

	_sntprintf(sTemp, countof(sTemp), _T("%s @ ~%i MHz"), string_cast<tstring>(sCpu).c_str(), nDeltaStamp / 1000000);
	pWnd->m_pProcessor->SetText(sTemp);

	return 0xDEADBEEF;
}

void CSysInfoWnd::RefreshLayout()
{
	RECT rc = GetClientRect();

	SetRect(&rc, rc.left + 10, rc.top + 10, rc.right - 10, rc.bottom - 10);

	m_pLayout->SetRect(rc.left, rc.top, rc.right, rc.bottom);
	m_pLayout->RefreshGeometry();

	Redraw();
}
