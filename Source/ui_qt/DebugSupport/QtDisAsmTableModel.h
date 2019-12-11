#pragma once

#include <QAbstractTableModel>
#include <QPixmap>

#include "MIPS.h"
#include "VirtualMachineStateView.h"

class CQtDisAsmTableModel : public QAbstractTableModel
{
	Q_OBJECT
public:
	CQtDisAsmTableModel(QObject* parent, CVirtualMachine&, CMIPS*, std::vector<std::string>);
	~CQtDisAsmTableModel();

	int rowCount(const QModelIndex& parent = QModelIndex()) const Q_DECL_OVERRIDE;
	int columnCount(const QModelIndex& parent = QModelIndex()) const Q_DECL_OVERRIDE;
	QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const Q_DECL_OVERRIDE;
	void DoubleClicked(const QModelIndex& index, QWidget* parent = nullptr);

	bool addItem(std::vector<std::string>);
	std::string getItem(const QModelIndex& index);
	void clear();

	void Redraw();
	void Redraw(uint32);
protected:
	QVariant headerData(int section, Qt::Orientation orientation, int role) const Q_DECL_OVERRIDE;

private:
	uint32 GetInstruction(uint32) const;
	CMIPS* m_ctx;
	CVirtualMachine& m_virtualMachine;
	uint32 m_instructionSize;

	std::vector<std::vector<std::string>> m_data;
	QVariantList m_headers;
	QPixmap start_line = QPixmap(25, 25);
	QPixmap end_line = QPixmap(25, 25);
	QPixmap line = QPixmap(25, 25);
	QPixmap arrow = QPixmap(25, 25);

};
