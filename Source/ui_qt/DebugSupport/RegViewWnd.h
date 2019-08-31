#pragma once

#include "VirtualMachineStateView.h"
#include <QTabWidget>

class CRegViewPage;
class CMIPS;

class CRegViewWnd : public QTabWidget, public CVirtualMachineStateView
{
public:
	CRegViewWnd(QWidget*, CMIPS*);
	virtual ~CRegViewWnd();

	void HandleMachineStateChange() override;
	void HandleRunningStateChange(CVirtualMachine::STATUS) override;

protected:
	//long OnSize(unsigned int, unsigned int, unsigned int) override;
	//long OnSysCommand(unsigned int) override;
	//LRESULT OnNotify(WPARAM, NMHDR*) override;

private:
	enum
	{
		MAXTABS = 4,
	};

	void SelectTab(unsigned int);
	void UnselectTab(unsigned int);
	void RefreshLayout();

	CRegViewPage* m_regView[MAXTABS];
	//CRegViewPage* m_current = nullptr;
	//Framework::Win32::CTab m_tabs;
	QTabWidget* m_tab;
};
