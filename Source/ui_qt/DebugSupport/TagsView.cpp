#include "TagsView.h"
#include "ui_TagsView.h"
#include "string_cast.h"
#include "string_format.h"
#include "lexical_cast_ex.h"
#include <QMessageBox>
#include <QInputDialog>

#define DEFAULT_GROUPID (1)
#define DEFAULT_GROUPNAME ("Global")

#define addressValueColumn (0)
#define addressValueRole (Qt::UserRole)

CTagsView::CTagsView(QMdiArea* parent)
    : QMdiSubWindow(parent)
    , ui(new Ui::CTagsView)
{
	ui->setupUi(this);

	parent->addSubWindow(this);

	setWidget(ui->widget);

	connect(this, &CTagsView::OnTagListChange, this, &CTagsView::RefreshList);

	connect(ui->treeWidget, &QTreeWidget::itemDoubleClicked, this, &CTagsView::OnListDblClick);
	connect(ui->btnNew, &QPushButton::clicked, this, &CTagsView::OnNewClick);
	connect(ui->btnRename, &QPushButton::clicked, this, &CTagsView::OnRenameClick);
	connect(ui->btnDelete, &QPushButton::clicked, this, &CTagsView::OnDeleteClick);
}

void CTagsView::showEvent(QShowEvent* evt)
{
	QMdiSubWindow::showEvent(evt);
	widget()->show();
}

void CTagsView::Refresh()
{
	RefreshList();
}

void CTagsView::RefreshList()
{
	m_groupMap.clear();
	ui->treeWidget->clear();

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

	for(auto itTag(m_tags->GetTagsBegin());
	    itTag != m_tags->GetTagsEnd(); itTag++)
	{
		std::string sTag(itTag->second);
		if(!m_filter.empty() && (sTag.find(m_filter) == std::string::npos))
		{
			continue;
		}

		QTreeWidgetItem* childItem = new QTreeWidgetItem();
		childItem->setText(0, sTag.c_str());
		childItem->setText(1, string_format("0x%08X", itTag->first).c_str());
		childItem->setData(addressValueColumn, addressValueRole, itTag->first);
		if(groupingEnabled)
		{
			GetTagGroup(itTag->first)->addChild(childItem);
		}
		else
		{
			ui->treeWidget->addTopLevelItem(childItem);
		}
	}

	if(!m_filter.empty())
	{
		ui->treeWidget->expandAll();
	}
}

void CTagsView::InitializeModuleGrouper()
{
	QTreeWidgetItem* rootItem = new QTreeWidgetItem(ui->treeWidget);
	rootItem->setText(0, DEFAULT_GROUPNAME);
	rootItem->setText(1, "");
	ui->treeWidget->addTopLevelItem(rootItem);

	for(const auto& module : m_modules)
	{
		QTreeWidgetItem* rootItem = new QTreeWidgetItem(ui->treeWidget);
		rootItem->setText(0, module.name.c_str());
		rootItem->setText(1, string_format("0x%08X -- 0x%08X", module.begin, module.end).c_str());
		m_groupMap.emplace(module.begin, rootItem);
		ui->treeWidget->addTopLevelItem(rootItem);
	}
}

QTreeWidgetItem* CTagsView::GetTagGroup(uint32 address)
{
	for(const auto& module : m_modules)
	{
		if(address >= module.begin && address < module.end) return m_groupMap[module.begin];
	}
	return ui->treeWidget->topLevelItem(0);
}

void CTagsView::SetStrings(const Strings& strings)
{
	m_strings = strings;
}

void CTagsView::SetContext(CMIPS* context, CMIPSTags* tags, CBiosDebugInfoProvider* biosDebugInfoProvider)
{
	m_context = context;
	m_biosDebugInfoProvider = biosDebugInfoProvider;
	m_tags = tags;
	m_tagsChangeConnection = m_tags->OnTagListChange.Connect(std::bind(&CTagsView::OnTagListChange, this));

	ui->filterEdit->setText(QString());

	OnTagListChange();
}

void CTagsView::OnListDblClick(QTreeWidgetItem* item, int column)
{
	uint32 nAddress = item->data(addressValueColumn, addressValueRole).toUInt();
	if(nAddress)
	{
		OnItemDblClick(nAddress);
	}
}

void CTagsView::OnNewClick()
{
	if(!m_context || !m_tags) return;

	std::string name;
	uint32 nAddress = 0;

	{
		bool ok;
		QString res = QInputDialog::getText(this, m_strings.newTagString,
		                                    m_strings.tagNameString, QLineEdit::Normal,
		                                    tr(""), &ok);
		if(!ok || res.isEmpty())
			return;

		name = res.toStdString();
	}

	{
		bool ok;
		QString res = QInputDialog::getText(this, m_strings.newTagString,
		                                    m_strings.tagAddressString, QLineEdit::Normal,
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

	m_tags->InsertTag(nAddress, name.c_str());

	RefreshList();
	OnStateChange();
}

void CTagsView::OnRenameClick()
{
	if(!m_context || !m_tags) return;

	auto items = ui->treeWidget->selectedItems();
	if(items.size() == 0 || items.first()->childCount() > 0)
		return;

	auto selectedAddressStr = items.first()->text(1).toStdString();
	uint32 nAddress = lexical_cast_hex(selectedAddressStr);
	const char* sName = m_tags->Find(nAddress);

	if(sName == NULL)
	{
		//WTF?
		return;
	}

	bool ok;
	QString res = QInputDialog::getText(this, m_strings.renameTagString,
	                                    m_strings.tagNameString, QLineEdit::Normal,
	                                    sName, &ok);
	if(!ok || res.isEmpty())
		return;

	m_tags->InsertTag(nAddress, res.toStdString().c_str());
	RefreshList();

	OnStateChange();
}

void CTagsView::OnDeleteClick()
{
	if(!m_context) return;

	auto items = ui->treeWidget->selectedItems();
	if(items.size() == 0)
		return;

	auto selectedItem = items.first();
	if(selectedItem->childCount() != 0)
	{
		auto selectedModuleName = selectedItem->text(0);
		if(selectedModuleName == DEFAULT_GROUPNAME)
		{
			return;
		}

		int ret = QMessageBox::warning(this, m_strings.deleteTagString,
		                               m_strings.deleteModuleTagsConfirmString.arg(selectedModuleName),
		                               QMessageBox::Ok | QMessageBox::Cancel, QMessageBox::Ok);
		if(ret != QMessageBox::Ok)
		{
			return;
		}

		std::vector<uint32> toDelete;
		for(auto tagIterator = m_tags->GetTagsBegin();
		    tagIterator != m_tags->GetTagsEnd(); tagIterator++)
		{
			auto tagGroupItem = GetTagGroup(tagIterator->first);
			if(tagGroupItem == selectedItem)
			{
				toDelete.push_back(tagIterator->first);
			}
		}

		for(auto address : toDelete)
		{
			m_tags->InsertTag(address, nullptr);
		}
	}
	else
	{
		auto selectedAddressStr = selectedItem->text(1).toStdString();
		uint32 nAddress = lexical_cast_hex(selectedAddressStr);

		int ret = QMessageBox::warning(this, m_strings.deleteTagString,
		                               m_strings.deleteTagConfirmString,
		                               QMessageBox::Ok | QMessageBox::Cancel, QMessageBox::Ok);
		if(ret != QMessageBox::Ok)
		{
			return;
		}

		m_tags->InsertTag(nAddress, nullptr);
	}

	RefreshList();

	OnStateChange();
}

void CTagsView::on_filterEdit_textChanged()
{
	m_filter = ui->filterEdit->text().toStdString();
	RefreshList();
}
