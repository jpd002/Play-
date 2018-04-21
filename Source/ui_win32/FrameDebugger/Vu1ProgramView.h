#pragma once

#include "../DisAsmVu.h"
#include "../MemoryViewMIPS.h"
#include "../RegViewVu.h"
#include "FrameDebuggerTab.h"
#include "GifPacketView.h"
#include "TabHost.h"
#include "Vu1Vm.h"
#include "win32/Splitter.h"
#include "win32/Window.h"

class CVu1ProgramView : public Framework::Win32::CWindow, public IFrameDebuggerTab
{
public:
	CVu1ProgramView(HWND, const RECT&, CVu1Vm&);
	virtual ~CVu1ProgramView();

	void UpdateState(CGSHandler*, CGsPacketMetadata*, DRAWINGKICK_INFO*) override;

	void StepVu1();

protected:
	long OnSize(unsigned int, unsigned int, unsigned int) override;

private:
	CVu1Vm&                                      m_virtualMachine;
	std::unique_ptr<Framework::Win32::CSplitter> m_mainSplitter;
	std::unique_ptr<Framework::Win32::CSplitter> m_subSplitter;

	std::unique_ptr<CTabHost> m_memoryTab;

	std::unique_ptr<CDisAsmVu>       m_disAsm;
	std::unique_ptr<CMemoryViewMIPS> m_memoryView;
	std::unique_ptr<CGifPacketView>  m_packetView;
	std::unique_ptr<CRegViewVU>      m_regView;

	uint32 m_vuMemPacketAddress;
};
