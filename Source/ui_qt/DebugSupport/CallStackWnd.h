#pragma once

#include "signal/Signal.h"
#include <QWidget>
#include <QListWidget>
#include "Types.h"
#include "MIPS.h"
#include "VirtualMachineStateView.h"

class CBiosDebugInfoProvider;

class CCallStackWnd : public QListWidget, public CVirtualMachineStateView
{
public:
	typedef Framework::CSignal<void(uint32)> OnFunctionDblClickSignal;

	CCallStackWnd(QWidget*, CMIPS*, CBiosDebugInfoProvider*);
	virtual ~CCallStackWnd() = default;

	void HandleMachineStateChange() override;

	OnFunctionDblClickSignal OnFunctionDblClick;

public slots:
	void listDoubleClick(QListWidgetItem* item);

private:
	void Update();

	CMIPS* m_context;
	CBiosDebugInfoProvider* m_biosDebugInfoProvider;
};
