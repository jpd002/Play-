#pragma once

#include <QMdiArea>
#include <QMdiSubWindow>
#include <QTableView>
#include "QtGenericTableModel.h"

#include "signal/Signal.h"
#include "Types.h"

namespace Ui
{
	class CAddressListViewWnd;
}

class CAddressListViewWnd : public QMdiSubWindow
{
	Q_OBJECT

public:
	typedef std::vector<uint32> AddressList;

	typedef Framework::CSignal<void(uint32)> AddressSelectedEvent;

	CAddressListViewWnd(QMdiArea*);
	virtual ~CAddressListViewWnd();

	void SetTitle(std::string);
	void SetAddressList(AddressList);

	AddressSelectedEvent AddressSelected;

public slots:
	void show();
	void tableDoubleClick(const QModelIndex&);

private:
	Ui::CAddressListViewWnd* ui;

	CQtGenericTableModel* m_model;
};
