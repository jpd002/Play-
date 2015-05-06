#pragma once

#include "win32/Window.h"
#include "win32/Tab.h"
#include "win32/TreeView.h"
#include "win32/Static.h"
#include "win32/Splitter.h"
#include "../OutputWnd.h"
#include "../GSH_Direct3D9.h"
#include "../../FrameDump.h"
#include "Vu1Vm.h"
#include "GsInputStateView.h"
#include "GsContextView.h"
#include "GsRegisterWriteListView.h"
#include "Vu1ProgramView.h"
#include "TabHost.h"

class CFrameDebugger : public Framework::Win32::CWindow
{
public:
													CFrameDebugger();
	virtual											~CFrameDebugger();

	HACCEL											GetAccelerators();

protected:
	long											OnSize(unsigned int, unsigned int, unsigned int) override;
	long											OnCommand(unsigned short, unsigned short, HWND) override;
	long											OnNotify(WPARAM, NMHDR*) override;
	long											OnSysCommand(unsigned int, LPARAM) override;

private:
	typedef std::vector<std::string> StatePathList;

	void											CreateAcceleratorTables();

	void											UpdateMenus();
	void											UpdateDisplay(int32);
	void											UpdateCurrentTab();

	void											LoadFrameDump(const TCHAR*);
	void											ShowFrameDumpSelector();
	void											ToggleAlphaTest();
	void											ToggleDepthTest();
	void											ToggleAlphaBlending();
	void											SetFbDisplayMode(CGsContextView::FB_DISPLAY_MODE);
	void											StepVu1();

	std::unique_ptr<CGSH_Direct3D9>					m_gs;
	CGsPacketMetadata								m_currentMetadata;
	DRAWINGKICK_INFO								m_currentDrawingKick;
	CGsContextView::FB_DISPLAY_MODE					m_fbDisplayMode = CGsContextView::FB_DISPLAY_MODE_RAW;
	CFrameDump										m_frameDump;
	CVu1Vm											m_vu1vm;

	std::unique_ptr<COutputWnd>						m_handlerOutputWindow;

	std::unique_ptr<Framework::Win32::CSplitter>	m_mainSplitter;

	std::unique_ptr<CGsRegisterWriteListView>		m_registerWriteListView;
	std::unique_ptr<CTabHost>						m_tab;

	std::unique_ptr<CGsContextView>					m_gsContextView0;
	std::unique_ptr<CGsContextView>					m_gsContextView1;
	std::unique_ptr<CGsInputStateView>				m_gsInputStateView;
	std::unique_ptr<CVu1ProgramView>				m_vu1ProgramView;

	HACCEL											m_accTable;
};
