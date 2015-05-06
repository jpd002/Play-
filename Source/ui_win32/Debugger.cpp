#include "../iop/IopBios.h"
#include <stdio.h>
#include <io.h>
#include <fcntl.h>
#include <iostream>
#include "../AppConfig.h"
#include "../MIPSAssembler.h"
#include "../Ps2Const.h"
#include "../ee/PS2OS.h"
#include "../MipsFunctionPatternDb.h"
#include "StdStream.h"
#include "win32/AcceleratorTableGenerator.h"
#include "win32/InputBox.h"
#include "win32/DpiUtils.h"
#include "xml/Parser.h"
#include "Debugger.h"
#include "resource.h"
#include "string_cast.h"

#define CLSNAME			_T("CDebugger")

#define WM_EXECUNLOAD	(WM_USER + 0)
#define WM_EXECCHANGE	(WM_USER + 1)

#define PREF_DEBUGGER_MEMORYVIEW_BYTEWIDTH	"debugger.memoryview.bytewidth"

CDebugger::CDebugger(CPS2VM& virtualMachine)
: m_virtualMachine(virtualMachine)
{
	RegisterPreferences();

	if(!DoesWindowClassExist(CLSNAME))
	{
		WNDCLASSEX wc;
		memset(&wc, 0, sizeof(WNDCLASSEX));
		wc.cbSize			= sizeof(WNDCLASSEX);
		wc.hCursor			= LoadCursor(NULL, IDC_ARROW);
		wc.hbrBackground	= (HBRUSH)GetStockObject(GRAY_BRUSH); 
		wc.hInstance		= GetModuleHandle(NULL);
		wc.lpszClassName	= CLSNAME;
		wc.lpfnWndProc		= CWindow::WndProc;
		RegisterClassEx(&wc);
	}
	
	Create(NULL, CLSNAME, _T(""), WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN, Framework::Win32::CRect(0, 0, 640, 480), NULL, NULL);
	SetClassPtr();

	SetMenu(LoadMenu(GetModuleHandle(NULL), MAKEINTRESOURCE(IDR_DEBUGGER)));

	CreateClient(NULL);

	//Show(SW_MAXIMIZE);

	//ELF View Initialization
	m_pELFView = new CELFView(m_pMDIClient->m_hWnd);
	m_pELFView->Show(SW_HIDE);

	//Functions View Initialization
	m_pFunctionsView = new CFunctionsView(m_pMDIClient->m_hWnd);
	m_pFunctionsView->Show(SW_HIDE);
	m_pFunctionsView->OnFunctionDblClick.connect(boost::bind(&CDebugger::OnFunctionsViewFunctionDblClick, this, _1));
	m_pFunctionsView->OnFunctionsStateChange.connect(boost::bind(&CDebugger::OnFunctionsViewFunctionsStateChange, this));

	//Threads View Initialization
	m_threadsView = new CThreadsViewWnd(m_pMDIClient->m_hWnd, m_virtualMachine);
	m_threadsView->Show(SW_HIDE);
	m_threadsView->OnGotoAddress.connect(boost::bind(&CDebugger::OnThreadsViewAddressDblClick, this, _1));

	//Find Callers View Initialization
	m_findCallersView = new CFindCallersViewWnd(m_pMDIClient->m_hWnd);
	m_findCallersView->Show(SW_HIDE);
	m_findCallersView->AddressSelected.connect([&] (uint32 address) { OnFindCallersAddressDblClick(address); });

	//Debug Views Initialization
	m_nCurrentView = -1;

	memset(m_pView, 0, sizeof(m_pView));
	m_pView[DEBUGVIEW_EE]	= new CDebugView(m_pMDIClient->m_hWnd, m_virtualMachine, &m_virtualMachine.m_ee->m_EE, 
		std::bind(&CPS2VM::StepEe, &m_virtualMachine), m_virtualMachine.m_ee->m_os, "EmotionEngine");
	m_pView[DEBUGVIEW_VU0]	= new CDebugView(m_pMDIClient->m_hWnd, m_virtualMachine, &m_virtualMachine.m_ee->m_VU0, 
		std::bind(&CPS2VM::StepVu0, &m_virtualMachine), nullptr, "Vector Unit 0", CDisAsmWnd::DISASM_VU);
	m_pView[DEBUGVIEW_VU1]	= new CDebugView(m_pMDIClient->m_hWnd, m_virtualMachine, &m_virtualMachine.m_ee->m_VU1, 
		std::bind(&CPS2VM::StepVu1, &m_virtualMachine), nullptr, "Vector Unit 1", CDisAsmWnd::DISASM_VU);
	m_pView[DEBUGVIEW_IOP]  = new CDebugView(m_pMDIClient->m_hWnd, m_virtualMachine, &m_virtualMachine.m_iop->m_cpu, 
		std::bind(&CPS2VM::StepIop, &m_virtualMachine), m_virtualMachine.m_iopOs.get(), "IO Processor");

	m_virtualMachine.m_ee->m_os->OnExecutableChange.connect(boost::bind(&CDebugger::OnExecutableChange, this));
	m_virtualMachine.m_ee->m_os->OnExecutableUnloading.connect(boost::bind(&CDebugger::OnExecutableUnloading, this));

	ActivateView(DEBUGVIEW_EE);
	LoadSettings();

	if(GetDisassemblyWindow()->IsVisible())
	{
		GetDisassemblyWindow()->SetFocus();
	}

	CreateAccelerators();
}

CDebugger::~CDebugger()
{
	OnExecutableUnloadingMsg();

	DestroyAccelerators();

	SaveSettings();

	for(unsigned int i = 0; i < DEBUGVIEW_MAX; i++)
	{
		delete m_pView[i];
	}

	delete m_pELFView;
	delete m_pFunctionsView;
}

HACCEL CDebugger::GetAccelerators()
{
	return m_nAccTable;
}

void CDebugger::RegisterPreferences()
{
	CAppConfig& config(CAppConfig::GetInstance());

	config.RegisterPreferenceInteger("debugger.disasm.posx",				0);
	config.RegisterPreferenceInteger("debugger.disasm.posy",				0);
	config.RegisterPreferenceInteger("debugger.disasm.sizex",				0);
	config.RegisterPreferenceInteger("debugger.disasm.sizey",				0);
	config.RegisterPreferenceBoolean("debugger.disasm.visible",				true);

	config.RegisterPreferenceInteger("debugger.regview.posx",				0);
	config.RegisterPreferenceInteger("debugger.regview.posy",				0);
	config.RegisterPreferenceInteger("debugger.regview.sizex",				0);
	config.RegisterPreferenceInteger("debugger.regview.sizey",				0);
	config.RegisterPreferenceBoolean("debugger.regview.visible",			true);

	config.RegisterPreferenceInteger("debugger.memoryview.posx",			0);
	config.RegisterPreferenceInteger("debugger.memoryview.posy",			0);
	config.RegisterPreferenceInteger("debugger.memoryview.sizex",			0);
	config.RegisterPreferenceInteger("debugger.memoryview.sizey",			0);
	config.RegisterPreferenceBoolean("debugger.memoryview.visible",			true);
	config.RegisterPreferenceInteger(PREF_DEBUGGER_MEMORYVIEW_BYTEWIDTH,	0);

	config.RegisterPreferenceInteger("debugger.callstack.posx",				0);
	config.RegisterPreferenceInteger("debugger.callstack.posy",				0);
	config.RegisterPreferenceInteger("debugger.callstack.sizex",			0);
	config.RegisterPreferenceInteger("debugger.callstack.sizey",			0);
	config.RegisterPreferenceBoolean("debugger.callstack.visible",			true);
}

void CDebugger::UpdateTitle()
{
	std::tstring sTitle(_T("Play! - Debugger"));

	if(GetCurrentView() != NULL)
	{
		sTitle += 
			_T(" - [ ") + 
			string_cast<std::tstring>(GetCurrentView()->GetName()) +
			_T(" ]");
	}

	SetText(sTitle.c_str());
}

void CDebugger::LoadSettings()
{
	LoadViewLayout();
	LoadBytesPerLine();
}

void CDebugger::SaveSettings()
{
	SaveViewLayout();
	SaveBytesPerLine();
}

void CDebugger::SerializeWindowGeometry(CWindow* pWindow, const char* sPosX, const char* sPosY, const char* sSizeX, const char* sSizeY, const char* sVisible)
{
	CAppConfig& config(CAppConfig::GetInstance());

	RECT rc = pWindow->GetWindowRect();
	ScreenToClient(m_pMDIClient->m_hWnd, (POINT*)&rc + 0);
	ScreenToClient(m_pMDIClient->m_hWnd, (POINT*)&rc + 1);

	config.SetPreferenceInteger(sPosX, rc.left);
	config.SetPreferenceInteger(sPosY, rc.top);

	if(sSizeX != NULL && sSizeY != NULL)
	{
		config.SetPreferenceInteger(sSizeX, (rc.right - rc.left));
		config.SetPreferenceInteger(sSizeY, (rc.bottom - rc.top));
	}

	config.SetPreferenceBoolean(sVisible, pWindow->IsVisible());
}

void CDebugger::UnserializeWindowGeometry(CWindow* pWindow, const char* sPosX, const char* sPosY, const char* sSizeX, const char* sSizeY, const char* sVisible)
{
	CAppConfig& config(CAppConfig::GetInstance());

	pWindow->SetPosition(config.GetPreferenceInteger(sPosX), config.GetPreferenceInteger(sPosY));
	pWindow->SetSize(config.GetPreferenceInteger(sSizeX), config.GetPreferenceInteger(sSizeY));

	if(!config.GetPreferenceBoolean(sVisible))
	{
		pWindow->Show(SW_HIDE);
	}
	else
	{
		pWindow->Show(SW_SHOW);
	}
}

void CDebugger::Resume()
{
	m_virtualMachine.Resume();
}

void CDebugger::StepCPU()
{
	if(m_virtualMachine.GetStatus() == CVirtualMachine::RUNNING)
	{
		MessageBeep(-1);
		return;
	}
	
	if(::GetParent(GetFocus()) != GetDisassemblyWindow()->m_hWnd)
	{
		GetDisassemblyWindow()->SetFocus();
	}

	GetCurrentView()->Step();
}

void CDebugger::FindValue()
{
	Framework::Win32::CInputBox Input(_T("Find Value in Memory"), _T("Enter value to find:"), _T("00000000"));
	
	const TCHAR* sValue = Input.GetValue(m_hWnd);
	if(sValue == NULL) return;

	uint32 nValue = 0;
	_stscanf(sValue, _T("%x"), &nValue);
	if(nValue == 0) return;

	printf("Search results for 0x%0.8X\r\n", nValue);
	printf("-----------------------------\r\n");
	for(unsigned int i = 0; i < PS2::EE_RAM_SIZE; i += 4)
	{
		if(*(uint32*)&m_virtualMachine.m_ee->m_ram[i] == nValue)
		{
			printf("0x%0.8X\r\n", i);
		}
	}
}

void CDebugger::AssembleJAL()
{
	Framework::Win32::CInputBox InputTarget(_T("Assemble JAL"), _T("Enter jump target:"), _T("00000000"));
	Framework::Win32::CInputBox InputAssemble(_T("Assemble JAL"), _T("Enter address to assemble JAL to:"), _T("00000000"));

	const TCHAR* sTarget = InputTarget.GetValue(m_hWnd);
	if(sTarget == NULL) return;

	const TCHAR* sAssemble = InputAssemble.GetValue(m_hWnd);
	if(sAssemble == NULL) return;

	uint32 nValueTarget = 0, nValueAssemble = 0;
	_stscanf(sTarget, _T("%x"), &nValueTarget);
	_stscanf(sAssemble, _T("%x"), &nValueAssemble);

	*(uint32*)&m_virtualMachine.m_ee->m_ram[nValueAssemble] = 0x0C000000 | (nValueTarget / 4);
}

void CDebugger::ReanalyzeEe()
{
	if(m_virtualMachine.m_ee->m_os->GetELF() == nullptr) return;

	auto executableRange = m_virtualMachine.m_ee->m_os->GetExecutableRange();
	uint32 minAddr = executableRange.first;
	uint32 maxAddr = executableRange.second & ~0x03;

	m_virtualMachine.m_ee->m_EE.m_analysis->Clear();
	m_virtualMachine.m_ee->m_EE.m_analysis->Analyse(minAddr, maxAddr);
}

void CDebugger::FindEeFunctions()
{
	if(m_virtualMachine.m_ee->m_os->GetELF() == nullptr) return;

	auto executableRange = m_virtualMachine.m_ee->m_os->GetExecutableRange();
	uint32 minAddr = executableRange.first;
	uint32 maxAddr = executableRange.second & ~0x03;

	{
		Framework::CStdStream patternStream("ee_functions.xml", "rb");
		boost::scoped_ptr<Framework::Xml::CNode> document(Framework::Xml::CParser::ParseDocument(patternStream));
		CMipsFunctionPatternDb patternDb(document.get());

		for(auto patternIterator(std::begin(patternDb.GetPatterns()));
			patternIterator != std::end(patternDb.GetPatterns()); ++patternIterator)
		{
			auto pattern = *patternIterator;
			for(uint32 address = minAddr; address <= maxAddr; address += 4)
			{
				uint32* text = reinterpret_cast<uint32*>(m_virtualMachine.m_ee->m_ram + address);
				uint32 textSize = (maxAddr - address);
				if(pattern.Matches(text, textSize))
				{
					m_virtualMachine.m_ee->m_EE.m_Functions.InsertTag(address, pattern.name.c_str());
					break;
				}
			}
		}
	}
	
	{
		//Identify functions that reference special string literals (TODO: Move that inside a file)
		static const std::map<std::string, std::string> stringFuncs = 
		{
			{	"SceSifrpcBind",									"SifBindRpc"		},
			{	"SceSifrpcCall",									"SifCallRpc"		},
			{	"call cdread cmd\n",								"CdRead"			},
			{	"sceGsPutDrawEnv: DMA Ch.2 does not terminate\r\n",	"GsPutDrawEnv"		},
			{	"sceGsSyncPath: DMA Ch.1 does not terminate\r\n",	"GsSyncPath"		},
			{	"sceDbcReceiveData: rpc error\n",					"DbcReceiveData"	}
		};

		{
			auto& eeFunctions = m_virtualMachine.m_ee->m_EE.m_Functions;
			const auto& eeComments = m_virtualMachine.m_ee->m_EE.m_Comments;
			const auto& eeAnalysis = m_virtualMachine.m_ee->m_EE.m_analysis;
			for(auto tagIterator = eeComments.GetTagsBegin(); 
				tagIterator != eeComments.GetTagsEnd(); tagIterator++)
			{
				const auto& tag = *tagIterator;
				auto subroutine = eeAnalysis->FindSubroutine(tag.first);
				if(subroutine == nullptr) continue;
				auto stringFunc = stringFuncs.find(tag.second);
				if(stringFunc == std::end(stringFuncs)) continue;
				eeFunctions.InsertTag(subroutine->start, stringFunc->second.c_str());
			}
		}
	}

	m_virtualMachine.m_ee->m_EE.m_Functions.OnTagListChange();
}

void CDebugger::Layout1024()
{
	auto disassemblyWindowRect = Framework::Win32::PointsToPixels(Framework::Win32::MakeRectPositionSize(0, 0, 700, 435));
	auto registerViewWindowRect = Framework::Win32::PointsToPixels(Framework::Win32::MakeRectPositionSize(700, 0, 324, 572));
	auto memoryViewWindowRect = Framework::Win32::PointsToPixels(Framework::Win32::MakeRectPositionSize(0, 435, 700, 265));
	auto callStackWindowRect = Framework::Win32::PointsToPixels(Framework::Win32::MakeRectPositionSize(700, 572, 324, 128));

	GetDisassemblyWindow()->SetSizePosition(disassemblyWindowRect);
	GetDisassemblyWindow()->Show(SW_SHOW);

	GetRegisterViewWindow()->SetSizePosition(registerViewWindowRect);
	GetRegisterViewWindow()->Show(SW_SHOW);

	GetMemoryViewWindow()->SetSizePosition(memoryViewWindowRect);
	GetMemoryViewWindow()->Show(SW_SHOW);

	GetCallStackWindow()->SetSizePosition(callStackWindowRect);
	GetCallStackWindow()->Show(SW_SHOW);
}

void CDebugger::Layout1280()
{
	auto disassemblyWindowRect = Framework::Win32::PointsToPixels(Framework::Win32::MakeRectPositionSize(0, 0, 900, 540));
	auto registerViewWindowRect = Framework::Win32::PointsToPixels(Framework::Win32::MakeRectPositionSize(900, 0, 380, 784));
	auto memoryViewWindowRect = Framework::Win32::PointsToPixels(Framework::Win32::MakeRectPositionSize(0, 540, 900, 416));
	auto callStackWindowRect = Framework::Win32::PointsToPixels(Framework::Win32::MakeRectPositionSize(900, 784, 380, 172));

	GetDisassemblyWindow()->SetSizePosition(disassemblyWindowRect);
	GetDisassemblyWindow()->Show(SW_SHOW);

	GetRegisterViewWindow()->SetSizePosition(registerViewWindowRect);
	GetRegisterViewWindow()->Show(SW_SHOW);

	GetMemoryViewWindow()->SetSizePosition(memoryViewWindowRect);
	GetMemoryViewWindow()->Show(SW_SHOW);

	GetCallStackWindow()->SetSizePosition(callStackWindowRect);
	GetCallStackWindow()->Show(SW_SHOW);
}

void CDebugger::Layout1600()
{
	auto disassemblyWindowRect = Framework::Win32::PointsToPixels(Framework::Win32::MakeRectPositionSize(0, 0, 1094, 725));
	auto registerViewWindowRect = Framework::Win32::PointsToPixels(Framework::Win32::MakeRectPositionSize(1094, 0, 506, 725));
	auto memoryViewWindowRect = Framework::Win32::PointsToPixels(Framework::Win32::MakeRectPositionSize(0, 725, 1094, 407));
	auto callStackWindowRect = Framework::Win32::PointsToPixels(Framework::Win32::MakeRectPositionSize(1094, 725, 506, 407));

	GetDisassemblyWindow()->SetSizePosition(disassemblyWindowRect);
	GetDisassemblyWindow()->Show(SW_SHOW);

	GetRegisterViewWindow()->SetSizePosition(registerViewWindowRect);
	GetRegisterViewWindow()->Show(SW_SHOW);

	GetMemoryViewWindow()->SetSizePosition(memoryViewWindowRect);
	GetMemoryViewWindow()->Show(SW_SHOW);

	GetCallStackWindow()->SetSizePosition(callStackWindowRect);
	GetCallStackWindow()->Show(SW_SHOW);
}

void CDebugger::InitializeConsole()
{
#ifdef _DEBUG
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
#endif
}

void CDebugger::ActivateView(unsigned int nView)
{
	if(m_nCurrentView == nView) return;

	if(m_nCurrentView != -1)
	{
		SaveBytesPerLine();
		SaveViewLayout();
		GetCurrentView()->Hide();
	}

	m_findCallersRequestConnection.disconnect();

	m_nCurrentView = nView;
	LoadViewLayout();
	LoadBytesPerLine();
	UpdateTitle();

	{
		auto biosDebugInfoProvider = GetCurrentView()->GetBiosDebugInfoProvider();
		m_pFunctionsView->SetContext(GetCurrentView()->GetContext(), biosDebugInfoProvider);
		m_threadsView->SetContext(GetCurrentView()->GetContext(), biosDebugInfoProvider);
	}

	if(GetDisassemblyWindow()->IsVisible())
	{
		GetDisassemblyWindow()->SetFocus();
	}

	m_findCallersRequestConnection = GetCurrentView()->GetDisassemblyWindow()->GetDisAsm()->FindCallersRequested.connect(
		[&] (uint32 address) { OnFindCallersRequested(address); });
}

void CDebugger::SaveViewLayout()
{
	SerializeWindowGeometry(GetDisassemblyWindow(), \
		"debugger.disasm.posx", \
		"debugger.disasm.posy", \
		"debugger.disasm.sizex", \
		"debugger.disasm.sizey", \
		"debugger.disasm.visible");

	SerializeWindowGeometry(GetRegisterViewWindow(), \
		"debugger.regview.posx", \
		"debugger.regview.posy", \
		"debugger.regview.sizex", \
		"debugger.regview.sizey", \
		"debugger.regview.visible");

	SerializeWindowGeometry(GetMemoryViewWindow(), \
		"debugger.memoryview.posx", \
		"debugger.memoryview.posy", \
		"debugger.memoryview.sizex", \
		"debugger.memoryview.sizey", \
		"debugger.memoryview.visible");

	SerializeWindowGeometry(GetCallStackWindow(), \
		"debugger.callstack.posx", \
		"debugger.callstack.posy", \
		"debugger.callstack.sizex", \
		"debugger.callstack.sizey", \
		"debugger.callstack.visible");
}

void CDebugger::LoadViewLayout()
{
	UnserializeWindowGeometry(GetDisassemblyWindow(), \
		"debugger.disasm.posx", \
		"debugger.disasm.posy", \
		"debugger.disasm.sizex", \
		"debugger.disasm.sizey", \
		"debugger.disasm.visible");

	UnserializeWindowGeometry(GetRegisterViewWindow(), \
		"debugger.regview.posx", \
		"debugger.regview.posy", \
		"debugger.regview.sizex", \
		"debugger.regview.sizey", \
		"debugger.regview.visible");

	UnserializeWindowGeometry(GetMemoryViewWindow(), \
		"debugger.memoryview.posx", \
		"debugger.memoryview.posy", \
		"debugger.memoryview.sizex", \
		"debugger.memoryview.sizey", \
		"debugger.memoryview.visible");

	UnserializeWindowGeometry(GetCallStackWindow(), \
		"debugger.callstack.posx", \
		"debugger.callstack.posy", \
		"debugger.callstack.sizex", \
		"debugger.callstack.sizey", \
		"debugger.callstack.visible");
}

void CDebugger::SaveBytesPerLine()
{
	auto memoryView = GetMemoryViewWindow()->GetMemoryView();
	auto bytesPerLine = memoryView->GetBytesPerLine();
	CAppConfig::GetInstance().SetPreferenceInteger(PREF_DEBUGGER_MEMORYVIEW_BYTEWIDTH, bytesPerLine);
}

void CDebugger::LoadBytesPerLine()
{
	auto bytesPerLine = CAppConfig::GetInstance().GetPreferenceInteger(PREF_DEBUGGER_MEMORYVIEW_BYTEWIDTH);
	auto memoryView = GetMemoryViewWindow()->GetMemoryView();
	memoryView->SetBytesPerLine(bytesPerLine);
}

CDebugView* CDebugger::GetCurrentView()
{
	if(m_nCurrentView == -1) return NULL;
	return m_pView[m_nCurrentView];
}

CMIPS* CDebugger::GetContext()
{
	return GetCurrentView()->GetContext();
}

CDisAsmWnd* CDebugger::GetDisassemblyWindow()
{
	return GetCurrentView()->GetDisassemblyWindow();
}

CMemoryViewMIPSWnd* CDebugger::GetMemoryViewWindow()
{
	return GetCurrentView()->GetMemoryViewWindow();
}

CRegViewWnd* CDebugger::GetRegisterViewWindow()
{
	return GetCurrentView()->GetRegisterViewWindow();
}

CCallStackWnd* CDebugger::GetCallStackWindow()
{
	return GetCurrentView()->GetCallStackWindow();
}

void CDebugger::CreateAccelerators()
{
	Framework::Win32::CAcceleratorTableGenerator generator;
	generator.Insert(ID_VIEW_FUNCTIONS,			'F',	FCONTROL | FVIRTKEY);
	generator.Insert(ID_VIEW_THREADS,			'T',	FCONTROL | FVIRTKEY);
	generator.Insert(ID_VM_STEP,				VK_F10,	FVIRTKEY);
	generator.Insert(ID_VM_RESUME,				VK_F5,	FVIRTKEY);
	generator.Insert(ID_VIEW_CALLSTACK,			'A',	FCONTROL | FVIRTKEY);
	generator.Insert(ID_VIEW_EEVIEW,			'1',	FALT | FVIRTKEY);
	generator.Insert(ID_VIEW_VU0VIEW,			'2',	FALT | FVIRTKEY);
	generator.Insert(ID_VIEW_VU1VIEW,			'3',	FALT | FVIRTKEY);
	generator.Insert(ID_VIEW_IOPVIEW,			'4',	FALT | FVIRTKEY);
	m_nAccTable = generator.Create();
}

void CDebugger::DestroyAccelerators()
{
	DestroyAcceleratorTable(m_nAccTable);
}

long CDebugger::OnCommand(unsigned short nID, unsigned short nMsg, HWND hFrom)
{
	switch(nID)
	{
	case ID_VM_STEP:
		StepCPU();
		break;
	case ID_VM_RESUME:
		Resume();
		break;
	case ID_VM_DUMPINTCHANDLERS:
		m_virtualMachine.DumpEEIntcHandlers();
		break;
	case ID_VM_DUMPDMACHANDLERS:
		m_virtualMachine.DumpEEDmacHandlers();
		break;
	case ID_VM_ASMJAL:
		AssembleJAL();
		break;
	case ID_VM_REANALYZE_EE:
		ReanalyzeEe();
		break;
	case ID_VM_FINDEEFUNCTIONS:
		FindEeFunctions();
		break;
	case ID_VM_FINDVALUE:
		FindValue();
		break;
	case ID_VIEW_MEMORY:
		GetMemoryViewWindow()->Show(SW_SHOW);
		GetMemoryViewWindow()->SetFocus();
		return FALSE;
		break;
	case ID_VIEW_CALLSTACK:
		GetCallStackWindow()->Show(SW_SHOW);
		GetCallStackWindow()->SetFocus();
		return FALSE;
		break;
	case ID_VIEW_FUNCTIONS:
		m_pFunctionsView->Show(SW_SHOW);
		m_pFunctionsView->SetFocus();
		return FALSE;
		break;
	case ID_VIEW_ELF:
		m_pELFView->Show(SW_SHOW);
		m_pELFView->SetFocus();
		return FALSE;
		break;
	case ID_VIEW_THREADS:
		m_threadsView->Show(SW_SHOW);
		m_threadsView->SetFocus();
		return FALSE;
		break;
	case ID_VIEW_DISASSEMBLY:
		GetDisassemblyWindow()->Show(SW_SHOW);
		GetDisassemblyWindow()->SetFocus();
		return FALSE;
		break;
	case ID_VIEW_EEVIEW:
		ActivateView(DEBUGVIEW_EE);
		break;
	case ID_VIEW_VU0VIEW:
		ActivateView(DEBUGVIEW_VU0);
		break;
	case ID_VIEW_VU1VIEW:
		ActivateView(DEBUGVIEW_VU1);
		break;
	case ID_VIEW_IOPVIEW:
		ActivateView(DEBUGVIEW_IOP);
		break;
	case ID_WINDOW_CASCAD:
		m_pMDIClient->Cascade();
		return FALSE;
		break;
	case ID_WINDOW_TILEHORIZONTAL:
		m_pMDIClient->TileHorizontal();
		return FALSE;
		break;
	case ID_WINDOW_TILEVERTICAL:
		m_pMDIClient->TileVertical();
		return FALSE;
		break;
	case ID_WINDOW_LAYOUT1024:
		Layout1024();
		return FALSE;
		break;
	case ID_WINDOW_LAYOUT1280:
		Layout1280();
		return FALSE;
		break;
	case ID_WINDOW_LAYOUT1600:
		Layout1600();
		return FALSE;
		break;
	}
	return TRUE;
}

long CDebugger::OnSysCommand(unsigned int nCmd, LPARAM lParam)
{
	switch(nCmd)
	{
	case SC_CLOSE:
		Show(SW_HIDE);
		return FALSE;
	case SC_KEYMENU:
		return FALSE;
	}
	return TRUE;
}

long CDebugger::OnWndProc(unsigned int nMsg, WPARAM wParam, LPARAM lParam)
{
	switch(nMsg)
	{
	case WM_EXECUNLOAD:
		OnExecutableUnloadingMsg();
		return FALSE;
		break;
	case WM_EXECCHANGE:
		OnExecutableChangeMsg();
		return FALSE;
		break;
	}
	return CMDIFrame::OnWndProc(nMsg, wParam, lParam);
}

void CDebugger::OnFunctionsViewFunctionDblClick(uint32 address)
{
	GetDisassemblyWindow()->GetDisAsm()->SetAddress(address);
}

void CDebugger::OnFunctionsViewFunctionsStateChange()
{
	GetDisassemblyWindow()->Refresh();
}

void CDebugger::OnThreadsViewAddressDblClick(uint32 address)
{
	auto disAsm = GetDisassemblyWindow()->GetDisAsm();
	disAsm->SetCenterAtAddress(address);
	disAsm->SetSelectedAddress(address);
}

void CDebugger::OnExecutableChange()
{
	SendMessage(m_hWnd, WM_EXECCHANGE, 0, 0);
}

void CDebugger::OnExecutableUnloading()
{
	SendMessage(m_hWnd, WM_EXECUNLOAD, 0, 0);
}

void CDebugger::OnFindCallersRequested(uint32 address)
{
	auto context = GetCurrentView()->GetContext();

	m_findCallersView->FindCallers(context, address);
	m_findCallersView->Show(SW_SHOW);
	m_findCallersView->SetFocus();
}

void CDebugger::OnFindCallersAddressDblClick(uint32 address)
{
	auto disAsm = GetDisassemblyWindow()->GetDisAsm();
	disAsm->SetCenterAtAddress(address);
	disAsm->SetSelectedAddress(address);
}

void CDebugger::OnExecutableChangeMsg()
{
	m_pELFView->SetELF(m_virtualMachine.m_ee->m_os->GetELF());
//	m_pFunctionsView->SetELF(m_virtualMachine.m_os->GetELF());

	LoadDebugTags();

	GetDisassemblyWindow()->Refresh();
	m_pFunctionsView->Refresh();
}

void CDebugger::OnExecutableUnloadingMsg()
{
	SaveDebugTags();
	m_pELFView->SetELF(NULL);
//	m_pFunctionsView->SetELF(NULL);
}

void CDebugger::LoadDebugTags()
{
#ifdef DEBUGGER_INCLUDED
	m_virtualMachine.LoadDebugTags(m_virtualMachine.m_ee->m_os->GetExecutableName());
#endif
}

void CDebugger::SaveDebugTags()
{
#ifdef DEBUGGER_INCLUDED
	if(m_virtualMachine.m_ee->m_os->GetELF() != NULL)
	{
		m_virtualMachine.SaveDebugTags(m_virtualMachine.m_ee->m_os->GetExecutableName());
	}
#endif
}
