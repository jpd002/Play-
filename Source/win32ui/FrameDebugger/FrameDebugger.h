#pragma once

#include "win32/Window.h"
#include "win32/Tab.h"
#include "win32/TreeView.h"
#include "win32/Static.h"
#include "win32/Splitter.h"
#include "../OutputWnd.h"
#include "../GSH_Direct3D9.h"
#include "../../FrameDump.h"
#include "GsInputStateView.h"
#include "GsContextView.h"
#include "GsRegisterWriteListView.h"

class CFrameDebugger : public Framework::Win32::CWindow
{
public:
													CFrameDebugger();
	virtual											~CFrameDebugger();

protected:
	long											OnSize(unsigned int, unsigned int, unsigned int) override;
	long											OnCommand(unsigned short, unsigned short, HWND) override;
	long											OnNotify(WPARAM, NMHDR*) override;
	long											OnSysCommand(unsigned int, LPARAM) override;

private:
	typedef std::vector<std::string> StatePathList;

	enum
	{
		MAX_TAB_ITEMS = 4
	};

	void											CreateAcceleratorTables();

	void											OnTabSelChanged();
	void											ResizeTabContents();

	void											UpdateMenus();
	void											UpdateDisplay(int32);
	void											UpdateCurrentTab();

	void											LoadFrameDump(const TCHAR*);
	void											ShowFrameDumpSelector();
	void											ToggleDepthTest();

	std::unique_ptr<CGSH_Direct3D9>					m_gs;
	CGsPacketMetadata								m_currentMetadata;
	CFrameDump										m_frameDump;

	std::unique_ptr<COutputWnd>						m_handlerOutputWindow;

	std::unique_ptr<Framework::Win32::CSplitter>	m_mainSplitter;

	std::unique_ptr<CGsRegisterWriteListView>		m_registerWriteListView;
	std::unique_ptr<Framework::Win32::CTab>			m_tab;

	std::unique_ptr<CGsContextView>					m_gsContextView0;
	std::unique_ptr<CGsContextView>					m_gsContextView1;
	std::unique_ptr<CGsInputStateView>				m_gsInputStateView;

	Framework::Win32::CWindow*						m_tabItems[MAX_TAB_ITEMS];
	unsigned int									m_currentSelection;
	
	HACCEL											m_accTable;
};
