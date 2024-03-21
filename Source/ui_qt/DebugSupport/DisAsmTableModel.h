#pragma once

#include <QAbstractTableModel>
#include <QPixmap>
#include <QTableView>
#include <QTextDocument>
#include <QStyledItemDelegate>
#include <QApplication>
#include <QPainter>
#include <QStyleOptionViewItem>
#include <QModelIndex>

#include "MIPS.h"
#include "VirtualMachineStateView.h"

class CDisAsmTableModel : public QAbstractTableModel
{
	Q_OBJECT
public:
	enum DISASM_TYPE
	{
		DISASM_STANDARD,
		DISASM_VU
	};

	CDisAsmTableModel(QTableView* parent, CVirtualMachine&, CMIPS*, uint64, uint32, DISASM_TYPE = DISASM_TYPE::DISASM_STANDARD);
	~CDisAsmTableModel() = default;

	int rowCount(const QModelIndex& parent = QModelIndex()) const Q_DECL_OVERRIDE;
	int columnCount(const QModelIndex& parent = QModelIndex()) const Q_DECL_OVERRIDE;
	QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const Q_DECL_OVERRIDE;

	void SetWindowCenter(uint32);

	void Redraw();
	void Redraw(uint32);

	uint32 TranslateAddress(uint32) const;
	uint32 TranslateModelIndexToAddress(const QModelIndex&) const;
	QModelIndex TranslateAddressToModelIndex(uint32) const;

	int GetLinePixMapWidth() const;

protected:
	QVariant headerData(int section, Qt::Orientation orientation, int role) const Q_DECL_OVERRIDE;

	void BuildIcons();

	uint32 GetInstruction(uint32) const;
	virtual std::string GetInstructionDetails(int, uint32) const;
	std::string GetInstructionMetadata(uint32) const;

	CMIPS* m_ctx;
	CVirtualMachine& m_virtualMachine;
	uint32 m_instructionSize;
	uint64 m_size = 0;
	uint32 m_windowStart = 0;
	uint32 m_windowSize = 0;
	DISASM_TYPE m_disAsmType;

	QVariantList m_headers;
	QPixmap m_start_line = QPixmap(22, 22);
	QPixmap m_end_line = QPixmap(22, 22);
	QPixmap m_line = QPixmap(22, 22);
	QPixmap m_arrow = QPixmap(22, 22);
	QPixmap m_breakpoint = QPixmap(22, 22);
	QPixmap m_breakpoint_arrow = QPixmap(22, 22);
};

class TableColumnDelegateTargetComment : public QStyledItemDelegate
{
	Q_OBJECT
public:
	explicit TableColumnDelegateTargetComment(QObject* = nullptr);

	void paint(QPainter*, const QStyleOptionViewItem&, const QModelIndex&) const Q_DECL_OVERRIDE;
};
