#pragma once

#include <QAbstractTableModel>

class CQtGenericTableModel : public QAbstractTableModel
{
	Q_OBJECT
public:
	CQtGenericTableModel(QObject* parent, std::vector<std::string>);
	~CQtGenericTableModel() = default;

	int rowCount(const QModelIndex& parent = QModelIndex()) const Q_DECL_OVERRIDE;
	int columnCount(const QModelIndex& parent = QModelIndex()) const Q_DECL_OVERRIDE;
	QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const Q_DECL_OVERRIDE;

	bool addItem(std::vector<std::string>);
	std::string getItem(const QModelIndex& index);
	void clear();

protected:
	QVariant headerData(int section, Qt::Orientation orientation, int role) const Q_DECL_OVERRIDE;

private:
	std::vector<std::vector<std::string>> m_data;
	QVariantList m_headers;
};
