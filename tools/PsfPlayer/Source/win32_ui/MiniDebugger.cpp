#include "MiniDebugger.h"
#include "win32/Rect.h"
#include "win32/AcceleratorTableGenerator.h"
#include "win32/DpiUtils.h"
#include "ui_win32/resource.h"
#include <functional>
#include <stdio.h>
#include <io.h>
#include <fcntl.h>

#define CLSNAME		_T("MiniDebugger")
#define WNDSTYLE	(WS_CAPTION | WS_POPUP | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_SYSMENU)
#define WNDSTYLEEX	(0)

CMiniDebugger::CMiniDebugger(CVirtualMachine& virtualMachine, const CDebuggable& debuggable) 
: m_virtualMachine(virtualMachine)
, m_debuggable(debuggable)
, m_functionsView(nullptr)
, m_mainSplitter(nullptr)
, m_subSplitter(nullptr)
, m_disAsmView(nullptr)
, m_registerView(nullptr)
, m_memoryView(nullptr)
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

	auto windowRect = Framework::Win32::PointsToPixels(Framework::Win32::CRect(0, 0, 1000, 600));
	Create(WNDSTYLEEX, CLSNAME, _T("MiniDebugger"), WNDSTYLE, windowRect, NULL, NULL);
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
	m_functionsView->OnFunctionDblClick.connect(boost::bind(&CMiniDebugger::OnFunctionDblClick, this, _1));
	m_functionsView->SetContext(&m_debuggable.GetCpu(), m_debuggable.biosDebugInfoProvider);
	m_functionsView->Refresh();

	m_subSplitter->SetChild(0, *m_disAsmView);
	m_subSplitter->SetChild(1, *m_registerView);
	m_subSplitter->SetEdgePosition(0.675);
	m_mainSplitter->SetChild(0, *m_subSplitter);
	m_mainSplitter->SetChild(1, *m_memoryView);
	m_disAsmView->SetAddress(m_debuggable.GetCpu().m_State.nPC);
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
	case ID_VM_STEP:
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
	Framework::Win32::CAcceleratorTableGenerator generator;
	generator.Insert(ID_VIEW_FUNCTIONS,			'F',	FCONTROL | FVIRTKEY);
	generator.Insert(ID_VM_STEP,				VK_F10,	FVIRTKEY);
	generator.Insert(ID_VM_RESUME,				VK_F5,	FVIRTKEY);
	m_acceleratorTable = generator.Create();
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
