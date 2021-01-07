#pragma once

#include <QAbstractTableModel>
#include <QPixmap>
#include <QTableView>

#include "QtDisAsmTableModel.h"
#include "MIPS.h"
#include "VirtualMachineStateView.h"

class CQtDisAsmVuTableModel : public CQtDisAsmTableModel
{
	Q_OBJECT
public:
	CQtDisAsmVuTableModel(QTableView* parent, CVirtualMachine&, CMIPS*);
	~CQtDisAsmVuTableModel() = default;

	virtual std::string GetInstructionDetails(int, uint32) const override;
};
