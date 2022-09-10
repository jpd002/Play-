#pragma once

#include <QWidget>
#include <QTableView>
#include "QtGenericTableModel.h"

#include "VirtualMachine.h"
#include "MIPS.h"
#include "BiosDebugInfoProvider.h"
#include "Types.h"
#include "VirtualMachineStateView.h"

class CKernelObjectListView : public QTableView, public CVirtualMachineStateView
{
public:
	typedef Framework::CSignal<void(uint32)> OnGotoAddressSignal;
	typedef Framework::CSignal<void(const char*)> OnObjectTypeChangedSignal;

	CKernelObjectListView(QWidget*);
	virtual ~CKernelObjectListView() = default;

	void HandleMachineStateChange() override;

	void SetContext(CMIPS*, CBiosDebugInfoProvider*);
	void SetObjectType(uint32);

	OnGotoAddressSignal OnGotoAddress;
	OnObjectTypeChangedSignal OnObjectTypeChanged;

public slots:
	void tableDoubleClick(const QModelIndex&);

private:
	void Update();

	CMIPS* m_context = nullptr;
	CBiosDebugInfoProvider* m_biosDebugInfoProvider = nullptr;
	BiosDebugObjectInfoMap m_schema;
	QTableView* m_tableView = nullptr;
	CQtGenericTableModel* m_model = nullptr;
	uint32 m_objectType = BIOS_DEBUG_OBJECT_TYPE_NULL;
};
