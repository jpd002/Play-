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

private:
	enum
	{
		MAXTABS = 4,
	};

	CRegViewPage* m_regView[MAXTABS] = {};
};
