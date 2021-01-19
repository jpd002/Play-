#include <QHeaderView>

#include "ui_AddressListViewWnd.h"
#include "AddressListViewWnd.h"
#include "string_format.h"
#include "string_cast.h"
#include "lexical_cast_ex.h"

CAddressListViewWnd::CAddressListViewWnd(QMdiArea* parent)
    : QMdiSubWindow(parent)
    , ui(new Ui::CAddressListViewWnd)
{
	ui->setupUi(this);
	setWidget(ui->addressListView);

	parent->addSubWindow(this);

	m_model = new CQtGenericTableModel(ui->addressListView, {"Address"});
	ui->addressListView->setModel(m_model);
	ui->addressListView->horizontalHeader()->setStretchLastSection(true);
	ui->addressListView->resizeColumnsToContents();

	connect(ui->addressListView, &QTableView::doubleClicked, this, &CAddressListViewWnd::tableDoubleClick);
}

CAddressListViewWnd::~CAddressListViewWnd()
{
	delete ui;
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

void CAddressListViewWnd::show()
{
	ui->addressListView->show();
	QMdiSubWindow::show();
}
