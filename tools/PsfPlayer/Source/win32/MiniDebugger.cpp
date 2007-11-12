#include "MiniDebugger.h"
#include "win32/Rect.h"
#include "win32ui/resource.h"
#include <boost/bind.hpp>

#define CLSNAME		_T("MiniDebugger")
#define WNDSTYLE	(WS_CAPTION | WS_POPUP | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_SYSMENU)
#define WNDSTYLEEX	(0)

using namespace Framework;
using namespace boost;

CMiniDebugger::CMiniDebugger(CPsfVm& virtualMachine) :
m_virtualMachine(virtualMachine),
m_functionsView(NULL),
m_splitter(NULL),
m_disAsmView(NULL),
m_registerView(NULL)
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

    Create(WNDSTYLEEX, CLSNAME, _T("MiniDebugger"), WNDSTYLE, Win32::CRect(0, 0, 1000, 600), NULL, NULL);
	SetClassPtr();

    Center();
    CreateAccelerators();

    m_splitter = new Win32::CHorizontalSplitter(m_hWnd, GetClientRect());
    m_disAsmView = new CDisAsm(m_splitter->m_hWnd, Win32::CRect(0, 0, 1, 1), m_virtualMachine, &m_virtualMachine.GetCpu());
    m_registerView = new CRegViewGeneral(m_splitter->m_hWnd, Win32::CRect(0, 0, 1, 1), m_virtualMachine, &m_virtualMachine.GetCpu());
    m_registerView->Show(SW_SHOW);

    m_functionsView = new CFunctionsView(NULL, &m_virtualMachine.GetCpu());
    m_functionsView->m_OnFunctionDblClick.connect(bind(&CMiniDebugger::OnFunctionDblClick, this, _1));
    m_functionsView->Refresh();

    m_splitter->SetChild(0, *m_disAsmView);
    m_splitter->SetChild(1, *m_registerView);
    m_splitter->SetEdgePosition(0.675);
    m_disAsmView->SetAddress(m_virtualMachine.GetCpu().m_State.nPC);
}

CMiniDebugger::~CMiniDebugger()
{
    delete m_functionsView;
    delete m_disAsmView;
    delete m_registerView;
    delete m_splitter;
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
        break;
    case ID_VM_RESUME:
        m_virtualMachine.Resume();
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
	    MessageBeep(-1);
	    return;
    }

    m_disAsmView->SetFocus();
    m_virtualMachine.Step();
}

void CMiniDebugger::OnFunctionDblClick(uint32 address)
{
    m_disAsmView->SetAddress(address);
}

void CMiniDebugger::CreateAccelerators()
{
	ACCEL Accel[10];

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
	Accel[8].fVirt  = FCONTROL | FVIRTKEY;

    Accel[9].cmd    = ID_VIEW_TESTENGINECONSOLE;
    Accel[9].key    = 'T';
    Accel[9].fVirt  = FCONTROL | FVIRTKEY;

	m_acceleratorTable = CreateAcceleratorTable(Accel, sizeof(Accel) / sizeof(ACCEL));
}
