#pragma once

#include <QAbstractTableModel>
#include <QPixmap>

#include "QtDisAsmTableModel.h"
#include "MIPS.h"
#include "VirtualMachineStateView.h"

class CQtDisAsmVuTableModel : public CQtDisAsmTableModel
{
	Q_OBJECT
public:
	CQtDisAsmVuTableModel(QObject* parent, CVirtualMachine&, CMIPS*, int);
	~CQtDisAsmVuTableModel();

	virtual std::string GetInstructionDetails(int, uint32) const override;
};
