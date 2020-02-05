#pragma once

#include "VirtualMachineStateView.h"
#include <QWidget>
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

private:
	enum
	{
		MAXTABS = 4,
	};

	void SelectTab(unsigned int);
	void UnselectTab(unsigned int);
	void RefreshLayout();

	CRegViewPage* m_regView[MAXTABS];
};
