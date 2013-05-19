#pragma once

#include <set>
#include "win32/Window.h"
#include "win32/Tab.h"
#include "win32/TreeView.h"
#include "win32/Static.h"
#include "win32/Splitter.h"
#include "../OutputWnd.h"
#include "../../FrameDump.h"
#include "GsInputStateView.h"
#include "GsContextView.h"

class CFrameDebugger : public Framework::Win32::CWindow
{
public:
													CFrameDebugger();
	virtual											~CFrameDebugger();

protected:
	long											OnSize(unsigned int, unsigned int, unsigned int);
	long											OnCommand(unsigned short, unsigned short, HWND);
	long											OnNotify(WPARAM, NMHDR*);
	long											OnSysCommand(unsigned int, LPARAM);

private:
	typedef std::vector<std::string> StatePathList;

	enum
	{
		MAX_TAB_ITEMS = 4
	};

	void											CreateAcceleratorTables();

	void											OnTabSelChanged();
	void											ResizeTabContents();

	void											OnPacketsTreeViewSelChanged();

	void											UpdateDisplay(int32);

	void											LoadFrameDump(const TCHAR*);
	void											ShowFrameDumpSelector();

	void											IdentifyDrawingKicks();

	std::unique_ptr<CGSHandler>						m_gs;
	CFrameDump										m_frameDump;

	std::set<uint32>								m_drawingKickIndices;

	std::unique_ptr<COutputWnd>						m_handlerOutputWindow;

	std::unique_ptr<Framework::Win32::CSplitter>	m_mainSplitter;

	std::unique_ptr<Framework::Win32::CTreeView>	m_packetsTreeView;
	std::unique_ptr<Framework::Win32::CTab>			m_tab;

	std::unique_ptr<CGsContextView>					m_gsContextView;
	std::unique_ptr<CGsInputStateView>				m_gsInputStateView;

	Framework::Win32::CWindow*						m_tabItems[MAX_TAB_ITEMS];
	unsigned int									m_currentSelection;
	
	HACCEL											m_accTable;
};
