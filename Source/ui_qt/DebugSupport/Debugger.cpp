#include "iop/IopBios.h"
#include <stdio.h>
#include <fcntl.h>
#include <iostream>
#include "AppConfig.h"
#include "MIPSAssembler.h"
#include "Ps2Const.h"
#include "ee/PS2OS.h"
#include "MipsFunctionPatternDb.h"
#include "StdStream.h"
//#include "win32/AcceleratorTableGenerator.h"
//#include "win32/InputBox.h"
//#include "win32/DpiUtils.h"
#include "xml/Parser.h"
#include "xml/Utils.h"
#include "Debugger.h"
//#include "../resource.h"
#include "string_cast.h"
#include "string_format.h"
#include "DebugView.h"

enum
{
	WM_EXECUNLOAD,
	WM_EXECCHANGE,
	WM_MACHINESTATECHANGE,
	WM_RUNNINGSTATECHANGE
};

#define PREF_DEBUGGER_MEMORYVIEW_BYTEWIDTH "debugger.memoryview.bytewidth"

#define FIND_MAX_ADDRESS 0x02000000

CDebugger::CDebugger(QWidget* parent, CPS2VM& virtualMachine)
		: QDockWidget("Debugger", parent),
    m_virtualMachine(virtualMachine),
		m_debuggerMdi(nullptr)
{
	// Setup QT Stuff
	RegisterPreferences();

	// Load The Cursor to an arrow
	// Set Background to "Gray Brush"

	// Setup a grid to contain all of the sub items we will add
	
	//Create(NULL, CLSNAME, _T(""), WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN, Framework::Win32::CRect(0, 0, 640, 480), NULL, NULL);
	//SetClassPtr();

	//SetMenu(LoadMenu(GetModuleHandle(NULL), MAKEINTRESOURCE(IDR_DEBUGGER)));

	//CreateClient(NULL);
	this->m_debuggerMdi = new QMdiArea(this);
	this->setWidget(m_debuggerMdi);
	this->setFloating(true);
	this->showMaximized();

	//ELF View Initialization
	//m_pELFView = new CELFView(m_pMDIClient->m_hWnd);
	//m_pELFView->Show(SW_HIDE);

	//Functions View Initialization
	//m_pFunctionsView = new CFunctionsView(m_pMDIClient->m_hWnd);
	//m_pFunctionsView->Show(SW_HIDE);
	//m_OnFunctionDblClickConnection = m_pFunctionsView->OnFunctionDblClick.Connect(std::bind(&CDebugger::OnFunctionsViewFunctionDblClick, this, std::placeholders::_1));
	//m_OnFunctionsStateChangeConnection = m_pFunctionsView->OnFunctionsStateChange.Connect(std::bind(&CDebugger::OnFunctionsViewFunctionsStateChange, this));

	//Threads View Initialization
	//m_threadsView = new CThreadsViewWnd(m_pMDIClient->m_hWnd);
	//m_threadsView->Show(SW_HIDE);
	//m_OnGotoAddressConnection = m_threadsView->OnGotoAddress.Connect(std::bind(&CDebugger::OnThreadsViewAddressDblClick, this, std::placeholders::_1));

	//Address List View Initialization
	//m_addressListView = new CAddressListViewWnd(m_pMDIClient->m_hWnd);
	//m_addressListView->Show(SW_HIDE);
	//m_AddressSelectedConnection = m_addressListView->AddressSelected.Connect([&](uint32 address) { OnFindCallersAddressDblClick(address); });

	//Debug Views Initialization
	m_nCurrentView = DEBUGVIEW_EE;
	//m_nCurrentView = -1;

	m_pView[DEBUGVIEW_EE] = new CDebugView(this->m_debuggerMdi, m_virtualMachine, &m_virtualMachine.m_ee->m_EE, std::bind(&CPS2VM::StepEe, &m_virtualMachine), m_virtualMachine.m_ee->m_os, "EmotionEngine");
	//this->m_debuggerMdi->addSubWindow(m_pView[DEBUGVIEW_EE]);
	//m_pView[DEBUGVIEW_VU0] = new CDebugView(this->m_debuggerMdi, m_virtualMachine, &m_virtualMachine.m_ee->m_VU0,
	//                                        std::bind(&CPS2VM::StepVu0, &m_virtualMachine), nullptr, "Vector Unit 0");//, CDisAsmWnd::DISASM_VU);
	//m_pView[DEBUGVIEW_VU1] = new CDebugView(this->m_debuggerMdi, m_virtualMachine, &m_virtualMachine.m_ee->m_VU1,
	//                                        std::bind(&CPS2VM::StepVu1, &m_virtualMachine), nullptr, "Vector Unit 1");//, CDisAsmWnd::DISASM_VU);
	//m_pView[DEBUGVIEW_IOP] = new CDebugView(this->m_debuggerMdi, m_virtualMachine, &m_virtualMachine.m_iop->m_cpu,
	//                                        std::bind(&CPS2VM::StepIop, &m_virtualMachine), m_virtualMachine.m_iop->m_bios.get(), "IO Processor");

	m_OnExecutableChangeConnection = m_virtualMachine.m_ee->m_os->OnExecutableChange.Connect(std::bind(&CDebugger::OnExecutableChange, this));
	m_OnExecutableUnloadingConnection = m_virtualMachine.m_ee->m_os->OnExecutableUnloading.Connect(std::bind(&CDebugger::OnExecutableUnloading, this));

	m_OnMachineStateChangeConnection = m_virtualMachine.OnMachineStateChange.Connect(std::bind(&CDebugger::OnMachineStateChange, this));
	m_OnRunningStateChangeConnection = m_virtualMachine.OnRunningStateChange.Connect(std::bind(&CDebugger::OnRunningStateChange, this));

//	ActivateView(DEBUGVIEW_EE);
//	LoadSettings();

	//if(GetDisassemblyWindow()->IsVisible())
	//{
	//	GetDisassemblyWindow()->SetFocus();
	//}

	CreateAccelerators();
	this->m_debuggerMdi->show();
}

CDebugger::~CDebugger()
{
	OnExecutableUnloadingMsg();

	DestroyAccelerators();

	SaveSettings();

	//Destroy explicitly since we're keeping the window alive
	//artificially by handling WM_SYSCOMMAND
	//Destroy();

	for(unsigned int i = 0; i < DEBUGVIEW_MAX; i++)
	{
		//delete m_pView[i];
	}
	delete m_pView[DEBUGVIEW_EE];

	//delete m_pELFView;
	//delete m_pFunctionsView;
}

//HACCEL CDebugger::GetAccelerators()
//{
//	return m_nAccTable;
//}

void CDebugger::RegisterPreferences()
{
	CAppConfig& config(CAppConfig::GetInstance());

	config.RegisterPreferenceInteger("debugger.disasm.posx", 0);
	config.RegisterPreferenceInteger("debugger.disasm.posy", 0);
	config.RegisterPreferenceInteger("debugger.disasm.sizex", 0);
	config.RegisterPreferenceInteger("debugger.disasm.sizey", 0);
	config.RegisterPreferenceBoolean("debugger.disasm.visible", true);

	config.RegisterPreferenceInteger("debugger.regview.posx", 0);
	config.RegisterPreferenceInteger("debugger.regview.posy", 0);
	config.RegisterPreferenceInteger("debugger.regview.sizex", 0);
	config.RegisterPreferenceInteger("debugger.regview.sizey", 0);
	config.RegisterPreferenceBoolean("debugger.regview.visible", true);

	config.RegisterPreferenceInteger("debugger.memoryview.posx", 0);
	config.RegisterPreferenceInteger("debugger.memoryview.posy", 0);
	config.RegisterPreferenceInteger("debugger.memoryview.sizex", 0);
	config.RegisterPreferenceInteger("debugger.memoryview.sizey", 0);
	config.RegisterPreferenceBoolean("debugger.memoryview.visible", true);
	config.RegisterPreferenceInteger(PREF_DEBUGGER_MEMORYVIEW_BYTEWIDTH, 0);

	config.RegisterPreferenceInteger("debugger.callstack.posx", 0);
	config.RegisterPreferenceInteger("debugger.callstack.posy", 0);
	config.RegisterPreferenceInteger("debugger.callstack.sizex", 0);
	config.RegisterPreferenceInteger("debugger.callstack.sizey", 0);
	config.RegisterPreferenceBoolean("debugger.callstack.visible", true);
}

void CDebugger::UpdateTitle()
{
	//std::string sTitle(_T("Play! - Debugger"));

	//if(GetCurrentView() != NULL)
	//{
	//	sTitle +=
	//	    _T(" - [ ") +
	//	    string_cast<std::tstring>(GetCurrentView()->GetName()) +
	//	    _T(" ]");
	//}

	//SetText(sTitle.c_str());
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

void CDebugger::SerializeWindowGeometry(const char* sPosX, const char* sPosY, const char* sSizeX, const char* sSizeY, const char* sVisible)
{
	//CAppConfig& config(CAppConfig::GetInstance());

	//RECT rc = pWindow->GetWindowRect();
	//ScreenToClient(m_pMDIClient->m_hWnd, (POINT*)&rc + 0);
	//ScreenToClient(m_pMDIClient->m_hWnd, (POINT*)&rc + 1);

	//config.SetPreferenceInteger(sPosX, rc.left);
	//config.SetPreferenceInteger(sPosY, rc.top);

	//if(sSizeX != NULL && sSizeY != NULL)
	//{
	//	config.SetPreferenceInteger(sSizeX, (rc.right - rc.left));
	//	config.SetPreferenceInteger(sSizeY, (rc.bottom - rc.top));
	//}

	//config.SetPreferenceBoolean(sVisible, this->isVisible());
}

void CDebugger::UnserializeWindowGeometry(const char* sPosX, const char* sPosY, const char* sSizeX, const char* sSizeY, const char* sVisible)
{
	//CAppConfig& config(CAppConfig::GetInstance());

	//pWindow->SetPosition(config.GetPreferenceInteger(sPosX), config.GetPreferenceInteger(sPosY));
	//pWindow->SetSize(config.GetPreferenceInteger(sSizeX), config.GetPreferenceInteger(sSizeY));

	//if(!config.GetPreferenceBoolean(sVisible))
	//{
	//	pWindow->Show(SW_HIDE);
	//}
	//else
	//{
	//}
}

void CDebugger::Resume()
{
	m_virtualMachine.Resume();
}

void CDebugger::StepCPU()
{
	if(m_virtualMachine.GetStatus() == CVirtualMachine::RUNNING)
	{
		//MessageBeep(-1);
		return;
	}

	//if(::GetParent(GetFocus()) != GetDisassemblyWindow()->m_hWnd)
	//{
	//	GetDisassemblyWindow()->SetFocus();
	//}

	
	GetCurrentView()->Step();
}

void CDebugger::FindWordValue(uint32 mask)
{
	//Framework::Win32::CInputBox input(_T("Find Value in Memory"), _T("Enter value to find:"), _T("00000000"));

	//auto valueString = input.GetValue(m_hWnd);
	//if(!valueString) return;

	uint32 targetValue = 0;
	//auto scanned = _stscanf(valueString, _T("%x"), &targetValue);
	//if(scanned == 0) return;

	//auto context = GetCurrentView()->GetContext();
	//auto title = string_format(_T("Search results for 0x%08X"), targetValue);
	//auto refs = FindWordValueRefs(context, targetValue & mask, mask);

	//m_addressListView->SetTitle(std::move(title));
	//m_addressListView->SetAddressList(std::move(refs));
	//m_addressListView->Show(SW_SHOW);
	//m_addressListView->SetFocus();
}

void CDebugger::AssembleJAL()
{
	//Framework::Win32::CInputBox InputTarget(_T("Assemble JAL"), _T("Enter jump target:"), _T("00000000"));
	//Framework::Win32::CInputBox InputAssemble(_T("Assemble JAL"), _T("Enter address to assemble JAL to:"), _T("00000000"));

	//const TCHAR* sTarget = InputTarget.GetValue(m_hWnd);
	//if(sTarget == NULL) return;

	//const TCHAR* sAssemble = InputAssemble.GetValue(m_hWnd);
	//if(sAssemble == NULL) return;

	uint32 nValueTarget = 0, nValueAssemble = 0;
	//_stscanf(sTarget, _T("%x"), &nValueTarget);
	//_stscanf(sAssemble, _T("%x"), &nValueAssemble);

	//*(uint32*)&m_virtualMachine.m_ee->m_ram[nValueAssemble] = 0x0C000000 | (nValueTarget / 4);
}

void CDebugger::ReanalyzeEe()
{
	if(m_virtualMachine.m_ee->m_os->GetELF() == nullptr) return;

	auto executableRange = m_virtualMachine.m_ee->m_os->GetExecutableRange();
	uint32 minAddr = executableRange.first;
	uint32 maxAddr = executableRange.second & ~0x03;

	//auto getAddress =
	//    [this](const TCHAR* prompt, uint32& address) {
	//	    Framework::Win32::CInputBox addressInputBox(_T("Analyze EE"), prompt,
	//	                                                string_format(_T("0x%08X"), address).c_str());
	//	    auto addrValue = addressInputBox.GetValue(m_hWnd);
	//	    if(addrValue == nullptr) return false;
	//	    uint32 addrValueTemp = 0;
	//	    int cvtCount = _stscanf(addrValue, _T("%x"), &addrValueTemp);
	//	    if(cvtCount != 0)
	//	    {
	//		    address = addrValueTemp & ~0x3;
	//	    }
	//	    return true;
	 //   };

	//if(!getAddress(_T("Start Address:"), minAddr)) return;
	//if(!getAddress(_T("End Address:"), maxAddr)) return;

	//if(minAddr > maxAddr)
	//{
	//	MessageBox(m_hWnd, _T("Start address is larger than end address."), _T("Analyze EE"), MB_ICONERROR);
	//	return;
	//}

	minAddr = std::min<uint32>(minAddr, PS2::EE_RAM_SIZE);
	maxAddr = std::min<uint32>(maxAddr, PS2::EE_RAM_SIZE);

	m_virtualMachine.m_ee->m_EE.m_analysis->Clear();
	m_virtualMachine.m_ee->m_EE.m_analysis->Analyse(minAddr, maxAddr);
}

void CDebugger::FindEeFunctions()
{
	if(m_virtualMachine.m_ee->m_os->GetELF() == nullptr) return;

	auto executableRange = m_virtualMachine.m_ee->m_os->GetExecutableRange();
	uint32 minAddr = executableRange.first;
	uint32 maxAddr = executableRange.second & ~0x03;

	Framework::CStdStream functionsStream("ee_functions.xml", "rb");
	auto functionsDocument = std::unique_ptr<Framework::Xml::CNode>(Framework::Xml::CParser::ParseDocument(functionsStream));
	auto functionsNode = functionsDocument->Select("Functions");

	//Check function patterns
	{
		CMipsFunctionPatternDb patternDb(functionsNode);

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

	//Check function comments
	{
		std::map<std::string, std::string> stringFuncs;
		auto commentNodes = functionsNode->SelectNodes("FunctionComments/FunctionComment");
		for(const auto& commentNode : commentNodes)
		{
			auto comment = Framework::Xml::GetAttributeStringValue(commentNode, "Comment");
			auto functionName = Framework::Xml::GetAttributeStringValue(commentNode, "Function");
			stringFuncs.insert(std::make_pair(comment, functionName));
		}

		//Identify functions that reference special string literals
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
	//auto disassemblyWindowRect = Framework::Win32::PointsToPixels(Framework::Win32::MakeRectPositionSize(0, 0, 700, 435));
	//auto registerViewWindowRect = Framework::Win32::PointsToPixels(Framework::Win32::MakeRectPositionSize(700, 0, 324, 572));
	//auto memoryViewWindowRect = Framework::Win32::PointsToPixels(Framework::Win32::MakeRectPositionSize(0, 435, 700, 265));
	//auto callStackWindowRect = Framework::Win32::PointsToPixels(Framework::Win32::MakeRectPositionSize(700, 572, 324, 128));

	//GetDisassemblyWindow()->SetSizePosition(disassemblyWindowRect);
	//GetDisassemblyWindow()->Show(SW_SHOW);

	//GetRegisterViewWindow()->SetSizePosition(registerViewWindowRect);
	//GetRegisterViewWindow()->Show(SW_SHOW);

	//GetMemoryViewWindow()->SetSizePosition(memoryViewWindowRect);
	//GetMemoryViewWindow()->Show(SW_SHOW);

	//GetCallStackWindow()->SetSizePosition(callStackWindowRect);
	//GetCallStackWindow()->Show(SW_SHOW);
}

void CDebugger::Layout1280()
{
	//auto disassemblyWindowRect = Framework::Win32::PointsToPixels(Framework::Win32::MakeRectPositionSize(0, 0, 900, 540));
	//auto registerViewWindowRect = Framework::Win32::PointsToPixels(Framework::Win32::MakeRectPositionSize(900, 0, 380, 784));
	//auto memoryViewWindowRect = Framework::Win32::PointsToPixels(Framework::Win32::MakeRectPositionSize(0, 540, 900, 416));
	//auto callStackWindowRect = Framework::Win32::PointsToPixels(Framework::Win32::MakeRectPositionSize(900, 784, 380, 172));

	//GetDisassemblyWindow()->SetSizePosition(disassemblyWindowRect);
	//GetDisassemblyWindow()->Show(SW_SHOW);

	//GetRegisterViewWindow()->SetSizePosition(registerViewWindowRect);
	//GetRegisterViewWindow()->Show(SW_SHOW);

	//GetMemoryViewWindow()->SetSizePosition(memoryViewWindowRect);
	//GetMemoryViewWindow()->Show(SW_SHOW);

	//GetCallStackWindow()->SetSizePosition(callStackWindowRect);
	//GetCallStackWindow()->Show(SW_SHOW);
}

void CDebugger::Layout1600()
{
	//auto disassemblyWindowRect = Framework::Win32::PointsToPixels(Framework::Win32::MakeRectPositionSize(0, 0, 1094, 725));
	//auto registerViewWindowRect = Framework::Win32::PointsToPixels(Framework::Win32::MakeRectPositionSize(1094, 0, 506, 725));
	//auto memoryViewWindowRect = Framework::Win32::PointsToPixels(Framework::Win32::MakeRectPositionSize(0, 725, 1094, 407));
	//auto callStackWindowRect = Framework::Win32::PointsToPixels(Framework::Win32::MakeRectPositionSize(1094, 725, 506, 407));

	//GetDisassemblyWindow()->SetSizePosition(disassemblyWindowRect);
	//GetDisassemblyWindow()->Show(SW_SHOW);

	//GetRegisterViewWindow()->SetSizePosition(registerViewWindowRect);
	//GetRegisterViewWindow()->Show(SW_SHOW);

	//GetMemoryViewWindow()->SetSizePosition(memoryViewWindowRect);
	//GetMemoryViewWindow()->Show(SW_SHOW);

	//GetCallStackWindow()->SetSizePosition(callStackWindowRect);
	//GetCallStackWindow()->Show(SW_SHOW);
}

void CDebugger::InitializeConsole()
{
#ifdef _DEBUG
	//AllocConsole();

	//CONSOLE_SCREEN_BUFFER_INFO ScreenBufferInfo;

	//GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &ScreenBufferInfo);
	//ScreenBufferInfo.dwSize.Y = 1000;
	//SetConsoleScreenBufferSize(GetStdHandle(STD_OUTPUT_HANDLE), ScreenBufferInfo.dwSize);

	//(*stdout) = *_fdopen(_open_osfhandle(
	//                         reinterpret_cast<intptr_t>(GetStdHandle(STD_OUTPUT_HANDLE)),
	//                         _O_TEXT),
	//                     "w");

	//setvbuf(stdout, NULL, _IONBF, 0);
	//std::ios::sync_with_stdio();
#endif
}

void CDebugger::ActivateView(unsigned int nView)
{
	if(m_nCurrentView == nView) return;

	if(m_nCurrentView != -1)
	{
		SaveBytesPerLine();
		SaveViewLayout();
		this->hide();
	}

	//m_findCallersRequestConnection.reset();

	m_nCurrentView = nView;
	LoadViewLayout();
	LoadBytesPerLine();
	UpdateTitle();

	{
		auto biosDebugInfoProvider = GetCurrentView()->GetBiosDebugInfoProvider();
		//m_pFunctionsView->SetContext(GetCurrentView()->GetContext(), biosDebugInfoProvider);
		//m_threadsView->SetContext(GetCurrentView()->GetContext(), biosDebugInfoProvider);
	}

	//if(GetDisassemblyWindow()->IsVisible())
	//{
	//	GetDisassemblyWindow()->SetFocus();
	//}

	//m_findCallersRequestConnection = GetCurrentView()->GetDisassemblyWindow()->GetDisAsm()->FindCallersRequested.Connect(
	//    [&](uint32 address) { OnFindCallersRequested(address); });
}

void CDebugger::SaveViewLayout()
{
	/*SerializeWindowGeometry(GetDisassemblyWindow(),
	                        "debugger.disasm.posx",
	                        "debugger.disasm.posy",
	                        "debugger.disasm.sizex",
	                        "debugger.disasm.sizey",
	                        "debugger.disasm.visible");

	SerializeWindowGeometry(GetRegisterViewWindow(),
	                        "debugger.regview.posx",
	                        "debugger.regview.posy",
	                        "debugger.regview.sizex",
	                        "debugger.regview.sizey",
	                        "debugger.regview.visible");

	SerializeWindowGeometry(GetMemoryViewWindow(),
	                        "debugger.memoryview.posx",
	                        "debugger.memoryview.posy",
	                        "debugger.memoryview.sizex",
	                        "debugger.memoryview.sizey",
	                        "debugger.memoryview.visible");

	SerializeWindowGeometry(GetCallStackWindow(),
	                        "debugger.callstack.posx",
	                        "debugger.callstack.posy",
	                        "debugger.callstack.sizex",
	                        "debugger.callstack.sizey",
	                        "debugger.callstack.visible");
													*/
}

void CDebugger::LoadViewLayout()
{
	/*
	UnserializeWindowGeometry(GetDisassemblyWindow(),
	                          "debugger.disasm.posx",
	                          "debugger.disasm.posy",
	                          "debugger.disasm.sizex",
	                          "debugger.disasm.sizey",
	                          "debugger.disasm.visible");

	UnserializeWindowGeometry(GetRegisterViewWindow(),
	                          "debugger.regview.posx",
	                          "debugger.regview.posy",
	                          "debugger.regview.sizex",
	                          "debugger.regview.sizey",
	                          "debugger.regview.visible");

	UnserializeWindowGeometry(GetMemoryViewWindow(),
	                          "debugger.memoryview.posx",
	                          "debugger.memoryview.posy",
	                          "debugger.memoryview.sizex",
	                          "debugger.memoryview.sizey",
	                          "debugger.memoryview.visible");

	UnserializeWindowGeometry(GetCallStackWindow(),
	                          "debugger.callstack.posx",
	                          "debugger.callstack.posy",
	                          "debugger.callstack.sizex",
	                          "debugger.callstack.sizey",
	                          "debugger.callstack.visible");
														*/
}

void CDebugger::SaveBytesPerLine()
{
	//auto memoryView = GetMemoryViewWindow()->GetMemoryView();
	//auto bytesPerLine = memoryView->GetBytesPerLine();
	//CAppConfig::GetInstance().SetPreferenceInteger(PREF_DEBUGGER_MEMORYVIEW_BYTEWIDTH, bytesPerLine);
}

void CDebugger::LoadBytesPerLine()
{
	//auto bytesPerLine = CAppConfig::GetInstance().GetPreferenceInteger(PREF_DEBUGGER_MEMORYVIEW_BYTEWIDTH);
	//auto memoryView = GetMemoryViewWindow()->GetMemoryView();
	//memoryView->SetBytesPerLine(bytesPerLine);
}

CDebugView* CDebugger::GetCurrentView()
{
	if(m_nCurrentView == -1) return NULL;
	return m_pView[m_nCurrentView];
}

CMIPS* CDebugger::GetContext()
{
	return nullptr;
	return GetCurrentView()->GetContext();
}
/*
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
*/
std::vector<uint32> CDebugger::FindCallers(CMIPS* context, uint32 address)
{
	std::vector<uint32> callers;
	for(uint32 i = 0; i < FIND_MAX_ADDRESS; i += 4)
	{
		uint32 opcode = context->m_pMemoryMap->GetInstruction(i);
		uint32 ea = context->m_pArch->GetInstructionEffectiveAddress(context, i, opcode);
		if(ea == address)
		{
			callers.push_back(i);
		}
	}
	return callers;
}

std::vector<uint32> CDebugger::FindWordValueRefs(CMIPS* context, uint32 targetValue, uint32 valueMask)
{
	std::vector<uint32> refs;
	for(uint32 i = 0; i < FIND_MAX_ADDRESS; i += 4)
	{
		uint32 valueAtAddress = context->m_pMemoryMap->GetWord(i);
		if((valueAtAddress & valueMask) == targetValue)
		{
			refs.push_back(i);
		}
	}
	return refs;
}

void CDebugger::CreateAccelerators()
{
	//Framework::Win32::CAcceleratorTableGenerator generator;
	//generator.Insert(ID_VIEW_FUNCTIONS, 'F', FCONTROL | FVIRTKEY);
	//generator.Insert(ID_VIEW_THREADS, 'T', FCONTROL | FVIRTKEY);
	//generator.Insert(ID_VM_STEP, VK_F10, FVIRTKEY);
	//generator.Insert(ID_VM_RESUME, VK_F5, FVIRTKEY);
	//generator.Insert(ID_VIEW_CALLSTACK, 'A', FCONTROL | FVIRTKEY);
	//generator.Insert(ID_VIEW_EEVIEW, '1', FALT | FVIRTKEY);
	//generator.Insert(ID_VIEW_VU0VIEW, '2', FALT | FVIRTKEY);
	//generator.Insert(ID_VIEW_VU1VIEW, '3', FALT | FVIRTKEY);
	//generator.Insert(ID_VIEW_IOPVIEW, '4', FALT | FVIRTKEY);
	//m_nAccTable = generator.Create();
}

void CDebugger::DestroyAccelerators()
{
	//DestroyAcceleratorTable(m_nAccTable);
}

/*long CDebugger::OnCommand(unsigned short nID, unsigned short nMsg, HWND hFrom)
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
	case ID_VM_FINDWORDVALUE:
		FindWordValue(~0);
		break;
	case ID_VM_FINDWORDLOWHALFVALUE:
		FindWordValue(0xFFFF);
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
}*/

/*
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
}*/

/*
LRESULT CDebugger::OnWndProc(unsigned int nMsg, WPARAM wParam, LPARAM lParam)
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
	case WM_MACHINESTATECHANGE:
		OnMachineStateChangeMsg();
		return FALSE;
		break;
	case WM_RUNNINGSTATECHANGE:
		OnRunningStateChangeMsg();
		return FALSE;
		break;
	}
	return CMDIFrame::OnWndProc(nMsg, wParam, lParam);
}*/

void CDebugger::OnFunctionsViewFunctionDblClick(uint32 address)
{
	//GetDisassemblyWindow()->GetDisAsm()->SetAddress(address);
}

void CDebugger::OnFunctionsViewFunctionsStateChange()
{
	//GetDisassemblyWindow()->HandleMachineStateChange();
	//GetCallStackWindow()->HandleMachineStateChange();
}

void CDebugger::OnThreadsViewAddressDblClick(uint32 address)
{
	//auto disAsm = GetDisassemblyWindow()->GetDisAsm();
	//disAsm->SetCenterAtAddress(address);
	//disAsm->SetSelectedAddress(address);
}

void CDebugger::OnExecutableChange()
{
	//SendMessage(m_hWnd, WM_EXECCHANGE, 0, 0);
}

void CDebugger::OnExecutableUnloading()
{
	//SendMessage(m_hWnd, WM_EXECUNLOAD, 0, 0);
}

void CDebugger::OnMachineStateChange()
{
	this->OnMachineStateChangeMsg();
	//m_pView[DEBUGVIEW_EE]->HandleMachineStateChange();
	//PostMessage(m_hWnd, WM_MACHINESTATECHANGE, 0, 0);
}

void CDebugger::OnRunningStateChange()
{
	this->OnRunningStateChangeMsg();
	//m_pView[DEBUGVIEW_EE]->HandleRunningStateChange();
	//PostMessage(m_hWnd, WM_RUNNINGSTATECHANGE, 0, 0);
}

void CDebugger::OnFindCallersRequested(uint32 address)
{
	//auto context = GetCurrentView()->GetContext();
	//auto callers = FindCallers(context, address);
	/*auto title =
	    [&]() {
		    auto functionName = context->m_Functions.Find(address);
		    if(functionName)
		    {
			    return string_format(_T("Find Callers For '%s' (0x%08X)"),
			                         string_cast<std::tstring>(functionName).c_str(), address);
		    }
		    else
		    {
			    return string_format(_T("Find Callers For 0x%08X"), address);
		    }
	    }();
	*/
	//m_addressListView->SetAddressList(std::move(callers));
	//m_addressListView->SetTitle(std::move(title));
	//m_addressListView->Show(SW_SHOW);
	//m_addressListView->SetFocus();
}

void CDebugger::OnFindCallersAddressDblClick(uint32 address)
{
	//auto disAsm = GetDisassemblyWindow()->GetDisAsm();
	//disAsm->SetCenterAtAddress(address);
	//disAsm->SetSelectedAddress(address);
}

void CDebugger::OnExecutableChangeMsg()
{
	//m_pELFView->SetELF(m_virtualMachine.m_ee->m_os->GetELF());
	//	m_pFunctionsView->SetELF(m_virtualMachine.m_os->GetELF());

	LoadDebugTags();

	//GetDisassemblyWindow()->HandleMachineStateChange();
	//GetCallStackWindow()->HandleMachineStateChange();
	//m_pFunctionsView->Refresh();
}

void CDebugger::OnExecutableUnloadingMsg()
{
	SaveDebugTags();
	//m_pELFView->SetELF(NULL);
	//	m_pFunctionsView->SetELF(NULL);
}

void CDebugger::OnMachineStateChangeMsg()
{
	m_pView[DEBUGVIEW_EE]->HandleMachineStateChange();
	//for(auto& view : m_pView)
	//{
	//	view->HandleMachineStateChange();
	//}
	//m_threadsView->HandleMachineStateChange();
}

void CDebugger::OnRunningStateChangeMsg()
{
	//auto newState = m_virtualMachine.GetStatus();
	//m_pView[DEBUGVIEW_EE]->HandleMachineStateChange();
	//for(auto& view : m_pView)
	//{
		//if(view != nullptr)
			//view->HandleRunningStateChange(newState);
	//}
	//m_threadsView->HandleRunningStateChange(newState);
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
	//if(m_virtualMachine.m_ee != nullptr)
	//{
	//	if(m_virtualMachine.m_ee->m_os->GetELF() != nullptr)
	//	{
	//		m_virtualMachine.SaveDebugTags(m_virtualMachine.m_ee->m_os->GetExecutableName());
	//	}
	//}
#endif
}
