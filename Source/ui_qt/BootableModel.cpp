#include <QPainter>
#include <QPixmap>

#include "ui_shared/BootablesProcesses.h"
#include "http/HttpClientFactory.h"

#include "BootableModel.h"
#include "CoverUtils.h"

BootableModel::BootableModel(QObject* parent, std::vector<BootablesDb::Bootable>& bootables)
    : QAbstractTableModel(parent)
    , m_bootables(bootables)
{
}

int BootableModel::rowCount(const QModelIndex& parent) const
{
	return m_bootables.size();
}

int BootableModel::columnCount(const QModelIndex& parent) const
{
	return 1;
}

QVariant BootableModel::data(const QModelIndex& index, int role) const
{
	if(role == Qt::DisplayRole)
	{
		int pos = index.row() + index.column();
		auto bootable = m_bootables.at(pos);
		return QVariant::fromValue(BootableCoverQVarient(bootable.discId));
	}
	return QVariant();
}

QSize BootableModel::SizeHint()
{
	static QSize size;
	if(size.isEmpty())
	{
		QPixmap pixmap = CoverUtils::find("PH");
		size = pixmap.size();
		size.setHeight(size.height() + 10);
		size.setWidth(size.width() + 10);
	}
	return size;
}

BootablesDb::Bootable BootableModel::GetBootable(const QModelIndex& index)
{
	unsigned int pos = index.row() + index.column();
	return m_bootables.at(pos);
}

void BootableModel::removeItem(const QModelIndex& index)
{
	unsigned int pos = index.row() + index.column();
	emit QAbstractTableModel::beginRemoveRows(QModelIndex(), pos, pos);
	m_bootables.erase(m_bootables.begin() + pos);
	emit QAbstractTableModel::endRemoveRows();
}

BootableCoverQVarient::BootableCoverQVarient(std::string key)
    : m_key(key)
{
}

void BootableCoverQVarient::paint(QPainter* painter, const QRect& rect, const QPalette& palette, int mode) const
{
	painter->save();

	painter->setRenderHint(QPainter::Antialiasing, true);
	painter->setPen(Qt::NoPen);

	QPixmap pixmap = CoverUtils::find(m_key.c_str());
	painter->drawPixmap(rect.x() + 5, rect.y() + 5, pixmap);

	painter->restore();
}

QSize BootableCoverQVarient::sizeHint() const
{
	static QSize size;
	if(size.isEmpty())
	{
		QPixmap pixmap = CoverUtils::find("PH");
		size = pixmap.size();
		size.setHeight(size.height() + 10);
		size.setWidth(size.width() + 10);
	}
	return size;
}

void BootImageItemDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
	if(index.data().canConvert<BootableCoverQVarient>())
	{
		BootableCoverQVarient bootablecover = qvariant_cast<BootableCoverQVarient>(index.data());

		if(option.state & QStyle::State_Selected)
			painter->fillRect(option.rect, option.palette.highlight());

		bootablecover.paint(painter, option.rect, option.palette, 0);
	}
	else
	{
		QStyledItemDelegate::paint(painter, option, index);
	}
}

QSize BootImageItemDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
{
	if(index.data().canConvert<BootableCoverQVarient>())
	{
		BootableCoverQVarient bootablecover = qvariant_cast<BootableCoverQVarient>(index.data());
		return bootablecover.sizeHint();
	}
	else
	{
		return QStyledItemDelegate::sizeHint(option, index);
	}
}
