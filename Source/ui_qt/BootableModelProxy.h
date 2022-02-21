#include <QSortFilterProxyModel>

class BootableModelProxy : public QSortFilterProxyModel
{
	Q_OBJECT

public:
	BootableModelProxy(QObject* parent);

	void setFilterState(const QString&);

protected:
	bool filterAcceptsRow(int, const QModelIndex&) const override;

private:
	std::string m_state;
};
