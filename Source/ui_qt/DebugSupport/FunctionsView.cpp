#include <QHeaderView>
#include <QGridLayout>
#include <QInputDialog>
#include <QMessageBox>
#include <QPushButton>

#include "FunctionsView.h"
#include "string_cast.h"
#include "string_format.h"
#include "lexical_cast_ex.h"

#define DEFAULT_GROUPID (1)
#define DEFAULT_GROUPNAME ("Global")

#define addressValueColumn (0)
#define addressValueRole (Qt::UserRole)

CFunctionsView::CFunctionsView(QMdiArea* parent)
    : QMdiSubWindow(parent)
{

	resize(300, 700);

	parent->addSubWindow(this);
	setWindowTitle("Functions");

	m_treeWidget = new QTreeWidget(this);
	auto btnNew = new QPushButton("New...", this);
	auto btnRename = new QPushButton("Rename...", this);
	auto btnDelete = new QPushButton("Delete", this);
	auto btnImport = new QPushButton("Load ELF symbols", this);

	connect(m_treeWidget, &QTreeWidget::itemDoubleClicked, this, &CFunctionsView::OnListDblClick);
	connect(btnNew, &QPushButton::clicked, this, &CFunctionsView::OnNewClick);
	connect(btnRename, &QPushButton::clicked, this, &CFunctionsView::OnRenameClick);
	connect(btnDelete, &QPushButton::clicked, this, &CFunctionsView::OnDeleteClick);
	connect(btnImport, &QPushButton::clicked, this, &CFunctionsView::OnImportClick);

	auto widget = new QWidget(this);
	auto layout = new QGridLayout(widget);
	layout->addWidget(m_treeWidget, 0, 0, 1, 4);
	layout->addWidget(btnNew, 1, 0, 1, 1);
	layout->addWidget(btnRename, 1, 1, 1, 1);
	layout->addWidget(btnDelete, 1, 2, 1, 1);
	layout->addWidget(btnImport, 1, 3, 1, 1);
	widget->setLayout(layout);
	setWidget(widget);

	m_treeWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
	QStringList headers = {"Name", "Address"};
	m_treeWidget->setColumnCount(headers.count());
	m_treeWidget->setHeaderLabels(headers);
	m_treeWidget->setSortingEnabled(false);
	m_treeWidget->header()->setVisible(true);
}

void CFunctionsView::showEvent(QShowEvent* evt)
{
	QMdiSubWindow::showEvent(evt);
	widget()->show();
}

void CFunctionsView::Refresh()
{
	RefreshList();
}

void CFunctionsView::RefreshList()
{
	m_groupMap.clear();
	m_treeWidget->clear();

	if(!m_context) return;
	if(m_biosDebugInfoProvider)
	{
		m_modules = m_biosDebugInfoProvider->GetModulesDebugInfo();
	}
	else
	{
		m_modules.clear();
	}
	bool groupingEnabled = m_modules.size() != 0;

	if(groupingEnabled)
	{
		InitializeModuleGrouper();
	}

	for(auto itTag(m_context->m_Functions.GetTagsBegin());
	    itTag != m_context->m_Functions.GetTagsEnd(); itTag++)
	{
		std::string sTag(itTag->second);
		QTreeWidgetItem* childItem = new QTreeWidgetItem();
		childItem->setText(0, sTag.c_str());
		childItem->setText(1, string_format("0x%08X", itTag->first).c_str());
		childItem->setData(addressValueColumn, addressValueRole, itTag->first);
		if(groupingEnabled)
		{
			GetFunctionGroup(itTag->first)->addChild(childItem);
		}
		else
		{
			m_treeWidget->addTopLevelItem(childItem);
		}
	}
}

void CFunctionsView::InitializeModuleGrouper()
{
	QTreeWidgetItem* rootItem = new QTreeWidgetItem(m_treeWidget);
	rootItem->setText(0, DEFAULT_GROUPNAME);
	rootItem->setText(1, "");
	m_treeWidget->addTopLevelItem(rootItem);

	for(const auto& module : m_modules)
	{
		QTreeWidgetItem* rootItem = new QTreeWidgetItem(m_treeWidget);
		rootItem->setText(0, module.name.c_str());
		rootItem->setText(1, string_format("0x%08X -- 0x%08X", module.begin, module.end).c_str());
		m_groupMap.emplace(module.begin, rootItem);
		m_treeWidget->addTopLevelItem(rootItem);
	}
}

QTreeWidgetItem* CFunctionsView::GetFunctionGroup(uint32 address)
{
	for(const auto& module : m_modules)
	{
		if(address >= module.begin && address < module.end) return m_groupMap[module.begin];
	}
	return m_treeWidget->topLevelItem(0);
}

void CFunctionsView::SetContext(CMIPS* context, CBiosDebugInfoProvider* biosDebugInfoProvider)
{
	m_context = context;
	m_biosDebugInfoProvider = biosDebugInfoProvider;

	m_functionTagsChangeConnection = m_context->m_Functions.OnTagListChange.Connect(
	    std::bind(&CFunctionsView::RefreshList, this));
	RefreshList();
}

void CFunctionsView::OnListDblClick(QTreeWidgetItem* item, int column)
{
	uint32 nAddress = item->data(addressValueColumn, addressValueRole).toUInt();
	if(nAddress)
	{
		OnFunctionDblClick(nAddress);
	}
}

void CFunctionsView::OnNewClick()
{
	if(!m_context) return;

	std::string name;
	uint32 nAddress = 0;

	{
		bool ok;
		QString res = QInputDialog::getText(this, tr("New Function"),
		                                    tr("New Function Name:"), QLineEdit::Normal,
		                                    tr(""), &ok);
		if(!ok || res.isEmpty())
			return;

		name = res.toStdString();
	}

	{
		bool ok;
		QString res = QInputDialog::getText(this, tr("New Function"),
		                                    tr("New Function Address:"), QLineEdit::Normal,
		                                    tr("00000000"), &ok);
		if(!ok || res.isEmpty())
			return;

		if(sscanf(res.toStdString().c_str(), "%x", &nAddress) <= 0 || (nAddress & 0x3) != 0x0)
		{
			QMessageBox msgBox;
			msgBox.setText("Invalid address.");
			msgBox.exec();
			return;
		}
	}

	m_context->m_Functions.InsertTag(nAddress, name.c_str());

	RefreshList();
	OnFunctionsStateChange();
}

void CFunctionsView::OnRenameClick()
{
	if(!m_context) return;

	auto items = m_treeWidget->selectedItems();
	if(items.size() == 0 || items.first()->childCount() > 0)
		return;

	auto selectedAddressStr = items.first()->text(1).toStdString();
	uint32 nAddress = lexical_cast_hex(selectedAddressStr);
	const char* sName = m_context->m_Functions.Find(nAddress);

	if(sName == NULL)
	{
		//WTF?
		return;
	}

	bool ok;
	QString res = QInputDialog::getText(this, tr("Rename Function"),
	                                    tr("New Function Name:"), QLineEdit::Normal,
	                                    tr(""), &ok);
	if(!ok || res.isEmpty())
		return;

	m_context->m_Functions.InsertTag(nAddress, res.toStdString().c_str());
	RefreshList();

	OnFunctionsStateChange();
}

void CFunctionsView::OnDeleteClick()
{
	if(!m_context) return;

	auto items = m_treeWidget->selectedItems();
	if(items.size() == 0 || items.first()->childCount() > 0)
		return;

	auto selectedAddressStr = items.first()->text(1).toStdString();
	uint32 nAddress = lexical_cast_hex(selectedAddressStr);

	int ret = QMessageBox::warning(this, tr("Delete this function?"),
	                               tr("Are you sure you want to delete this function?"),
	                               QMessageBox::Ok | QMessageBox::Cancel, QMessageBox::Ok);
	if(ret != QMessageBox::Ok)
	{
		return;
	}

	m_context->m_Functions.InsertTag(nAddress, NULL);
	RefreshList();

	OnFunctionsStateChange();
}

void CFunctionsView::OnImportClick()
{
	if(!m_context) return;

	for(auto moduleIterator(std::begin(m_modules));
	    std::end(m_modules) != moduleIterator; moduleIterator++)
	{
		const auto& module(*moduleIterator);
		CELF* moduleImage = reinterpret_cast<CELF*>(module.param);

		if(moduleImage == NULL) continue;

		auto pSymTab = moduleImage->FindSection(".symtab");
		if(pSymTab == NULL) continue;

		const char* pStrTab = (const char*)moduleImage->GetSectionData(pSymTab->nIndex);
		if(pStrTab == NULL) continue;

		auto pSym = reinterpret_cast<const CELF::ELFSYMBOL*>(moduleImage->FindSectionData(".symtab"));
		unsigned int nCount = pSymTab->nSize / sizeof(CELF::ELFSYMBOL);

		for(unsigned int i = 0; i < nCount; i++)
		{
			if((pSym[i].nInfo & 0x0F) != 0x02) continue;
			auto symbolSection = moduleImage->GetSection(pSym[i].nSectionIndex);
			if(symbolSection == NULL) continue;
			m_context->m_Functions.InsertTag(module.begin + (pSym[i].nValue - symbolSection->nStart), pStrTab + pSym[i].nName);
		}
	}

	RefreshList();

	OnFunctionsStateChange();
}
