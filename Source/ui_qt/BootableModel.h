#pragma once

#include <string>
#include <vector>
#include <QListView>
#include <QAbstractTableModel>
#include <QStyledItemDelegate>
#include "ui_shared/BootablesDbClient.h"

class BootableModel : public QAbstractTableModel
{
	Q_OBJECT

public:
	BootableModel(QObject* parent, std::vector<BootablesDb::Bootable>&);

	int rowCount(const QModelIndex& parent = QModelIndex()) const Q_DECL_OVERRIDE;
	int columnCount(const QModelIndex& parent = QModelIndex()) const Q_DECL_OVERRIDE;
	QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const Q_DECL_OVERRIDE;

	QSize SizeHint();
	BootablesDb::Bootable GetBootable(const QModelIndex&);
	void removeItem(const QModelIndex&);
	void SetWidth(int);

private:
	std::vector<BootablesDb::Bootable>& m_bootables;
};

class BootableCoverQVariant
{

public:
	explicit BootableCoverQVariant(std::string = "PH", std::string = "", std::string = "", BootablesDb::BootableStateList = {});
	~BootableCoverQVariant() = default;

	void paint(QPainter* painter, const QRect& rect, const QPalette& palette, int mode) const;
	QSize sizeHint() const;

	std::string GetKey();
	std::string GetTitle();
	std::string GetPath();
	bool HasState(std::string);

private:
	int GetPadding() const;
	std::string m_key;
	std::string m_title;
	std::string m_path;
	std::string m_statusColor;
	BootablesDb::BootableStateList m_states;
};

Q_DECLARE_METATYPE(BootableCoverQVariant)

class BootImageItemDelegate : public QStyledItemDelegate
{
	Q_OBJECT

public:
	BootImageItemDelegate(QWidget* parent = 0);

	void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
	QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;
};
