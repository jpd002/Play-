#include "ELFView.h"
#include "string_format.h"
#include "lexical_cast_ex.h"

#include <QLabel>
#include <QFontMetrics>

CELFView::CELFView(QMdiArea* parent)
    : QMdiSubWindow(parent)
{

	resize(568, 457);
	parent->addSubWindow(this);
	setWindowTitle("ELF File Viewer");

	m_centralwidget = new QWidget(this);
	m_treeWidget = new QTreeWidget(m_centralwidget);
	m_layout = new QHBoxLayout(m_centralwidget);
	m_groupBox = new QGroupBox(m_centralwidget);

	m_layout->addWidget(m_treeWidget);
	m_layout->addWidget(m_groupBox);
	auto groupBoxLayout = new QHBoxLayout();
	m_groupBox->setLayout(groupBoxLayout);

	setWidget(m_centralwidget);

	m_treeWidget->setMaximumSize(QSize(200, 2000));
	QTreeWidgetItem* colHeader = m_treeWidget->headerItem();
	colHeader->setText(0, "ELF");

	m_pHeaderView = new CELFHeaderView(this, groupBoxLayout);
	m_pSymbolView = new CELFSymbolView(this, groupBoxLayout);
	m_pSectionView = new CELFSectionView(this, groupBoxLayout);
	m_pProgramView = new CELFProgramView(this, groupBoxLayout);

	connect(m_treeWidget, &QTreeWidget::itemSelectionChanged, this, &CELFView::itemSelectionChanged);
}

void CELFView::Reset()
{
	m_pHeaderView->Reset();
	m_pSymbolView->Reset();
	m_pSectionView->Reset();
	m_pProgramView->Reset();
}

void CELFView::SetELF(CELF* pELF)
{
	Reset();

	m_pELF = pELF;
	if(m_pELF == NULL) return;

	m_pHeaderView->SetELF(m_pELF);
	m_pSymbolView->SetELF(m_pELF);
	m_pSectionView->SetELF(m_pELF);
	m_pProgramView->SetELF(m_pELF);

	PopulateList();
}

void CELFView::resizeEvent(QResizeEvent* evt)
{
	QMdiSubWindow::resizeEvent(evt);
	m_pSectionView->ResizeEvent();
}

void CELFView::PopulateList()
{
	m_treeWidget->clear();

	QTreeWidgetItem* headRootItem = new QTreeWidgetItem(m_treeWidget, {"Header"});
	m_treeWidget->addTopLevelItem(headRootItem);

	QTreeWidgetItem* sectionsRootItem = new QTreeWidgetItem(m_treeWidget, {"Sections"});
	m_treeWidget->addTopLevelItem(sectionsRootItem);
	const auto& header = m_pELF->GetHeader();

	const char* sStrTab = (const char*)m_pELF->GetSectionData(header.nSectHeaderStringTableIndex);
	for(unsigned int i = 0; i < header.nSectHeaderCount; i++)
	{
		std::string sDisplay;
		const char* sName(NULL);

		auto pSect = m_pELF->GetSection(i);

		if(sStrTab != NULL)
		{
			sName = sStrTab + pSect->nStringTableIndex;
		}
		else
		{
			sName = "";
		}

		if(strlen(sName))
		{
			sDisplay = sName;
		}
		else
		{
			sDisplay = ("Section ") + lexical_cast_uint<std::string>(i);
		}
		sectionsRootItem->addChild(new QTreeWidgetItem(sectionsRootItem, {sDisplay.c_str()}));
	}
	sectionsRootItem->setExpanded(true);

	m_hasPrograms = header.nProgHeaderCount != 0;
	if(m_hasPrograms)
	{
		QTreeWidgetItem* segmentsRootItem = new QTreeWidgetItem(m_treeWidget, {"Segments"});
		m_treeWidget->addTopLevelItem(segmentsRootItem);

		for(unsigned int i = 0; i < header.nProgHeaderCount; i++)
		{
			std::string sDisplay(("Segment ") + lexical_cast_uint<std::string>(i));
			segmentsRootItem->addChild(new QTreeWidgetItem(segmentsRootItem, {sDisplay.c_str()}));
		}
		segmentsRootItem->setExpanded(true);
	}

	m_hasSymbols = m_pELF->FindSection(".strtab") && m_pELF->FindSection(".symtab");
	if(m_hasSymbols)
	{
		QTreeWidgetItem* symbolsRootItem = new QTreeWidgetItem(m_treeWidget, {"Symbols"});
		m_treeWidget->addTopLevelItem(symbolsRootItem);
	}
}

void CELFView::itemSelectionChanged()
{
	m_pHeaderView->hide();
	m_pSectionView->hide();
	m_pProgramView->hide();
	m_pSymbolView->hide();

	auto item = m_treeWidget->selectedItems().at(0);
	if(!item)
		return;

	bool isRoot = item->parent() == nullptr;
	int rootIndex = -1;
	auto index = -1;
	if(!isRoot)
	{
		index = item->parent()->indexOfChild(item);
		rootIndex = m_treeWidget->indexOfTopLevelItem(item->parent());
	}
	else
	{
		rootIndex = m_treeWidget->indexOfTopLevelItem(item);
	}
	if(rootIndex == 0)
	{
		m_pHeaderView->show();
	}
	else if(rootIndex == 1)
	{
		if(index > -1)
		{
			m_pSectionView->SetSection(index);
			m_pSectionView->show();
		}
	}
	else if(rootIndex != -1)
	{
		if(rootIndex == 2 && m_hasPrograms)
		{
			if(index > -1)
			{
				m_pProgramView->SetProgram(index);
				m_pProgramView->show();
			}
		}
		else
		{
			m_pSymbolView->show();
		}
	}
}
