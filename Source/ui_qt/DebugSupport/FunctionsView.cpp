#include <QHeaderView>

#include "FunctionsView.h"
#include "string_cast.h"
#include "string_format.h"

#define DEFAULT_GROUPID (1)
#define DEFAULT_GROUPNAME _T("Global")

// #define SCALE(x) MulDiv(x, ydpi, 96)

CFunctionsView::CFunctionsView(QMdiArea* parent)
    : QMdiSubWindow(parent)
{

	setMinimumHeight(700);
	setMinimumWidth(300);

	parent->addSubWindow(this)->setWindowTitle("Functions");

	m_tableView = new QTableView(this);
	setWidget(m_tableView);
	m_tableView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

	m_model = new CQtGenericTableModel(parent, {"Name", "Address"});
	m_tableView->setModel(m_model);
	m_tableView->horizontalHeader()->setStretchLastSection(true);
	m_tableView->resizeColumnsToContents();


	// m_pNew = new Framework::Win32::CButton(_T("New..."), m_hWnd, Framework::Win32::CRect(0, 0, 0, 0));
	// m_pRename = new Framework::Win32::CButton(_T("Rename..."), m_hWnd, Framework::Win32::CRect(0, 0, 0, 0));
	// m_pDelete = new Framework::Win32::CButton(_T("Delete"), m_hWnd, Framework::Win32::CRect(0, 0, 0, 0));
	// m_pImport = new Framework::Win32::CButton(_T("Load ELF symbols"), m_hWnd, Framework::Win32::CRect(0, 0, 0, 0));

	// Framework::FlatLayoutPtr pSubLayout0 = Framework::CHorizontalLayout::Create();
	// pSubLayout0->InsertObject(Framework::CLayoutStretch::Create());
	// pSubLayout0->InsertObject(Framework::Win32::CLayoutWindow::CreateButtonBehavior(SCALE(100), SCALE(23), m_pNew));
	// pSubLayout0->InsertObject(Framework::Win32::CLayoutWindow::CreateButtonBehavior(SCALE(100), SCALE(23), m_pRename));
	// pSubLayout0->InsertObject(Framework::Win32::CLayoutWindow::CreateButtonBehavior(SCALE(100), SCALE(23), m_pDelete));
	// pSubLayout0->InsertObject(Framework::Win32::CLayoutWindow::CreateButtonBehavior(SCALE(100), SCALE(23), m_pImport));

	// m_pLayout = Framework::CVerticalLayout::Create();
	// m_pLayout->InsertObject(Framework::Win32::CLayoutWindow::CreateCustomBehavior(1, 1, 1, 1, m_pList));
	// m_pLayout->InsertObject(pSubLayout0);

	// SetSize(SCALE(469), SCALE(612));

	// RefreshLayout();
}

void CFunctionsView::Refresh()
{
	RefreshList();
}

// LRESULT CFunctionsView::OnNotify(WPARAM wParam, NMHDR* pH)
// {
// 	if(pH->hwndFrom == m_pList->m_hWnd)
// 	{
// 		NMLISTVIEW* pN = reinterpret_cast<NMLISTVIEW*>(pH);
// 		switch(pN->hdr.code)
// 		{
// 		case NM_DBLCLK:
// 			OnListDblClick();
// 			break;
// 		}
// 	}

// 	return FALSE;
// }

// long CFunctionsView::OnCommand(unsigned short nID, unsigned short nCmd, HWND hSender)
// {
// 	if(hSender == m_pNew->m_hWnd)
// 	{
// 		OnNewClick();
// 	}
// 	if(hSender == m_pRename->m_hWnd)
// 	{
// 		OnRenameClick();
// 	}
// 	if(hSender == m_pDelete->m_hWnd)
// 	{
// 		OnDeleteClick();
// 	}
// 	if(hSender == m_pImport->m_hWnd)
// 	{
// 		OnImportClick();
// 	}
// 	return TRUE;
// }


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

// void CFunctionsView::OnListDblClick()
// {
// 	int nItem = m_pList->GetSelection();
// 	if(nItem == -1) return;
// 	uint32 nAddress = (uint32)m_pList->GetItemData(nItem);
// 	OnFunctionDblClick(nAddress);
// }

// void CFunctionsView::OnNewClick()
// {
// 	if(!m_context) return;

// 	TCHAR sNameX[256];
// 	uint32 nAddress = 0;

// 	{
// 		Framework::Win32::CInputBox inputBox(_T("New Function"), _T("New Function Name:"), _T(""));
// 		const TCHAR* sValue = inputBox.GetValue(m_hWnd);
// 		if(sValue == NULL) return;
// 		_tcsncpy(sNameX, sValue, 255);
// 	}

// 	{
// 		Framework::Win32::CInputBox inputBox(_T("New Function"), _T("New Function Address:"), _T("00000000"));
// 		const TCHAR* sValue = inputBox.GetValue(m_hWnd);
// 		if(sValue == NULL) return;
// 		if(sValue != NULL)
// 		{
// 			_stscanf(sValue, _T("%x"), &nAddress);
// 			if((nAddress & 0x3) != 0x0)
// 			{
// 				MessageBox(m_hWnd, _T("Invalid address."), NULL, 16);
// 				return;
// 			}
// 		}
// 	}

// 	m_context->m_Functions.InsertTag(nAddress, string_cast<std::string>(sNameX).c_str());

// 	RefreshList();
// 	OnFunctionsStateChange();
// }

// void CFunctionsView::OnRenameClick()
// {
// 	if(!m_context) return;

// 	int nItem = m_pList->GetSelection();
// 	if(nItem == -1) return;

// 	uint32 nAddress = m_pList->GetItemData(nItem);
// 	const char* sName = m_context->m_Functions.Find(nAddress);

// 	if(sName == NULL)
// 	{
// 		//WTF?
// 		return;
// 	}

// 	Framework::Win32::CInputBox RenameInput(_T("Rename Function"), _T("New Function Name:"), string_cast<std::tstring>(sName).c_str());
// 	const TCHAR* sNewNameX = RenameInput.GetValue(m_hWnd);

// 	if(sNewNameX == NULL) return;

// 	m_context->m_Functions.InsertTag(nAddress, string_cast<std::string>(sNewNameX).c_str());
// 	RefreshList();

// 	OnFunctionsStateChange();
// }

// void CFunctionsView::OnImportClick()
// {
// 	if(!m_context) return;

// 	for(auto moduleIterator(std::begin(m_modules));
// 	    std::end(m_modules) != moduleIterator; moduleIterator++)
// 	{
// 		const auto& module(*moduleIterator);
// 		CELF* moduleImage = reinterpret_cast<CELF*>(module.param);

// 		if(moduleImage == NULL) continue;

// 		ELFSECTIONHEADER* pSymTab = moduleImage->FindSection(".symtab");
// 		if(pSymTab == NULL) continue;

// 		const char* pStrTab = (const char*)moduleImage->GetSectionData(pSymTab->nIndex);
// 		if(pStrTab == NULL) continue;

// 		ELFSYMBOL* pSym = (ELFSYMBOL*)moduleImage->FindSectionData(".symtab");
// 		unsigned int nCount = pSymTab->nSize / sizeof(ELFSYMBOL);

// 		for(unsigned int i = 0; i < nCount; i++)
// 		{
// 			if((pSym[i].nInfo & 0x0F) != 0x02) continue;
// 			ELFSECTIONHEADER* symbolSection = moduleImage->GetSection(pSym[i].nSectionIndex);
// 			if(symbolSection == NULL) continue;
// 			m_context->m_Functions.InsertTag(module.begin + (pSym[i].nValue - symbolSection->nStart), pStrTab + pSym[i].nName);
// 		}
// 	}

// 	RefreshList();

// 	OnFunctionsStateChange();
// }

// void CFunctionsView::OnDeleteClick()
// {
// 	if(!m_context) return;

// 	int nItem = m_pList->GetSelection();
// 	if(nItem == -1) return;
// 	if(MessageBox(m_hWnd, _T("Delete this function?"), NULL, MB_ICONQUESTION | MB_YESNO) != IDYES)
// 	{
// 		return;
// 	}

// 	uint32 nAddress = m_pList->GetItemData(nItem);
// 	m_context->m_Functions.InsertTag(nAddress, NULL);
// 	RefreshList();

// 	OnFunctionsStateChange();
// }
