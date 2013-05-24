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
#include "string_cast.h"
#include "string_format.h"
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
	m_gs->Initialize();
	m_gs->Reset();

	SetMenu(LoadMenu(GetModuleHandle(NULL), MAKEINTRESOURCE(IDR_FRAMEDEBUGGER)));

	m_mainSplitter = std::make_unique<Framework::Win32::CHorizontalSplitter>(m_hWnd, GetClientRect());

	m_tab = std::make_unique<Framework::Win32::CTab>(*m_mainSplitter, GetClientRect());
	m_tab->InsertTab(_T("Context 1"));
	m_tab->InsertTab(_T("Context 2"));
	m_tab->InsertTab(_T("Input State"));
	m_tab->InsertTab(_T("VU1 Microprogram"));

	m_gsContextView = std::make_unique<CGsContextView>(*m_tab, GetClientRect(), m_gs.get());
	m_gsContextView->UpdateState();

	m_gsInputStateView = std::make_unique<CGsInputStateView>(*m_tab, GetClientRect());
	m_gsInputStateView->UpdateState(m_gs.get());

	m_tabItems[0] = m_gsContextView.get();
	m_tabItems[2] = m_gsInputStateView.get();

	OnTabSelChanged();

	m_packetsTreeView = std::make_unique<Framework::Win32::CTreeView>(*m_mainSplitter, Framework::Win32::CRect(0, 0, 300, 300), 
		TVS_LINESATROOT | TVS_HASBUTTONS | TVS_SHOWSELALWAYS | TVS_HASLINES);

	m_mainSplitter->SetChild(0, *m_packetsTreeView);
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
	if(CWindow::IsNotifySource(m_packetsTreeView.get(), header))
	{
		switch(header->code)
		{
		case TVN_SELCHANGED:
			OnPacketsTreeViewSelChanged();
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

void CFrameDebugger::OnPacketsTreeViewSelChanged()
{
	HTREEITEM selectedItem = m_packetsTreeView->GetSelection();
	uint32 cmdIndex = m_packetsTreeView->GetItemParam<uint32>(selectedItem);
	UpdateDisplay(cmdIndex);
}

void CFrameDebugger::UpdateDisplay(int32 targetCmdIndex)
{
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

	m_gsInputStateView->UpdateState(m_gs.get());
	m_gsContextView->UpdateState();
}

void CFrameDebugger::LoadFrameDump(const TCHAR* dumpPathName)
{
	boost::filesystem::path dumpPath(dumpPathName);
	auto inputStream = Framework::CreateInputStdStream(dumpPath.native());
	m_frameDump.Read(inputStream);

	IdentifyDrawingKicks();

	uint32 cmdIndex = 0;
	m_packetsTreeView->DeleteAllItems();
	for(const auto& packet : m_frameDump.GetPackets())
	{
		auto lowerBoundIterator = m_drawingKickIndices.upper_bound(cmdIndex);
		auto upperBoundIterator = m_drawingKickIndices.lower_bound(cmdIndex + packet.size());

		int kickCount = static_cast<int>(std::distance(lowerBoundIterator, upperBoundIterator));
		
		auto packetDescription = string_format("Packet (Write Count: %d, Draw Count: %d)", packet.size(), kickCount);
		HTREEITEM packetRootItem = m_packetsTreeView->InsertItem(TVI_ROOT, string_cast<std::tstring>(packetDescription).c_str());
		m_packetsTreeView->SetItemParam(packetRootItem, cmdIndex);

		for(const auto& registerWrite : packet)
		{
//			std::tstring packetWriteDescription = _T("Register Write (reg: 0x") + lexical_cast_hex<std::tstring>(registerWrite.first) + _T(")");
			auto packetWriteDescription = CGSHandler::GetWriteDescription(registerWrite.first, registerWrite.second);
			if(m_drawingKickIndices.find(cmdIndex) != std::end(m_drawingKickIndices))
			{
				packetWriteDescription = "(DK) " + packetWriteDescription;
			}
			HTREEITEM newItem = m_packetsTreeView->InsertItem(packetRootItem, string_cast<std::tstring>(packetWriteDescription).c_str());
			m_packetsTreeView->SetItemParam(newItem, cmdIndex++);
		}
	}

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

void CFrameDebugger::IdentifyDrawingKicks()
{
	static const unsigned int g_initVertexCounts[8] = { 1, 2, 2, 3, 3, 3, 2, 0 };
	static const unsigned int g_nextVertexCounts[8] = { 1, 2, 1, 3, 1, 1, 2, 0 };

	CGSHandler::PRIM currentPrim;
	currentPrim <<= m_gs->GetRegisters()[GS_REG_PRIM];

	unsigned int vertexCount = g_initVertexCounts[currentPrim.nType];

	uint32 cmdIndex = 0;
	for(const auto& packet : m_frameDump.GetPackets())
	{
		for(const auto& registerWrite : packet)
		{
			if(registerWrite.first == GS_REG_PRIM)
			{
				currentPrim <<= registerWrite.second;
				vertexCount = g_initVertexCounts[currentPrim.nType];
			}
			else if(
				(registerWrite.first == GS_REG_XYZ2) || 
				(registerWrite.first == GS_REG_XYZ3) ||
				(registerWrite.first == GS_REG_XYZF2) ||
				(registerWrite.first == GS_REG_XYZF3))
			{
				if(vertexCount != 0)
				{
					vertexCount--;
					if(vertexCount == 0)
					{
						bool drawingKick = (registerWrite.first == GS_REG_XYZ2) || (registerWrite.first == GS_REG_XYZF2);
						vertexCount = g_nextVertexCounts[currentPrim.nType];
						if(drawingKick)
						{
							m_drawingKickIndices.insert(cmdIndex);
						}
					}
				}
			}

			cmdIndex++;
		}
	}
}
