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
#define DEFAULT_GROUPNAME _T("Global")

CFunctionsView::CFunctionsView(QMdiArea* parent)
    : QMdiSubWindow(parent)
{

	resize(300, 700);

	parent->addSubWindow(this)->setWindowTitle("Functions");

	m_tableView = new QTableView(this);
	auto btnNew = new QPushButton("New...", this);
	auto btnRename = new QPushButton("Rename...", this);
	auto btnDelete = new QPushButton("Delete", this);
	auto btnImport = new QPushButton("Load ELF symbols", this);

	connect(m_tableView, &QTableView::doubleClicked, this, &CFunctionsView::OnListDblClick);
	connect(btnNew, &QPushButton::clicked, this, &CFunctionsView::OnNewClick);
	connect(btnRename, &QPushButton::clicked, this, &CFunctionsView::OnRenameClick);
	connect(btnDelete, &QPushButton::clicked, this, &CFunctionsView::OnDeleteClick);
	connect(btnImport, &QPushButton::clicked, this, &CFunctionsView::OnImportClick);

	auto widget = new QWidget();
	auto layout = new QGridLayout(widget);
	layout->addWidget(m_tableView, 0, 0, 1, 4);
	layout->addWidget(btnNew, 1, 0, 1, 1);
	layout->addWidget(btnRename, 1, 1, 1, 1);
	layout->addWidget(btnDelete, 1, 2, 1, 1);
	layout->addWidget(btnImport, 1, 3, 1, 1);
	widget->setLayout(layout);
	setWidget(widget);

	m_model = new CQtGenericTableModel(parent, {"Name", "Address"});
	m_tableView->setModel(m_model);
	auto header = m_tableView->horizontalHeader();
	header->setSectionResizeMode(0, QHeaderView::Stretch);
	header->setSectionResizeMode(1, QHeaderView::ResizeToContents);
}

void CFunctionsView::Refresh()
{
	RefreshList();
}

void CFunctionsView::RefreshList()
{
	m_model->clear();

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

	// if(groupingEnabled)
	// {
	// 	InitializeModuleGrouper();
	// }
	// else
	// {
	// 	m_pList->EnableGroupView(false);
	// }

	for(auto itTag(m_context->m_Functions.GetTagsBegin());
	    itTag != m_context->m_Functions.GetTagsEnd(); itTag++)
	{
		std::string sTag(itTag->second);
		m_model->addItem({sTag, string_format("0x%08X", itTag->first)});
	}
}

void CFunctionsView::InitializeModuleGrouper()
{
	// m_pList->RemoveAllGroups();
	// m_pList->EnableGroupView(true);
	// m_pList->InsertGroup(DEFAULT_GROUPNAME, DEFAULT_GROUPID);
	// for(const auto& module : m_modules)
	// {
	// 	m_pList->InsertGroup(
	// 	    string_cast<std::tstring>(module.name.c_str()).c_str(),
	// 	    module.begin);
	// }
}

uint32 CFunctionsView::GetFunctionGroupId(uint32 address)
{
	for(const auto& module : m_modules)
	{
		if(address >= module.begin && address < module.end) return module.begin;
	}
	return DEFAULT_GROUPID;
}

void CFunctionsView::SetContext(CMIPS* context, CBiosDebugInfoProvider* biosDebugInfoProvider)
{
	m_context = context;
	m_biosDebugInfoProvider = biosDebugInfoProvider;

	m_functionTagsChangeConnection = m_context->m_Functions.OnTagListChange.Connect(
	    std::bind(&CFunctionsView::RefreshList, this));
	RefreshList();
}

void CFunctionsView::OnListDblClick(const QModelIndex& indexRow)
{
	auto index = m_model->index(indexRow.row(), 1);
	auto selectedAddressStr = m_model->getItem(index);
	uint32 nAddress = lexical_cast_hex(selectedAddressStr);
	OnFunctionDblClick(nAddress);
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

	auto row = m_tableView->currentIndex().row();
	if(row < 0)
		return;

	auto index = m_model->index(row, 1);
	auto selectedAddressStr = m_model->getItem(index);
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

	auto row = m_tableView->currentIndex().row();
	if(row < 0)
		return;

	auto index = m_model->index(row, 1);
	auto selectedAddressStr = m_model->getItem(index);
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

		ELFSECTIONHEADER* pSymTab = moduleImage->FindSection(".symtab");
		if(pSymTab == NULL) continue;

		const char* pStrTab = (const char*)moduleImage->GetSectionData(pSymTab->nIndex);
		if(pStrTab == NULL) continue;

		ELFSYMBOL* pSym = (ELFSYMBOL*)moduleImage->FindSectionData(".symtab");
		unsigned int nCount = pSymTab->nSize / sizeof(ELFSYMBOL);

		for(unsigned int i = 0; i < nCount; i++)
		{
			if((pSym[i].nInfo & 0x0F) != 0x02) continue;
			ELFSECTIONHEADER* symbolSection = moduleImage->GetSection(pSym[i].nSectionIndex);
			if(symbolSection == NULL) continue;
			m_context->m_Functions.InsertTag(module.begin + (pSym[i].nValue - symbolSection->nStart), pStrTab + pSym[i].nName);
		}
	}

	RefreshList();

	OnFunctionsStateChange();
}
