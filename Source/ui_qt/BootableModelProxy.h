#include <QSortFilterProxyModel>

class BootableModelProxy : public QSortFilterProxyModel
{
	Q_OBJECT

public:
	BootableModelProxy(QObject* parent);

protected:
	bool filterAcceptsRow(int, const QModelIndex&) const override;
};
