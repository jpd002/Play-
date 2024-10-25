#include "BootableModelProxy.h"
#include "BootableModel.h"
#include "QStringUtils.h"
#include <regex>

BootableModelProxy::BootableModelProxy(QObject* parent)
    : QSortFilterProxyModel(parent)
{
}

void BootableModelProxy::setFilterState(const QString& state)
{
	if(state == "All")
	{
		m_state.clear();
	}
	else
	{
		m_state = state.toStdString();
	}
	invalidateFilter();
}

void BootableModelProxy::setBootableTypeFilterState(int bitIndex, bool value)
{
	if(value)
		m_bootableType |= bitIndex;
	else
		m_bootableType &= ~bitIndex;

	invalidateFilter();
}

int BootableModelProxy::getBootableTypeFilterState()
{
	return m_bootableType;
}

bool BootableModelProxy::filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const
{
	QModelIndex index = sourceModel()->index(sourceRow, 0, sourceParent);

	QVariant data = sourceModel()->data(index);
	if(data.canConvert<BootableCoverQVariant>())
	{
		BootableCoverQVariant bootablecover = qvariant_cast<BootableCoverQVariant>(data);
		QString key = QString::fromStdString(bootablecover.GetKey());
		QString title = QString::fromStdString(bootablecover.GetTitle());
		QString path = PathToQString(bootablecover.GetPath());
		auto bootableType = bootablecover.GetBootableType();
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
		QRegularExpression regex = filterRegularExpression();
		regex.setPatternOptions(QRegularExpression::CaseInsensitiveOption);
#else
		// QRegExp is deprecated in Qt6, while QRegularExpression is recommended over QRegExp since Qt5
		// however QRegularExpression is producing incorrect results in Qt5
		QRegExp regex = filterRegExp();
		regex.setCaseSensitivity(Qt::CaseInsensitive);
#endif

		bool res = (key.contains(regex) || title.contains(regex) || path.contains(regex));
		if(!m_state.empty())
		{
			res &= bootablecover.HasState(m_state);
		}

		if(m_bootableType != 0)
		{
			res &= (bootableType & m_bootableType) != 0;
		}
		return res;
	}
	return false;
}
