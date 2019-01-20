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
	return static_cast<int>(m_bootables.size());
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
		auto bootable =  m_bootables.at(static_cast<unsigned int>(pos));
		return QVariant::fromValue(BootableCoverQVarient(bootable.discId, bootable.title));
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
	int pos = index.row() + index.column();
	return m_bootables.at(static_cast<unsigned int>(pos));
}

void BootableModel::removeItem(const QModelIndex& index)
{
	int pos = index.row() + index.column();
	emit QAbstractTableModel::beginRemoveRows(QModelIndex(), pos, pos);
	m_bootables.erase(m_bootables.begin() + pos);
	emit QAbstractTableModel::endRemoveRows();
}

/* start of BootImageItemDelegate */
BootableCoverQVarient::BootableCoverQVarient(std::string key, std::string title)
    : m_key(key)
    , m_title(title)
{
}

void BootableCoverQVarient::paint(QPainter* painter, const QRect& rect, const QPalette& palette, int mode) const
{
	painter->save();

	painter->setRenderHint(QPainter::Antialiasing, true);
	painter->setPen(Qt::NoPen);

	QPixmap pixmap = CoverUtils::find(m_key.c_str());
	if(pixmap.isNull())
	{
		pixmap = CoverUtils::find("PH");
		QPainter painter(&pixmap);
		painter.setFont(QFont("Arial"));
		QTextOption opts(Qt::AlignCenter);
		opts.setWrapMode(QTextOption::WordWrap);
		QRect pix_rect = pixmap.rect();
		pix_rect.setTop(pixmap.height() * 70 / 100);
		std::string text;
		if(!m_key.empty())
		{
			text = m_key + "\n";
		}
		text += m_title;
		painter.drawText(pix_rect, text.c_str(), opts);
	}
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

/* start of BootImageItemDelegate */
BootImageItemDelegate::BootImageItemDelegate(QWidget* parent)
    : QStyledItemDelegate(parent)
{
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
