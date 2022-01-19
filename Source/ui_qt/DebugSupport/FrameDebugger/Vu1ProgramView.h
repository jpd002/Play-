#pragma once

#include "../DisAsmWnd.h"
#include "../MemoryViewTable.h"
#include "../RegViewVU.h"
#include "Vu1Vm.h"
#include "gs/GSHandler.h"
#include "FrameDump.h"

#include "GifPacketView.h"

class CVu1ProgramView : public QWidget
{
	Q_OBJECT

public:
	CVu1ProgramView(QWidget*, CVu1Vm&);
	virtual ~CVu1ProgramView() = default;

	void UpdateState(CGSHandler*, CGsPacketMetadata*, DRAWINGKICK_INFO*);

	void StepVu1();

protected:
	void resizeEvent(QResizeEvent*) Q_DECL_OVERRIDE;
	void showEvent(QShowEvent*) Q_DECL_OVERRIDE;

signals:
	void OnMachineStateChange();
	void OnRunningStateChange();

private:
	void OnMachineStateChangeMsg();
	void OnRunningStateChangeMsg();

	CVu1Vm& m_virtualMachine;

	std::unique_ptr<CDisAsmWnd> m_disAsm;
	std::unique_ptr<CMemoryViewTable> m_memoryView;
	std::unique_ptr<CGifPacketView> m_packetView;
	std::unique_ptr<CRegViewVU> m_regView;

	uint32 m_vuMemPacketAddress;

	CVirtualMachine::MachineStateChangeEvent::Connection m_OnMachineStateChangeConnection;
	CVirtualMachine::RunningStateChangeEvent::Connection m_OnRunningStateChangeConnection;
};
