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

private:
	std::vector<BootablesDb::Bootable>& m_bootables;
};

class BootableCoverQVarient
{

public:
	explicit BootableCoverQVarient(std::string = "PH", std::string = "");
	~BootableCoverQVarient() = default;

	void paint(QPainter* painter, const QRect& rect, const QPalette& palette, int mode) const;
	QSize sizeHint() const;

private:
	std::string m_key;
	std::string m_title;
};

Q_DECLARE_METATYPE(BootableCoverQVarient)

class BootImageItemDelegate : public QStyledItemDelegate
{
	Q_OBJECT

public:
	BootImageItemDelegate(QWidget* parent = 0)
	    : QStyledItemDelegate(parent)
	{
	}

	void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
	QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;
};
