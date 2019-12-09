#include <QHeaderView>

#include "AddressListViewWnd.h"
#include "string_format.h"
#include "string_cast.h"
#include "lexical_cast_ex.h"

CAddressListViewWnd::CAddressListViewWnd(QMdiArea* parent)
    : QMdiSubWindow(parent)
{

	resize(400, 700);

	parent->addSubWindow(this);

	m_tableView = new QTableView(this);
	setWidget(m_tableView);
	m_tableView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

	m_model = new CQtGenericTableModel(parent, {"Address"});
	m_tableView->setModel(m_model);
	m_tableView->horizontalHeader()->setStretchLastSection(true);
	m_tableView->resizeColumnsToContents();

	connect(m_tableView, &QTableView::doubleClicked, this, &CAddressListViewWnd::tableDoubleClick);
}

void CAddressListViewWnd::SetTitle(std::string title)
{
	setWindowTitle(title.c_str());
}

void CAddressListViewWnd::SetAddressList(AddressList addressList)
{
	m_model->clear();
	for(const auto& address : addressList)
	{
		auto addressString = string_format("0x%08X", address);
		m_model->addItem({addressString});
	}
}

void CAddressListViewWnd::tableDoubleClick(const QModelIndex& index)
{
	auto selectedAddressStr = m_model->getItem(index);
	uint32 selectedAddress = lexical_cast_hex(selectedAddressStr);
	AddressSelected(selectedAddress);
}
