#include "BootableModelProxy.h"
#include "BootableModel.h"
#include <regex>
BootableModelProxy::BootableModelProxy(QObject* parent)
    : QSortFilterProxyModel(parent)
{
}

bool BootableModelProxy::filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const
{
	QModelIndex index = sourceModel()->index(sourceRow, 0, sourceParent);

	QVariant data = sourceModel()->data(index);
	if(data.canConvert<BootableCoverQVarient>())
	{
		BootableCoverQVarient bootablecover = qvariant_cast<BootableCoverQVarient>(data);
		QString key = QString::fromStdString(bootablecover.GetKey());
		QString title = QString::fromStdString(bootablecover.GetTitle());
		QRegularExpression regex = filterRegularExpression();
		regex.setPatternOptions(QRegularExpression::CaseInsensitiveOption);

		return key.contains(regex) || title.contains(regex);
	}
	return false;
}
