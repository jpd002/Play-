#pragma once

#include <QAbstractTableModel>
#include "Types.h"

class CInputBindingManager;

class CInputBindingModel : public QAbstractTableModel
{
	Q_OBJECT
public:
	CInputBindingModel(QObject* parent, CInputBindingManager*, uint32 padIndex);
	~CInputBindingModel() = default;

	int rowCount(const QModelIndex& parent = QModelIndex()) const Q_DECL_OVERRIDE;
	int columnCount(const QModelIndex& parent = QModelIndex()) const Q_DECL_OVERRIDE;
	QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const Q_DECL_OVERRIDE;
	bool setHeaderData(int section, Qt::Orientation orientation, const QVariant& value, int role = Qt::EditRole) Q_DECL_OVERRIDE;
	void Refresh();

protected:
	QVariant headerData(int section, Qt::Orientation orientation, int role) const Q_DECL_OVERRIDE;

private:
	CInputBindingManager* m_inputManager = nullptr;
	uint32 m_padIndex = 0;
	QVariantList m_h_header;
};
