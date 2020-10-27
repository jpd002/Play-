#include "filesystem_def.h"
#include "AppConfig.h"
#include "FrameDebugger.h"
#include "win32/AcceleratorTableGenerator.h"
#include "win32/Rect.h"
#include "win32/HorizontalSplitter.h"
#include "win32/FileDialog.h"
#include "win32/MenuItem.h"
#include "../../resource.h"
#include "StdStreamUtils.h"
#include "string_cast.h"
#include "string_format.h"

#define WNDSTYLE (WS_CLIPCHILDREN | WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SIZEBOX)
#define WNDTITLE _T("Play! - Frame Debugger")

#define PREF_FRAMEDEBUGGER_FRAMEBUFFER_DISPLAYMODE "framedebugger.framebuffer.displaymode"

CFrameDebugger::CFrameDebugger()
    : m_accTable(nullptr)
{
	CAppConfig::GetInstance().RegisterPreferenceInteger(PREF_FRAMEDEBUGGER_FRAMEBUFFER_DISPLAYMODE, CGsContextView::FB_DISPLAY_MODE_RAW);
	m_fbDisplayMode = static_cast<CGsContextView::FB_DISPLAY_MODE>(CAppConfig::GetInstance().GetPreferenceInteger(PREF_FRAMEDEBUGGER_FRAMEBUFFER_DISPLAYMODE));

	Create(0, Framework::Win32::CDefaultWndClass::GetName(), WNDTITLE, WNDSTYLE, Framework::Win32::CRect(0, 0, 1024, 768), nullptr, nullptr);
	SetClassPtr();

	m_handlerOutputWindow = std::make_unique<COutputWnd>(m_hWnd);
	m_handlerOutputWindow->Show(SW_SHOW);

	m_gs = std::make_unique<CGSH_Direct3D9>(m_handlerOutputWindow.get());
	m_gs->SetLoggingEnabled(false);
	m_gs->Initialize();
	m_gs->Reset();

	memset(&m_currentMetadata, 0, sizeof(m_currentMetadata));

	SetMenu(LoadMenu(GetModuleHandle(NULL), MAKEINTRESOURCE(IDR_FRAMEDEBUGGER)));

	m_mainSplitter = std::make_unique<Framework::Win32::CHorizontalSplitter>(m_hWnd, GetClientRect());

	m_packetListView = std::make_unique<CGsPacketListView>(*m_mainSplitter, GetClientRect());

	m_tab = std::make_unique<CTabHost>(*m_mainSplitter, GetClientRect());

	m_gsContextView0 = std::make_unique<CGsContextView>(*m_tab, GetClientRect(), m_gs.get(), 0);
	m_gsContextView0->SetFbDisplayMode(m_fbDisplayMode);
	m_gsContextView0->Show(SW_HIDE);

	m_gsContextView1 = std::make_unique<CGsContextView>(*m_tab, GetClientRect(), m_gs.get(), 1);
	m_gsContextView1->SetFbDisplayMode(m_fbDisplayMode);
	m_gsContextView1->Show(SW_HIDE);

	m_gsInputStateView = std::make_unique<CGsInputStateView>(*m_tab, GetClientRect());

	m_vu1ProgramView = std::make_unique<CVu1ProgramView>(*m_tab, GetClientRect(), m_vu1vm);
	m_vu1ProgramView->Show(SW_HIDE);

	m_tab->InsertTab(_T("Context 1"), m_gsContextView0.get());
	m_tab->InsertTab(_T("Context 2"), m_gsContextView1.get());
	m_tab->InsertTab(_T("Input State"), m_gsInputStateView.get());
	m_tab->InsertTab(_T("VU1 Microprogram"), m_vu1ProgramView.get());

	UpdateMenus();

	m_mainSplitter->SetChild(0, *m_packetListView);
	m_mainSplitter->SetChild(1, *m_tab);

	CreateAcceleratorTables();
}

CFrameDebugger::~CFrameDebugger()
{
	m_gs->Release();

	if(m_accTable != NULL)
	{
		DestroyAcceleratorTable(m_accTable);
	}

	//Destroy explicitly since we're keeping the window alive
	//artificially by handling WM_SYSCOMMAND
	Destroy();
}

HACCEL CFrameDebugger::GetAccelerators()
{
	return m_accTable;
}

long CFrameDebugger::OnSize(unsigned int type, unsigned int x, unsigned int y)
{
	if(m_mainSplitter)
	{
		Framework::Win32::CRect splitterRect = GetClientRect();
		splitterRect.Inflate(-10, -10);
		m_mainSplitter->SetSizePosition(splitterRect);
	}
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
		case ID_FD_SETTINGS_ALPHATEST:
			ToggleAlphaTest();
			break;
		case ID_FD_SETTINGS_DEPTHTEST:
			ToggleDepthTest();
			break;
		case ID_FD_SETTINGS_ALPHABLEND:
			ToggleAlphaBlending();
			break;
		case ID_FD_SETTINGS_FB_RAW:
			SetFbDisplayMode(CGsContextView::FB_DISPLAY_MODE_RAW);
			break;
		case ID_FD_SETTINGS_FB_448I:
			SetFbDisplayMode(CGsContextView::FB_DISPLAY_MODE_448I);
			break;
		case ID_FD_SETTINGS_FB_448P:
			SetFbDisplayMode(CGsContextView::FB_DISPLAY_MODE_448P);
			break;
		case ID_FD_VU1_STEP:
			StepVu1();
			break;
		}
	}
	return TRUE;
}

LRESULT CFrameDebugger::OnNotify(WPARAM param, NMHDR* header)
{
	if(!IsWindowEnabled(m_hWnd))
	{
		return FALSE;
	}

	if(CWindow::IsNotifySource(m_tab.get(), header))
	{
		switch(header->code)
		{
		case CTabHost::NOTIFICATION_SELCHANGED:
			UpdateCurrentTab();
			break;
		}
		return FALSE;
	}
	else if(CWindow::IsNotifySource(m_packetListView.get(), header))
	{
		switch(header->code)
		{
		case CGsPacketListView::NOTIFICATION_SELCHANGED:
		{
			auto selchangedInfo = reinterpret_cast<CGsPacketListView::SELCHANGED_INFO*>(header);
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
	case SC_KEYMENU:
		return FALSE;
	}
	return TRUE;
}

void CFrameDebugger::CreateAcceleratorTables()
{
	Framework::Win32::CAcceleratorTableGenerator generator;
	generator.Insert(ID_FD_VU1_STEP, VK_F10, FVIRTKEY);
	m_accTable = generator.Create();
}

void CFrameDebugger::UpdateMenus()
{
	{
		auto alphaTestMenuItem = Framework::Win32::CMenuItem::FindById(GetMenu(m_hWnd), ID_FD_SETTINGS_ALPHATEST);
		assert(!alphaTestMenuItem.IsEmpty());
		alphaTestMenuItem.Check(m_gs->GetAlphaTestingEnabled());
	}
	{
		auto depthTestMenuItem = Framework::Win32::CMenuItem::FindById(GetMenu(m_hWnd), ID_FD_SETTINGS_DEPTHTEST);
		assert(!depthTestMenuItem.IsEmpty());
		depthTestMenuItem.Check(m_gs->GetDepthTestingEnabled());
	}
	{
		auto alphaBlendMenuItem = Framework::Win32::CMenuItem::FindById(GetMenu(m_hWnd), ID_FD_SETTINGS_ALPHABLEND);
		assert(!alphaBlendMenuItem.IsEmpty());
		alphaBlendMenuItem.Check(m_gs->GetAlphaBlendingEnabled());
	}
	{
		auto fbDisplayModeRawMenuItem = Framework::Win32::CMenuItem::FindById(GetMenu(m_hWnd), ID_FD_SETTINGS_FB_RAW);
		assert(!fbDisplayModeRawMenuItem.IsEmpty());
		fbDisplayModeRawMenuItem.Check(m_fbDisplayMode == CGsContextView::FB_DISPLAY_MODE_RAW);
	}
	{
		auto fbDisplayMode448iMenuItem = Framework::Win32::CMenuItem::FindById(GetMenu(m_hWnd), ID_FD_SETTINGS_FB_448I);
		assert(!fbDisplayMode448iMenuItem.IsEmpty());
		fbDisplayMode448iMenuItem.Check(m_fbDisplayMode == CGsContextView::FB_DISPLAY_MODE_448I);
	}
	{
		auto fbDisplayMode448pMenuItem = Framework::Win32::CMenuItem::FindById(GetMenu(m_hWnd), ID_FD_SETTINGS_FB_448P);
		assert(!fbDisplayMode448pMenuItem.IsEmpty());
		fbDisplayMode448pMenuItem.Check(m_fbDisplayMode == CGsContextView::FB_DISPLAY_MODE_448P);
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
	m_gs->SetSMODE2(m_frameDump.GetInitialSMODE2());

	int32 cmdIndex = 0;
	for(const auto& packet : m_frameDump.GetPackets())
	{
		if((cmdIndex - 1) >= targetCmdIndex) break;

		m_currentMetadata = packet.metadata;

		if(packet.registerWrites.empty())
		{
			m_gs->ProcessWriteBuffer(nullptr);
			m_gs->FeedImageData(packet.imageData.data(), packet.imageData.size());
		}
		else
		{
			for(const auto& registerWrite : packet.registerWrites)
			{
				if((cmdIndex - 1) >= targetCmdIndex) break;
				m_gs->WriteRegister(registerWrite);
				cmdIndex++;
			}
		}
	}

	m_gs->ProcessWriteBuffer(nullptr);
	m_gs->Finish();

	const auto& drawingKicks = m_frameDump.GetDrawingKicks();
	if(!drawingKicks.empty())
	{
		auto prevKickIndexIterator = drawingKicks.lower_bound(targetCmdIndex);
		if((prevKickIndexIterator == std::end(drawingKicks)) || (prevKickIndexIterator->first != targetCmdIndex))
		{
			prevKickIndexIterator = std::prev(prevKickIndexIterator);
		}
		if(prevKickIndexIterator != std::end(drawingKicks))
		{
			m_currentDrawingKick = prevKickIndexIterator->second;
		}
		else
		{
			m_currentDrawingKick = DRAWINGKICK_INFO();
		}
	}
	else
	{
		m_currentDrawingKick = DRAWINGKICK_INFO();
	}
	UpdateCurrentTab();

	EnableWindow(m_hWnd, TRUE);
}

void CFrameDebugger::UpdateCurrentTab()
{
	if(m_tab->GetSelection() != -1)
	{
		if(auto tab = m_tab->GetTab(m_tab->GetSelection()))
		{
			auto debuggerTab = dynamic_cast<IFrameDebuggerTab*>(tab);
			debuggerTab->UpdateState(m_gs.get(), &m_currentMetadata, &m_currentDrawingKick);
		}
	}
}

void CFrameDebugger::LoadFrameDump(const TCHAR* dumpPathName)
{
	try
	{
		fs::path dumpPath(dumpPathName);
		auto inputStream = Framework::CreateInputStdStream(dumpPath.native());
		m_frameDump.Read(inputStream);
		m_frameDump.IdentifyDrawingKicks();
	}
	catch(const std::exception& exception)
	{
		std::string message = string_format("Failed to open frame dump:\r\n\r\n%s", exception.what());
		MessageBox(m_hWnd, string_cast<std::tstring>(message).c_str(), nullptr, MB_ICONERROR);
		return;
	}

	m_vu1vm.Reset();
	m_packetListView->SetFrameDump(&m_frameDump);

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

void CFrameDebugger::ToggleAlphaTest()
{
	m_gs->SetAlphaTestingEnabled(!m_gs->GetAlphaTestingEnabled());
	uint32 selectedItemIndex = m_packetListView->GetSelectedItemIndex();
	if(selectedItemIndex != -1)
	{
		UpdateDisplay(selectedItemIndex);
	}
	UpdateMenus();
}

void CFrameDebugger::ToggleDepthTest()
{
	m_gs->SetDepthTestingEnabled(!m_gs->GetDepthTestingEnabled());
	uint32 selectedItemIndex = m_packetListView->GetSelectedItemIndex();
	if(selectedItemIndex != -1)
	{
		UpdateDisplay(selectedItemIndex);
	}
	UpdateMenus();
}

void CFrameDebugger::ToggleAlphaBlending()
{
	m_gs->SetAlphaBlendingEnabled(!m_gs->GetAlphaBlendingEnabled());
	uint32 selectedItemIndex = m_packetListView->GetSelectedItemIndex();
	if(selectedItemIndex != -1)
	{
		UpdateDisplay(selectedItemIndex);
	}
	UpdateMenus();
}

void CFrameDebugger::SetFbDisplayMode(CGsContextView::FB_DISPLAY_MODE fbDisplayMode)
{
	m_gsContextView0->SetFbDisplayMode(fbDisplayMode);
	m_gsContextView1->SetFbDisplayMode(fbDisplayMode);
	m_fbDisplayMode = fbDisplayMode;
	CAppConfig::GetInstance().SetPreferenceInteger(PREF_FRAMEDEBUGGER_FRAMEBUFFER_DISPLAYMODE, fbDisplayMode);
	UpdateMenus();
}

void CFrameDebugger::StepVu1()
{
#ifdef DEBUGGER_INCLUDED
	if(m_currentMetadata.pathIndex != 1)
	{
		MessageBeep(-1);
		return;
	}

	const int vu1TabIndex = 3;
	if(m_tab->GetSelection() != vu1TabIndex)
	{
		m_tab->SetSelection(vu1TabIndex);
	}

	m_vu1ProgramView->StepVu1();
#endif
}
