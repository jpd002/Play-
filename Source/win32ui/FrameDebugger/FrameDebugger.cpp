#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>
#include "FrameDebugger.h"
#include "win32/AcceleratorTableGenerator.h"
#include "win32/Rect.h"
#include "win32/HorizontalSplitter.h"
#include "win32/FileDialog.h"
#include "../resource.h"
#include "StdStreamUtils.h"
#include "lexical_cast_ex.h"
#include "../GSH_Direct3D9.h"

#define WNDSTYLE					(WS_CLIPCHILDREN | WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SIZEBOX)
#define WNDTITLE					_T("Play! - Frame Debugger")

CFrameDebugger::CFrameDebugger()
: m_accTable(nullptr)
, m_currentSelection(0)
{
	memset(&m_tabItems, 0, sizeof(m_tabItems));

	Create(0, Framework::Win32::CDefaultWndClass::GetName(), WNDTITLE, WNDSTYLE, Framework::Win32::CRect(0, 0, 1024, 768), nullptr, nullptr);
	SetClassPtr();

	m_handlerOutputWindow = std::make_unique<COutputWnd>(m_hWnd);
	m_handlerOutputWindow->Show(SW_SHOW);

	m_gs = std::make_unique<CGSH_Direct3D9>(m_handlerOutputWindow.get());
	m_gs->SetLoggingEnabled(false);
	m_gs->Initialize();
	m_gs->Reset();

	SetMenu(LoadMenu(GetModuleHandle(NULL), MAKEINTRESOURCE(IDR_FRAMEDEBUGGER)));

	m_mainSplitter = std::make_unique<Framework::Win32::CHorizontalSplitter>(m_hWnd, GetClientRect());

	m_registerWriteListView = std::make_unique<CGsRegisterWriteListView>(*m_mainSplitter, GetClientRect());

	m_tab = std::make_unique<Framework::Win32::CTab>(*m_mainSplitter, GetClientRect());
	m_tab->InsertTab(_T("Context 1"));
	m_tab->InsertTab(_T("Context 2"));
	m_tab->InsertTab(_T("Input State"));
	m_tab->InsertTab(_T("VU1 Microprogram"));

	m_gsContextView0 = std::make_unique<CGsContextView>(*m_tab, GetClientRect(), m_gs.get(), 0);
	m_gsContextView0->Show(SW_HIDE);

	m_gsContextView1 = std::make_unique<CGsContextView>(*m_tab, GetClientRect(), m_gs.get(), 1);
	m_gsContextView1->Show(SW_HIDE);

	m_gsInputStateView = std::make_unique<CGsInputStateView>(*m_tab, GetClientRect());

	m_tabItems[0] = m_gsContextView0.get();
	m_tabItems[1] = m_gsContextView1.get();
	m_tabItems[2] = m_gsInputStateView.get();

	OnTabSelChanged();

	m_mainSplitter->SetChild(0, *m_registerWriteListView);
	m_mainSplitter->SetChild(1, *m_tab);
}

CFrameDebugger::~CFrameDebugger()
{
	m_gs->Release();

	if(m_accTable != NULL)
	{
		DestroyAcceleratorTable(m_accTable);
	}
}

long CFrameDebugger::OnSize(unsigned int type, unsigned int x, unsigned int y)
{
	if(m_mainSplitter)
	{
		Framework::Win32::CRect splitterRect = GetClientRect();
		splitterRect.Inflate(-10, -10);
		m_mainSplitter->SetSizePosition(splitterRect);
	}
	ResizeTabContents();
	return TRUE;
}

long CFrameDebugger::OnCommand(unsigned short id, unsigned short msg, HWND hwndFrom)
{
	if(!IsWindowEnabled(m_hWnd))
	{
		return TRUE;
	}

	if(hwndFrom == NULL)
	{
		switch(id)
		{
		case ID_FD_FILE_LOADDUMP:
			ShowFrameDumpSelector();
			break;
//		case ID_VM_STEP:
//			Step();
//			break;
//		case ID_VM_NEXTMICRO:
//			NextMicro();
//			break;
		}
	}
	else if(CWindow::IsCommandSource(m_mainSplitter.get(), hwndFrom))
	{
		ResizeTabContents();
	}
	return TRUE;
}

long CFrameDebugger::OnNotify(WPARAM param, NMHDR* header)
{
	if(!IsWindowEnabled(m_hWnd))
	{
		return FALSE;
	}

	if(CWindow::IsNotifySource(m_tab.get(), header))
	{
		switch(header->code)
		{
		case TCN_SELCHANGE:
			OnTabSelChanged();
			break;
		}
		return FALSE;
	}
	else if(CWindow::IsNotifySource(m_registerWriteListView.get(), header))
	{
		switch(header->code)
		{
		case CGsRegisterWriteListView::NOTIFICATION_SELCHANGED:
			{
				auto selchangedInfo = reinterpret_cast<CGsRegisterWriteListView::SELCHANGED_INFO*>(header);
				UpdateDisplay(selchangedInfo->selectedCmdIndex);
			}
			break;
		}
		return FALSE;
	}
	return FALSE;
}

long CFrameDebugger::OnSysCommand(unsigned int nCmd, LPARAM lParam)
{
	switch(nCmd)
	{
	case SC_CLOSE:
		Show(SW_HIDE);
		return FALSE;
	}
	return TRUE;
}

void CFrameDebugger::CreateAcceleratorTables()
{
	Framework::Win32::CAcceleratorTableGenerator generator;
//	generator.Insert(ID_VM_STEP,				VK_F10, FVIRTKEY);
//	generator.Insert(ID_VM_NEXTMICRO,			VK_F5,	FVIRTKEY);
//	generator.Insert(ID_VM_SAVESTATE,           VK_F7,  FVIRTKEY);
//	generator.Insert(ID_VM_LOADSTATE,           VK_F8,  FVIRTKEY);
//	generator.Insert(ID_VIEW_FUNCTIONS,         'F',    FCONTROL | FVIRTKEY);
//	generator.Insert(ID_VM_STEP1,               VK_F10, FVIRTKEY);
//	generator.Insert(ID_VM_RESUME,              VK_F5,  FVIRTKEY);
//	generator.Insert(ID_VIEW_CALLSTACK,         'A',    FCONTROL | FVIRTKEY);
//	generator.Insert(ID_VIEW_EEVIEW,            '1',    FALT | FVIRTKEY);
//	generator.Insert(ID_VIEW_VU0VIEW,           '2',    FALT | FVIRTKEY);
//	generator.Insert(ID_VIEW_VU1VIEW,           '3',    FALT | FVIRTKEY);
//	generator.Insert(ID_VIEW_IOPVIEW,           '4',    FALT | FVIRTKEY);
//	generator.Insert(ID_EDIT_COPY,              'C',    FCONTROL | FVIRTKEY);
//	generator.Insert(ID_VIEW_TESTENGINECONSOLE, 'T',    FCONTROL | FVIRTKEY);
	m_accTable = generator.Create();
}

void CFrameDebugger::OnTabSelChanged()
{
	int selectedTab = m_tab->GetSelection();
	if(m_currentSelection != -1)
	{
		auto tabItem(m_tabItems[m_currentSelection]);
		if(tabItem)
		{
			tabItem->Show(SW_HIDE);
		}
	}

	assert(selectedTab < MAX_TAB_ITEMS);
	m_currentSelection = selectedTab;
	UpdateCurrentTab();

	auto tabItem(m_tabItems[m_currentSelection]);
	if(tabItem)
	{
		tabItem->SetSizePosition(m_tab->GetDisplayAreaRect());
		tabItem->Show(SW_SHOW);
	}
}

void CFrameDebugger::ResizeTabContents()
{
	if(m_currentSelection != -1)
	{
		auto tabItem(m_tabItems[m_currentSelection]);
		if(tabItem)
		{
			tabItem->SetSizePosition(m_tab->GetDisplayAreaRect());
		}
	}
}

void CFrameDebugger::UpdateDisplay(int32 targetCmdIndex)
{
	EnableWindow(m_hWnd, FALSE);

	m_gs->Reset();

	uint8* gsRam = m_gs->GetRam();
	uint64* gsRegisters = m_gs->GetRegisters();
	memcpy(gsRam, m_frameDump.GetInitialGsRam(), CGSHandler::RAMSIZE);
	memcpy(gsRegisters, m_frameDump.GetInitialGsRegisters(), CGSHandler::REGISTER_MAX * sizeof(uint64));

	CFrameDump::Packet writes;

	int32 cmdIndex = 0;
	for(const auto& packet : m_frameDump.GetPackets())
	{
		if((cmdIndex - 1) >= targetCmdIndex) break;

		for(const auto& registerWrite : packet)
		{
			if((cmdIndex - 1) >= targetCmdIndex) break;
			writes.push_back(registerWrite);
			cmdIndex++;
		}
	}

	m_gs->WriteRegisterMassively(writes.data(), writes.size());
	m_gs->Flip();

	UpdateCurrentTab();

	EnableWindow(m_hWnd, TRUE);
}

void CFrameDebugger::UpdateCurrentTab()
{
	if(m_currentSelection != -1 && m_tabItems[m_currentSelection])
	{
		auto debuggerTab = dynamic_cast<IFrameDebuggerTab*>(m_tabItems[m_currentSelection]);
		debuggerTab->UpdateState(m_gs.get());
	}
}

void CFrameDebugger::LoadFrameDump(const TCHAR* dumpPathName)
{
	boost::filesystem::path dumpPath(dumpPathName);
	auto inputStream = Framework::CreateInputStdStream(dumpPath.native());
	m_frameDump.Read(inputStream);

	m_registerWriteListView->SetFrameDump(&m_frameDump);

	UpdateDisplay(0);
}

void CFrameDebugger::ShowFrameDumpSelector()
{
	Framework::Win32::CFileDialog fileDialog;
	fileDialog.m_OFN.lpstrFilter = _T("Play! Frame Dumps (*.dmp.zip)\0*.dmp.zip\0All files (*.*)\0*.*\0");
	unsigned int result = fileDialog.SummonOpen(m_hWnd);
	if(result == IDOK)
	{
		LoadFrameDump(fileDialog.GetPath());
	}
}
