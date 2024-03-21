#pragma once

#include <QAbstractTableModel>
#include <QPixmap>
#include <QTableView>

#include "DisAsmTableModel.h"
#include "MIPS.h"
#include "VirtualMachineStateView.h"

class CDisAsmVuTableModel : public CDisAsmTableModel
{
	Q_OBJECT
public:
	CDisAsmVuTableModel(QTableView* parent, CVirtualMachine&, CMIPS*, uint64, uint32);
	~CDisAsmVuTableModel() = default;

	virtual std::string GetInstructionDetails(int, uint32) const override;
};
