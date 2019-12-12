#include "QtDebugger.h"
#include "ui_QtDebugger.h"

#include <QInputDialog>
#include <QMessageBox>

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
#include "xml/Parser.h"
#include "xml/Utils.h"
#include "string_cast.h"
#include "string_format.h"


#define PREF_DEBUGGER_MEMORYVIEW_BYTEWIDTH "debugger.memoryview.bytewidth"

#define FIND_MAX_ADDRESS 0x02000000

QtDebugger::QtDebugger(QWidget *parent, CPS2VM& virtualMachine)
    : QMainWindow(parent)
    , ui(new Ui::QtDebugger)
    , m_virtualMachine(virtualMachine)
{
	ui->setupUi(this);

	// Setup QT Stuff
	RegisterPreferences();

	//ELF View Initialization
	//m_pELFView = new CELFView(m_pMDIClient->m_hWnd);
	//m_pELFView->Show(SW_HIDE);

	//Functions View Initialization
	m_pFunctionsView = new CFunctionsView(ui->mdiArea);
	m_pFunctionsView->hide();
	m_OnFunctionDblClickConnection = m_pFunctionsView->OnFunctionDblClick.Connect(std::bind(&QtDebugger::OnFunctionsViewFunctionDblClick, this, std::placeholders::_1));
	m_OnFunctionsStateChangeConnection = m_pFunctionsView->OnFunctionsStateChange.Connect(std::bind(&QtDebugger::OnFunctionsViewFunctionsStateChange, this));

	//Threads View Initialization
	m_threadsView = new CThreadsViewWnd(ui->mdiArea);
	m_threadsView->hide();
	m_OnGotoAddressConnection = m_threadsView->OnGotoAddress.Connect(std::bind(&QtDebugger::OnThreadsViewAddressDblClick, this, std::placeholders::_1));

	//Address List View Initialization
	m_addressListView = new CAddressListViewWnd(ui->mdiArea);
	m_addressListView->hide();
	m_AddressSelectedConnection = m_addressListView->AddressSelected.Connect([&](uint32 address) { OnFindCallersAddressDblClick(address); });

	//Debug Views Initialization
	m_nCurrentView = -1;

	m_pView[DEBUGVIEW_EE] = new CDebugView(ui->mdiArea, m_virtualMachine, &m_virtualMachine.m_ee->m_EE,
	                                       std::bind(&CPS2VM::StepEe, &m_virtualMachine), m_virtualMachine.m_ee->m_os, "EmotionEngine");
	m_pView[DEBUGVIEW_VU0] = new CDebugView(ui->mdiArea, m_virtualMachine, &m_virtualMachine.m_ee->m_VU0,
	                                       std::bind(&CPS2VM::StepVu0, &m_virtualMachine), nullptr, "Vector Unit 0", CQtDisAsmTableModel::DISASM_VU);
	m_pView[DEBUGVIEW_VU1] = new CDebugView(ui->mdiArea, m_virtualMachine, &m_virtualMachine.m_ee->m_VU1,
	                                       std::bind(&CPS2VM::StepVu1, &m_virtualMachine), nullptr, "Vector Unit 1", CQtDisAsmTableModel::DISASM_VU);
	m_pView[DEBUGVIEW_IOP] = new CDebugView(ui->mdiArea, m_virtualMachine, &m_virtualMachine.m_iop->m_cpu,
	                                       std::bind(&CPS2VM::StepIop, &m_virtualMachine), m_virtualMachine.m_iop->m_bios.get(), "IO Processor");

	m_OnExecutableChangeConnection = m_virtualMachine.m_ee->m_os->OnExecutableChange.Connect(std::bind(&QtDebugger::OnExecutableChange, this));
	m_OnExecutableUnloadingConnection = m_virtualMachine.m_ee->m_os->OnExecutableUnloading.Connect(std::bind(&QtDebugger::OnExecutableUnloading, this));

	m_OnMachineStateChangeConnection = m_virtualMachine.OnMachineStateChange.Connect(std::bind(&QtDebugger::OnMachineStateChange, this));
	m_OnRunningStateChangeConnection = m_virtualMachine.OnRunningStateChange.Connect(std::bind(&QtDebugger::OnRunningStateChange, this));

	ActivateView(DEBUGVIEW_EE);
	LoadSettings();

	if(GetDisassemblyWindow()->isVisible())
	{
		GetDisassemblyWindow()->setFocus(Qt::ActiveWindowFocusReason);
	}

	CreateAccelerators();
}

QtDebugger::~QtDebugger()
{
	delete ui;

	OnExecutableUnloadingMsg();

	DestroyAccelerators();

	SaveSettings();

	//Destroy explicitly since we're keeping the window alive
	//artificially by handling WM_SYSCOMMAND
	//Destroy();

	for(unsigned int i = 0; i < DEBUGVIEW_MAX; i++)
	{
		delete m_pView[i];
	}

	//delete m_pELFView;
	delete m_pFunctionsView;
}

//HACCEL QtDebugger::GetAccelerators()
//{
//	return m_nAccTable;
//}

void QtDebugger::RegisterPreferences()
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

void QtDebugger::UpdateTitle()
{
	std::string sTitle("Play! - Debugger");

	if(GetCurrentView() != NULL)
	{
		sTitle += (" - [ ");
		sTitle += GetCurrentView()->GetName();
		sTitle += (" ]");
	}

	setWindowTitle(sTitle.c_str());
}

void QtDebugger::LoadSettings()
{
	LoadViewLayout();
	LoadBytesPerLine();
}

void QtDebugger::SaveSettings()
{
	SaveViewLayout();
	SaveBytesPerLine();
}

void QtDebugger::SerializeWindowGeometry(QMdiSubWindow* pWindow, const char* sPosX, const char* sPosY, const char* sSizeX, const char* sSizeY, const char* sVisible)
{
	CAppConfig& config(CAppConfig::GetInstance());

	auto geometry = pWindow->geometry();

	config.SetPreferenceInteger(sPosX, geometry.x());
	config.SetPreferenceInteger(sPosY, geometry.y());

	//if(sSizeX != NULL && sSizeY != NULL)
	//{
		config.SetPreferenceInteger(sSizeX, geometry.width());
		config.SetPreferenceInteger(sSizeY, geometry.height());
	//}

	config.SetPreferenceBoolean(sVisible, pWindow->isVisible());
}

void QtDebugger::UnserializeWindowGeometry(QMdiSubWindow* pWindow, const char* sPosX, const char* sPosY, const char* sSizeX, const char* sSizeY, const char* sVisible)
{
	CAppConfig& config(CAppConfig::GetInstance());

	pWindow->setGeometry(config.GetPreferenceInteger(sPosX), config.GetPreferenceInteger(sPosY),
	                     config.GetPreferenceInteger(sSizeX), config.GetPreferenceInteger(sSizeY));

	if(!config.GetPreferenceBoolean(sVisible))
	{
		pWindow->hide();
	}
	else
	{
		pWindow->show();
	}
}

void QtDebugger::Resume()
{
	m_virtualMachine.Resume();
}

void QtDebugger::StepCPU()
{
	if(m_virtualMachine.GetStatus() == CVirtualMachine::RUNNING)
	{
		//MessageBeep(-1);
		return;
	}

	if(!GetDisassemblyWindow()->hasFocus())
	{
		GetDisassemblyWindow()->setFocus(Qt::ActiveWindowFocusReason);
	}

	
	GetCurrentView()->Step();
}

void QtDebugger::FindWordValue(uint32 mask)
{
	uint32 targetValue = 0;
	{
		bool ok;
		QString res = QInputDialog::getText(this, tr("Find Value in Memory"),
											tr("Enter value to find:"), QLineEdit::Normal,
											tr("00000000"), &ok);
		if (!ok  || res.isEmpty())
			return;

		if(sscanf(res.toStdString().c_str(), "%x", &targetValue) <= 0)
		{
			QMessageBox msgBox;
			msgBox.setText("Invalid hex value.");
			msgBox.exec();
			return;
		}
	}
	auto context = GetCurrentView()->GetContext();
	auto title = string_format("Search results for 0x%08X", targetValue);
	auto refs = FindWordValueRefs(context, targetValue & mask, mask);

	m_addressListView->SetTitle(std::move(title));
	m_addressListView->SetAddressList(std::move(refs));
	m_addressListView->show();
	m_addressListView->setFocus(Qt::ActiveWindowFocusReason);
}

void QtDebugger::AssembleJAL()
{
	uint32 nValueTarget = 0, nValueAssemble = 0;
	auto getAddress =
	   [this](const char* prompt, uint32& address)
	{
		bool ok;
		QString res = QInputDialog::getText(this, tr("Assemble JAL"),
											tr(prompt), QLineEdit::Normal,
											tr("00000000"), &ok);
		if (!ok  || res.isEmpty())
			return false;
		uint32 addrValueTemp = 0;
		if(sscanf(res.toStdString().c_str(), "%x", &address) <= 0)
		{
			QMessageBox msgBox;
			msgBox.setText("Invalid value.");
			msgBox.exec();
			return false;
		}
		return true;
	};
	if(!getAddress("Enter jump target:", nValueTarget)) return;
	if(!getAddress("Enter address to assemble JAL to:", nValueAssemble)) return;

	*(uint32*)&m_virtualMachine.m_ee->m_ram[nValueAssemble] = 0x0C000000 | (nValueTarget / 4);
}

void QtDebugger::ReanalyzeEe()
{
	if(m_virtualMachine.m_ee->m_os->GetELF() == nullptr) return;

	auto executableRange = m_virtualMachine.m_ee->m_os->GetExecutableRange();
	uint32 minAddr = executableRange.first;
	uint32 maxAddr = executableRange.second & ~0x03;

	auto getAddress =
	   [this](const char* prompt, uint32& address)
	{
		bool ok;
		QString res = QInputDialog::getText(this, tr("Analyze EE"),
											tr(prompt), QLineEdit::Normal,
											tr(string_format("0x%08X", address).c_str()), &ok);
		if (!ok  || res.isEmpty())
			return false;
		uint32 addrValueTemp = 0;
		if(sscanf(res.toStdString().c_str(), "%x", &addrValueTemp) <= 0)
		{
			QMessageBox msgBox;
			msgBox.setText("Invalid value.");
			msgBox.exec();
			return false;
		}
		if(addrValueTemp != 0)
		{
			address = addrValueTemp & ~0x3;
		}
		return true;
	};

	if(!getAddress("Start Address:", minAddr)) return;
	if(!getAddress("End Address:", maxAddr)) return;

	if(minAddr > maxAddr)
	{
		QMessageBox msgBox;
		msgBox.setText("Start address is larger than end address.");
		msgBox.exec();
		return;
	}

	minAddr = std::min<uint32>(minAddr, PS2::EE_RAM_SIZE);
	maxAddr = std::min<uint32>(maxAddr, PS2::EE_RAM_SIZE);

	m_virtualMachine.m_ee->m_EE.m_analysis->Clear();
	m_virtualMachine.m_ee->m_EE.m_analysis->Analyse(minAddr, maxAddr);
}

void QtDebugger::FindEeFunctions()
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

void QtDebugger::Layout1024()
{
	GetDisassemblyWindow()->setGeometry(0, 0, 700, 435);
	GetDisassemblyWindow()->show();

	GetRegisterViewWindow()->setGeometry(700, 0, 324, 572);
	GetRegisterViewWindow()->show();

	//GetMemoryViewWindow()->setGeometry(0, 435, 700, 265);
	//GetMemoryViewWindow()->show();

	GetCallStackWindow()->setGeometry(700, 572, 324, 128);
	GetCallStackWindow()->show();
}

void QtDebugger::Layout1280()
{
	GetDisassemblyWindow()->setGeometry(0, 0, 900, 540);
	GetDisassemblyWindow()->show();

	GetRegisterViewWindow()->setGeometry(900, 0, 380, 784);
	GetRegisterViewWindow()->show();

	//GetMemoryViewWindow()->setGeometry(0, 540, 900, 416);
	//GetMemoryViewWindow()->show();

	GetCallStackWindow()->setGeometry(900, 784, 380, 172);
	GetCallStackWindow()->show();
}

void QtDebugger::Layout1600()
{
	GetDisassemblyWindow()->setGeometry(0, 0, 1094, 725);
	GetDisassemblyWindow()->show();

	GetRegisterViewWindow()->setGeometry(1094, 0, 506, 725);
	GetRegisterViewWindow()->show();

	//GetMemoryViewWindow()->setGeometry(0, 725, 1094, 407);
	//GetMemoryViewWindow()->show();

	GetCallStackWindow()->setGeometry(1094, 725, 506, 407);
	GetCallStackWindow()->show();
}

void QtDebugger::InitializeConsole()
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

void QtDebugger::ActivateView(unsigned int nView)
{
	if(m_nCurrentView == nView) return;

	if(m_nCurrentView != -1)
	{
		SaveBytesPerLine();
		SaveViewLayout();
		GetCurrentView()->Hide();
	}

	m_findCallersRequestConnection.reset();

	m_nCurrentView = nView;
	LoadViewLayout();
	LoadBytesPerLine();
	UpdateTitle();

	{
		auto biosDebugInfoProvider = GetCurrentView()->GetBiosDebugInfoProvider();
		m_pFunctionsView->SetContext(GetCurrentView()->GetContext(), biosDebugInfoProvider);
		m_threadsView->SetContext(GetCurrentView()->GetContext(), biosDebugInfoProvider);
	}

	if(GetDisassemblyWindow()->isVisible())
	{
		GetDisassemblyWindow()->setFocus(Qt::ActiveWindowFocusReason);
	}

	m_findCallersRequestConnection = GetCurrentView()->GetDisassemblyWindow()->FindCallersRequested.Connect(
	   [&](uint32 address) { OnFindCallersRequested(address); });
}

void QtDebugger::SaveViewLayout()
{
	SerializeWindowGeometry(GetDisassemblyWindow(),
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

	// SerializeWindowGeometry(GetMemoryViewWindow(),
	//                         "debugger.memoryview.posx",
	//                         "debugger.memoryview.posy",
	//                         "debugger.memoryview.sizex",
	//                         "debugger.memoryview.sizey",
	//                         "debugger.memoryview.visible");

	SerializeWindowGeometry(GetCallStackWindow(),
	                        "debugger.callstack.posx",
	                        "debugger.callstack.posy",
	                        "debugger.callstack.sizex",
	                        "debugger.callstack.sizey",
	                        "debugger.callstack.visible");
}

void QtDebugger::LoadViewLayout()
{
	// UnserializeWindowGeometry(GetDisassemblyWindow(),
	//                           "debugger.disasm.posx",
	//                           "debugger.disasm.posy",
	//                           "debugger.disasm.sizex",
	//                           "debugger.disasm.sizey",
	//                           "debugger.disasm.visible");

	UnserializeWindowGeometry(GetRegisterViewWindow(),
	                          "debugger.regview.posx",
	                          "debugger.regview.posy",
	                          "debugger.regview.sizex",
	                          "debugger.regview.sizey",
	                          "debugger.regview.visible");

	// UnserializeWindowGeometry(GetMemoryViewWindow(),
	//                           "debugger.memoryview.posx",
	//                           "debugger.memoryview.posy",
	//                           "debugger.memoryview.sizex",
	//                           "debugger.memoryview.sizey",
	//                           "debugger.memoryview.visible");

	UnserializeWindowGeometry(GetCallStackWindow(),
	                          "debugger.callstack.posx",
	                          "debugger.callstack.posy",
	                          "debugger.callstack.sizex",
	                          "debugger.callstack.sizey",
	                          "debugger.callstack.visible");
}

void QtDebugger::SaveBytesPerLine()
{
	//auto memoryView = GetMemoryViewWindow()->GetMemoryView();
	//auto bytesPerLine = memoryView->GetBytesPerLine();
	//CAppConfig::GetInstance().SetPreferenceInteger(PREF_DEBUGGER_MEMORYVIEW_BYTEWIDTH, bytesPerLine);
}

void QtDebugger::LoadBytesPerLine()
{
	//auto bytesPerLine = CAppConfig::GetInstance().GetPreferenceInteger(PREF_DEBUGGER_MEMORYVIEW_BYTEWIDTH);
	//auto memoryView = GetMemoryViewWindow()->GetMemoryView();
	//memoryView->SetBytesPerLine(bytesPerLine);
}

CDebugView* QtDebugger::GetCurrentView()
{
	if(m_nCurrentView == -1) return NULL;
	return m_pView[m_nCurrentView];
}

CMIPS* QtDebugger::GetContext()
{
	return nullptr;
	return GetCurrentView()->GetContext();
}

CDisAsmWnd* QtDebugger::GetDisassemblyWindow()
{
	return GetCurrentView()->GetDisassemblyWindow();
}

/*
CMemoryViewMIPSWnd* QtDebugger::GetMemoryViewWindow()
{
	return GetCurrentView()->GetMemoryViewWindow();
}
*/

CRegViewWnd* QtDebugger::GetRegisterViewWindow()
{
	return GetCurrentView()->GetRegisterViewWindow();
}

CCallStackWnd* QtDebugger::GetCallStackWindow()
{
	return GetCurrentView()->GetCallStackWindow();
}

std::vector<uint32> QtDebugger::FindCallers(CMIPS* context, uint32 address)
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

std::vector<uint32> QtDebugger::FindWordValueRefs(CMIPS* context, uint32 targetValue, uint32 valueMask)
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

void QtDebugger::CreateAccelerators()
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

void QtDebugger::DestroyAccelerators()
{
	//DestroyAcceleratorTable(m_nAccTable);
}

/*long QtDebugger::OnCommand(unsigned short nID, unsigned short nMsg, HWND hFrom)
{
	switch(nID)
	{
	case ID_VIEW_MEMORY:
		GetMemoryViewWindow()->Show(SW_SHOW);
		GetMemoryViewWindow()->setFocus(Qt::ActiveWindowFocusReason);
		return FALSE;
		break;
	case ID_VIEW_ELF:
		m_pELFView->Show(SW_SHOW);
		m_pELFView->setFocus(Qt::ActiveWindowFocusReason);
		return FALSE;
		break;
	return TRUE;
}*/

/*
LRESULT QtDebugger::OnWndProc(unsigned int nMsg, WPARAM wParam, LPARAM lParam)
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

void QtDebugger::OnFunctionsViewFunctionDblClick(uint32 address)
{
	GetDisassemblyWindow()->SetAddress(address);
}

void QtDebugger::OnFunctionsViewFunctionsStateChange()
{
	GetDisassemblyWindow()->HandleMachineStateChange();
	GetCallStackWindow()->HandleMachineStateChange();
}

void QtDebugger::OnThreadsViewAddressDblClick(uint32 address)
{
	auto disAsm = GetDisassemblyWindow();
	disAsm->SetCenterAtAddress(address);
	disAsm->SetSelectedAddress(address);
}

void QtDebugger::OnExecutableChange()
{
	//SendMessage(m_hWnd, WM_EXECCHANGE, 0, 0);
}

void QtDebugger::OnExecutableUnloading()
{
	//SendMessage(m_hWnd, WM_EXECUNLOAD, 0, 0);
}

void QtDebugger::OnMachineStateChange()
{
	this->OnMachineStateChangeMsg();
}

void QtDebugger::OnRunningStateChange()
{
	this->OnRunningStateChangeMsg();
}

void QtDebugger::OnFindCallersRequested(uint32 address)
{
	auto context = GetCurrentView()->GetContext();
	auto callers = FindCallers(context, address);
	auto title =
	    [&]() {
		    auto functionName = context->m_Functions.Find(address);
		    if(functionName)
		    {
			    return string_format("Find Callers For '%s' (0x%08X)",
			                         functionName, address);
		    }
		    else
		    {
			    return string_format("Find Callers For 0x%08X", address);
		    }
	    }();
	
	m_addressListView->SetAddressList(std::move(callers));
	m_addressListView->SetTitle(std::move(title));
	m_addressListView->show();
	m_addressListView->setFocus(Qt::ActiveWindowFocusReason);
}

void QtDebugger::OnFindCallersAddressDblClick(uint32 address)
{
	auto disAsm = GetDisassemblyWindow();
	disAsm->SetCenterAtAddress(address);
	disAsm->SetSelectedAddress(address);
}

void QtDebugger::OnExecutableChangeMsg()
{
	//m_pELFView->SetELF(m_virtualMachine.m_ee->m_os->GetELF());
		// m_pFunctionsView->SetELF(m_virtualMachine.m_os->GetELF());

	LoadDebugTags();

	GetDisassemblyWindow()->HandleMachineStateChange();
	GetCallStackWindow()->HandleMachineStateChange();
	m_pFunctionsView->Refresh();
}

void QtDebugger::OnExecutableUnloadingMsg()
{
	SaveDebugTags();
	//m_pELFView->SetELF(NULL);
		// m_pFunctionsView->SetELF(NULL);
}

void QtDebugger::OnMachineStateChangeMsg()
{
	for(auto& view : m_pView)
	{
		view->HandleMachineStateChange();
	}
	m_threadsView->HandleMachineStateChange();
}

void QtDebugger::OnRunningStateChangeMsg()
{
	auto newState = m_virtualMachine.GetStatus();
	for(auto& view : m_pView)
	{
		view->HandleRunningStateChange(newState);
	}
	m_threadsView->HandleRunningStateChange(newState);
}

void QtDebugger::LoadDebugTags()
{
#ifdef DEBUGGER_INCLUDED
	m_virtualMachine.LoadDebugTags(m_virtualMachine.m_ee->m_os->GetExecutableName());
#endif
}

void QtDebugger::SaveDebugTags()
{
#ifdef DEBUGGER_INCLUDED
	if(m_virtualMachine.m_ee != nullptr)
	{
		if(m_virtualMachine.m_ee->m_os->GetELF() != nullptr)
		{
			m_virtualMachine.SaveDebugTags(m_virtualMachine.m_ee->m_os->GetExecutableName());
		}
	}
#endif
}

void QtDebugger::on_actionResume_triggered()
{
	Resume();
}

void QtDebugger::on_actionStep_CPU_triggered()
{
	StepCPU();
}

void QtDebugger::on_actionDump_INTC_Handlers_triggered()
{
	m_virtualMachine.DumpEEIntcHandlers();
}

void QtDebugger::on_actionDump_DMAC_Handlers_triggered()
{
	m_virtualMachine.DumpEEDmacHandlers();
}

void QtDebugger::on_actionAssemble_JAL_triggered()
{
	AssembleJAL();
}

void QtDebugger::on_actionReanalyse_ee_triggered()
{
	ReanalyzeEe();
}

void QtDebugger::on_actionFind_Functions_triggered()
{
	FindEeFunctions();
}

void QtDebugger::on_actionCascade_triggered()
{
	ui->mdiArea->cascadeSubWindows();
}

void QtDebugger::on_actionTile_triggered()
{
	ui->mdiArea->tileSubWindows();
}

void QtDebugger::on_actionLayout_1024x768_triggered()
{
	Layout1024();
}

void QtDebugger::on_actionLayout_1280x1024_triggered()
{
	Layout1280();
}

void QtDebugger::on_actionLayout_1600x1200_triggered()
{
	Layout1600();
}

void QtDebugger::on_actionfind_word_value_triggered()
{
	FindWordValue(~0);
}

void QtDebugger::on_actionFind_Word_Half_Value_triggered()
{
	FindWordValue(0xFFFF);
}

void QtDebugger::on_actionCall_Stack_triggered()
{
	GetCallStackWindow()->show();
	GetCallStackWindow()->setFocus(Qt::ActiveWindowFocusReason);
}

void QtDebugger::on_actionFunctions_triggered()
{
	m_pFunctionsView->show();
	m_pFunctionsView->setFocus(Qt::ActiveWindowFocusReason);
}

void QtDebugger::on_actionThreads_triggered()
{
	m_threadsView->show();
	m_threadsView->setFocus(Qt::ActiveWindowFocusReason);
}

void QtDebugger::on_actionView_Disassmebly_triggered()
{
	GetDisassemblyWindow()->show();
	GetDisassemblyWindow()->setFocus(Qt::ActiveWindowFocusReason);
}

void QtDebugger::on_actionView_Registers_triggered()
{
	GetRegisterViewWindow()->show();
	GetRegisterViewWindow()->setFocus(Qt::ActiveWindowFocusReason);
}

void QtDebugger::on_actionEmotionEngine_View_triggered()
{
	ActivateView(DEBUGVIEW_EE);
}

void QtDebugger::on_actionVector_Unit_0_triggered()
{
	ActivateView(DEBUGVIEW_VU0);
}

void QtDebugger::on_actionVector_Unti_1_triggered()
{
	ActivateView(DEBUGVIEW_VU1);
}

void QtDebugger::on_actionIOP_View_triggered()
{
	ActivateView(DEBUGVIEW_IOP);
}
