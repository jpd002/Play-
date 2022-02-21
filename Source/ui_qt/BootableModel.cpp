#include <QPainter>
#include <QPixmap>

#include "ui_shared/BootablesProcesses.h"
#include "http/HttpClientFactory.h"

#include "BootableModel.h"
#include "CoverUtils.h"

int g_viewwidth;

BootableModel::BootableModel(QObject* parent, std::vector<BootablesDb::Bootable>& bootables)
    : QAbstractTableModel(parent)
    , m_bootables(bootables)
{
}

int BootableModel::rowCount(const QModelIndex&) const
{
	return static_cast<int>(m_bootables.size());
}

int BootableModel::columnCount(const QModelIndex&) const
{
	return 1;
}

QVariant BootableModel::data(const QModelIndex& index, int role) const
{
	if(role == Qt::DisplayRole)
	{
		int pos = index.row() + index.column();
		auto bootable = m_bootables.at(static_cast<unsigned int>(pos));
		return QVariant::fromValue(BootableCoverQVariant(bootable.discId, bootable.title, bootable.path.string(), bootable.states));
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

void BootableModel::SetWidth(int width)
{
	g_viewwidth = width;
}

/* start of BootImageItemDelegate */
BootableCoverQVariant::BootableCoverQVariant(std::string key, std::string title, std::string path, BootablesDb::BootableStateList states)
    : m_key(key)
    , m_title(title)
    , m_path(path)
    , m_states(states)
{
	for(auto state : states)
	{
		if(state.name.find("state") != std::string::npos)
		{
			m_statusColor = "#" + state.color;
		}
	}
}

void BootableCoverQVariant::paint(QPainter* painter, const QRect& rect, const QPalette&, int) const
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

	if(!m_statusColor.empty())
	{
		QPainter painter(&pixmap);
		auto radius = pixmap.height() - (pixmap.height() * 92.5 / 100);
		QRect pix_rect = QRect(pixmap.width() - radius - 5, pixmap.height() - radius - 5, radius, radius);

		QColor color(m_statusColor.c_str());
		Qt::BrushStyle style = Qt::SolidPattern;
		QBrush brush(color, style);
		painter.setBrush(brush);
		painter.drawEllipse(pix_rect);
	}

	painter->drawPixmap(rect.x() + 5 + (GetPadding() / 2), rect.y() + 5, pixmap);

	painter->restore();
}

int BootableCoverQVariant::GetPadding() const
{
	QPixmap pixmap = CoverUtils::find("PH");
	int cover_width = pixmap.size().width() + 10;

	int cover_count = (g_viewwidth / cover_width);
	int reminder = (g_viewwidth % cover_width);
	if(reminder > 0 && cover_count > 0)
		return reminder / cover_count;

	return 0;
}

QSize BootableCoverQVariant::sizeHint() const
{
	QSize size;
	if(size.isEmpty())
	{
		QPixmap pixmap = CoverUtils::find("PH");
		size = pixmap.size();
		size.setHeight(size.height() + 10);
		size.setWidth(size.width() + 10);

		size.rwidth() += GetPadding();
	}
	return size;
}

std::string BootableCoverQVariant::GetKey()
{
	return m_key;
}

std::string BootableCoverQVariant::GetTitle()
{
	return m_title;
}

std::string BootableCoverQVariant::GetPath()
{
	return m_path;
}

bool BootableCoverQVariant::HasState(std::string state)
{
	auto itr = std::find_if(std::begin(m_states), std::end(m_states), [state](BootablesDb::BootableState bootState) {
		return bootState.name == state;
	});
	return itr != std::end(m_states);
}

/* start of BootImageItemDelegate */
BootImageItemDelegate::BootImageItemDelegate(QWidget* parent)
    : QStyledItemDelegate(parent)
{
}

void BootImageItemDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
	if(index.data().canConvert<BootableCoverQVariant>())
	{
		BootableCoverQVariant bootablecover = qvariant_cast<BootableCoverQVariant>(index.data());

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
	if(index.data().canConvert<BootableCoverQVariant>())
	{
		BootableCoverQVariant bootablecover = qvariant_cast<BootableCoverQVariant>(index.data());
		return bootablecover.sizeHint();
	}
	else
	{
		return QStyledItemDelegate::sizeHint(option, index);
	}
}
