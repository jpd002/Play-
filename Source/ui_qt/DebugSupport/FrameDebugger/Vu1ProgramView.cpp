#include "Vu1ProgramView.h"
#include "FrameDump.h"
#include "Ps2Const.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QTabWidget>

CVu1ProgramView::CVu1ProgramView(QWidget* parent, CVu1Vm& virtualMachine)
    : QWidget(parent)
    , m_virtualMachine(virtualMachine)
    , m_vuMemPacketAddress(0)
{
	m_OnMachineStateChangeConnection = virtualMachine.OnMachineStateChange.Connect(std::bind(&CVu1ProgramView::OnMachineStateChange, this));
	m_OnRunningStateChangeConnection = virtualMachine.OnRunningStateChange.Connect(std::bind(&CVu1ProgramView::OnRunningStateChange, this));

	auto v = new QVBoxLayout(this);
	auto h = new QHBoxLayout();

	m_disAsm = std::make_unique<CDisAsmWnd>(this, m_virtualMachine, m_virtualMachine.GetVu1Context(), "Vector Unit 1", CQtDisAsmTableModel::DISASM_VU);
	h->addWidget(m_disAsm.get());

	m_regView = std::make_unique<CRegViewVU>(this, m_virtualMachine.GetVu1Context());
	h->addWidget(m_regView.get());
	v->addLayout(h);

	m_memoryView = std::make_unique<CMemoryViewTable>(this);
	auto getByte = [ctx = m_virtualMachine.GetVu1Context()](uint32 address) {
		return ctx->m_pMemoryMap->GetByte(address);
	};

	m_memoryView->Setup(&m_virtualMachine, m_virtualMachine.GetVu1Context(), true);
	m_memoryView->SetData(getByte, PS2::VUMEM1SIZE);

	auto tab = new QTabWidget(this);
	v->addWidget(tab);

	m_packetView = std::make_unique<CGifPacketView>(this);
	tab->addTab(m_memoryView.get(), "VU Memory");
	tab->addTab(m_packetView.get(), "Packet");

	connect(this, &CVu1ProgramView::OnMachineStateChange, this, &CVu1ProgramView::OnMachineStateChangeMsg);
	connect(this, &CVu1ProgramView::OnRunningStateChange, this, &CVu1ProgramView::OnRunningStateChangeMsg);
}

void CVu1ProgramView::showEvent(QShowEvent* evt)
{
	QWidget::showEvent(evt);
	m_memoryView->ShowEvent();
}

void CVu1ProgramView::resizeEvent(QResizeEvent* evt)
{
	QWidget::resizeEvent(evt);
	m_memoryView->ResizeEvent();
}

void CVu1ProgramView::UpdateState(CGSHandler* gs, CGsPacketMetadata* metadata, DRAWINGKICK_INFO*)
{
#ifdef DEBUGGER_INCLUDED
	memcpy(m_virtualMachine.GetMicroMem1(), metadata->microMem1, PS2::MICROMEM1SIZE);
	memcpy(m_virtualMachine.GetVuMem1(), metadata->vuMem1, PS2::VUMEM1SIZE);
	memcpy(&m_virtualMachine.GetVu1Context()->m_State, &metadata->vu1State, sizeof(MIPSSTATE));
	m_virtualMachine.SetVpu1Top(metadata->vpu1Top);
	m_virtualMachine.SetVpu1Itop(metadata->vpu1Itop);
	m_vuMemPacketAddress = metadata->vuMemPacketAddress;
#endif
	OnMachineStateChange();
}

void CVu1ProgramView::StepVu1()
{
	m_virtualMachine.StepVu1();
}

void CVu1ProgramView::OnMachineStateChangeMsg()
{
	// we're triggering PAUSED status to force the view to move to current location
	auto newState = CVirtualMachine::STATUS::PAUSED;
	m_disAsm->HandleRunningStateChange(newState);
	m_memoryView->HandleRunningStateChange(newState);

	m_packetView->SetPacket(m_virtualMachine.GetVuMem1(), m_vuMemPacketAddress, PS2::VUMEM1SIZE - m_vuMemPacketAddress);
	m_regView->Update();
}

void CVu1ProgramView::OnRunningStateChangeMsg()
{
	auto newState = CVirtualMachine::STATUS::PAUSED;
	m_disAsm->HandleRunningStateChange(newState);
	m_memoryView->HandleRunningStateChange(newState);
}
