#pragma once

#include <QMdiArea>
#include <QMdiSubWindow>
#include <QTableView>
#include "QtGenericTableModel.h"

#include "signal/Signal.h"
#include "Types.h"

class CAddressListViewWnd : public QMdiSubWindow
{
public:
	typedef std::vector<uint32> AddressList;

	typedef Framework::CSignal<void(uint32)> AddressSelectedEvent;

	CAddressListViewWnd(QMdiArea*);
	virtual ~CAddressListViewWnd() = default;

	void SetTitle(std::string);
	void SetAddressList(AddressList);

	AddressSelectedEvent AddressSelected;

public slots:
	void tableDoubleClick(const QModelIndex&);


private:
	QTableView* m_tableView;
	CQtGenericTableModel* m_model;
};
