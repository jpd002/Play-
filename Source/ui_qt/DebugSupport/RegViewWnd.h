#pragma once

#include "VirtualMachineStateView.h"
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QTabWidget>

class CRegViewPage;
class CMIPS;

class CRegViewWnd : public QMdiSubWindow, public CVirtualMachineStateView
{
public:
	CRegViewWnd(QMdiArea*, CMIPS*);
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
	QTabWidget* m_tab;
	QTabWidget* m_tableWidget;
};
