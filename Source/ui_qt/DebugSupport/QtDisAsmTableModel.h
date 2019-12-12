#pragma once

#include <QAbstractTableModel>
#include <QPixmap>

#include "MIPS.h"
#include "VirtualMachineStateView.h"

class CQtDisAsmTableModel : public QAbstractTableModel
{
	Q_OBJECT
public:
	enum DISASM_TYPE
	{
		DISASM_STANDARD,
		DISASM_VU
	};
	CQtDisAsmTableModel(QObject* parent, CVirtualMachine&, CMIPS*);
	~CQtDisAsmTableModel();

	int rowCount(const QModelIndex& parent = QModelIndex()) const Q_DECL_OVERRIDE;
	int columnCount(const QModelIndex& parent = QModelIndex()) const Q_DECL_OVERRIDE;
	QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const Q_DECL_OVERRIDE;
	void DoubleClicked(const QModelIndex& index, QWidget* parent = nullptr);

	void Redraw();
	void Redraw(uint32);

	virtual std::string GetInstructionDetails(int, uint32) const;
	std::string GetInstructionMetadata(uint32) const;

protected:
	QVariant headerData(int section, Qt::Orientation orientation, int role) const Q_DECL_OVERRIDE;

	uint32 GetInstruction(uint32) const;
	CMIPS* m_ctx;
	CVirtualMachine& m_virtualMachine;
	uint32 m_instructionSize;
	DISASM_TYPE m_disAsmType;

	QVariantList m_headers;
	QPixmap start_line = QPixmap(25, 25);
	QPixmap end_line = QPixmap(25, 25);
	QPixmap line = QPixmap(25, 25);
	QPixmap arrow = QPixmap(25, 25);
	QPixmap breakpoint = QPixmap(25, 25);
	QPixmap breakpoint_arrow = QPixmap(25, 25);

};
