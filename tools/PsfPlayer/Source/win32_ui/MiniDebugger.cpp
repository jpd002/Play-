#include "MiniDebugger.h"
#include "win32/Rect.h"
#include "win32ui/resource.h"
#include <boost/bind.hpp>
#include <stdio.h>
#include <io.h>
#include <fcntl.h>

#define CLSNAME		_T("MiniDebugger")
#define WNDSTYLE	(WS_CAPTION | WS_POPUP | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_SYSMENU)
#define WNDSTYLEEX	(0)

#define ID_VM_PAUSE (0xBEEF)

CMiniDebugger::CMiniDebugger(CVirtualMachine& virtualMachine, const CDebuggable& debuggable) :
m_virtualMachine(virtualMachine),
m_debuggable(debuggable),
m_functionsView(NULL),
m_mainSplitter(NULL),
m_subSplitter(NULL),
m_disAsmView(NULL),
m_registerView(NULL),
m_memoryView(NULL)
{
	InitializeConsole();

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

	Create(WNDSTYLEEX, CLSNAME, _T("MiniDebugger"), WNDSTYLE, Framework::Win32::CRect(0, 0, 1000, 600), NULL, NULL);
	SetClassPtr();

	Center();
	CreateAccelerators();

	m_mainSplitter = new Framework::Win32::CVerticalSplitter(m_hWnd, GetClientRect());

	m_subSplitter = new Framework::Win32::CHorizontalSplitter(m_mainSplitter->m_hWnd, GetClientRect());
	m_memoryView = new CMemoryViewMIPS(m_mainSplitter->m_hWnd, Framework::Win32::CRect(0, 0, 1, 1), m_virtualMachine, &m_debuggable.GetCpu());

	m_disAsmView = new CDisAsm(m_subSplitter->m_hWnd, Framework::Win32::CRect(0, 0, 1, 1), m_virtualMachine, &m_debuggable.GetCpu());
	m_registerView = new CRegViewGeneral(m_subSplitter->m_hWnd, Framework::Win32::CRect(0, 0, 1, 1), m_virtualMachine, &m_debuggable.GetCpu());
	m_registerView->Show(SW_SHOW);

	m_functionsView = new CFunctionsView(NULL);
	m_functionsView->m_OnFunctionDblClick.connect(boost::bind(&CMiniDebugger::OnFunctionDblClick, this, _1));
	m_functionsView->SetContext(&m_debuggable.GetCpu(), m_debuggable.GetModules);
	m_functionsView->Refresh();

	m_subSplitter->SetChild(0, *m_disAsmView);
	m_subSplitter->SetChild(1, *m_registerView);
	m_subSplitter->SetEdgePosition(0.675);
	m_mainSplitter->SetChild(0, *m_subSplitter);
	m_mainSplitter->SetChild(1, *m_memoryView);
	m_disAsmView->SetAddress(m_debuggable.GetCpu().m_State.nPC);

//	CMIPS& context = m_virtualMachine.GetCpu();
//	for(unsigned int i = 0; i < CPsxVm::RAMSIZE / 4; i++)
//	{
//		uint32 nVal = context.m_pMemoryMap->GetWord(i * 4);
//		if((nVal & 0xFFFF) == 0x2910)
//		{
//			printf("Rawr: 0x%0.8X.\r\n", i * 4);
//		}
//	}
}

CMiniDebugger::~CMiniDebugger()
{
	m_virtualMachine.Pause();
	delete m_functionsView;
	delete m_disAsmView;
	delete m_registerView;
	delete m_mainSplitter;
	delete m_subSplitter;
}

void CMiniDebugger::Run()
{
	while(IsWindow())
	{
		MSG msg;
		GetMessage(&msg, 0, 0, 0);
		if(!TranslateAccelerator(m_hWnd, m_acceleratorTable, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
}

long CMiniDebugger::OnCommand(unsigned short command, unsigned short id, HWND hwndFrom)
{
	switch(command)
	{
	case ID_VM_STEP1:
		StepCPU();
		return TRUE;
		break;
	case ID_VM_RESUME:
		if(m_virtualMachine.GetStatus() == CVirtualMachine::PAUSED)
		{
			m_virtualMachine.Resume();
		}
		else
		{
			m_virtualMachine.Pause();
		}
		break;
//    case ID_VM_PAUSE:
//        m_virtualMachine.Pause();
//        break;
	case ID_VIEW_FUNCTIONS:
		if(m_functionsView != NULL)
		{
			m_functionsView->Show(SW_SHOW);
			SetForegroundWindow(*m_functionsView);
		}
		break;
	}

	return 0;
}

void CMiniDebugger::StepCPU()
{
	if(m_virtualMachine.GetStatus() == CVirtualMachine::RUNNING)
	{
//	    MessageBeep(-1);
		return;
	}

	m_disAsmView->SetFocus();
	m_debuggable.Step();
}

void CMiniDebugger::OnFunctionDblClick(uint32 address)
{
	m_disAsmView->SetAddress(address);
}

void CMiniDebugger::CreateAccelerators()
{
	ACCEL Accel[11];

	Accel[0].cmd	= ID_VM_SAVESTATE;
	Accel[0].key	= VK_F7;
	Accel[0].fVirt	= FVIRTKEY;

	Accel[1].cmd	= ID_VM_LOADSTATE;
	Accel[1].key	= VK_F8;
	Accel[1].fVirt	= FVIRTKEY;

	Accel[2].cmd	= ID_VIEW_FUNCTIONS;
	Accel[2].key	= 'F';
	Accel[2].fVirt	= FCONTROL | FVIRTKEY;

	Accel[3].cmd	= ID_VM_STEP1;
	Accel[3].key	= VK_F10;
	Accel[3].fVirt	= FVIRTKEY;

	Accel[4].cmd	= ID_VM_RESUME;
	Accel[4].key	= VK_F5;
	Accel[4].fVirt	= FVIRTKEY;

	Accel[5].cmd	= ID_VIEW_CALLSTACK;
	Accel[5].key	= 'A';
	Accel[5].fVirt	= FCONTROL | FVIRTKEY;

	Accel[6].cmd	= ID_VIEW_EEVIEW;
	Accel[6].key	= '1';
	Accel[6].fVirt	= FALT | FVIRTKEY;

	Accel[7].cmd	= ID_VIEW_VU1VIEW;
	Accel[7].key	= '3';
	Accel[7].fVirt	= FALT | FVIRTKEY;

	Accel[8].cmd	= 0xDEAD;
	Accel[8].key	= 'C';
	Accel[8].fVirt	= FCONTROL | FVIRTKEY;

	Accel[9].cmd	= ID_VIEW_TESTENGINECONSOLE;
	Accel[9].key	= 'T';
	Accel[9].fVirt	= FCONTROL | FVIRTKEY;

	Accel[10].cmd	= ID_VM_PAUSE;
	Accel[10].key	= VK_F6;
	Accel[10].fVirt	= FVIRTKEY;

	m_acceleratorTable = CreateAcceleratorTable(Accel, sizeof(Accel) / sizeof(ACCEL));
}

void CMiniDebugger::InitializeConsole()
{
	AllocConsole();

	CONSOLE_SCREEN_BUFFER_INFO ScreenBufferInfo;

	GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &ScreenBufferInfo);
	ScreenBufferInfo.dwSize.Y = 1000;
	SetConsoleScreenBufferSize(GetStdHandle(STD_OUTPUT_HANDLE), ScreenBufferInfo.dwSize);

	(*stdout) = *_fdopen(_open_osfhandle(
		reinterpret_cast<intptr_t>(GetStdHandle(STD_OUTPUT_HANDLE)),
		_O_TEXT), "w");

	setvbuf(stdout, NULL, _IONBF, 0);
	std::ios::sync_with_stdio();	
}
