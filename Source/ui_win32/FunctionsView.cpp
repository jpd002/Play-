#include <stdio.h>
#include "FunctionsView.h"
#include "layout/HorizontalLayout.h"
#include "layout/LayoutStretch.h"
#include "win32/LayoutWindow.h"
#include "win32/InputBox.h"
#include "string_cast.h"
#include "lexical_cast_ex.h"
#include "PtrMacro.h"

#define CLSNAME _T("FunctionsView")

#define DEFAULT_GROUPID		(1)
#define DEFAULT_GROUPNAME	_T("Global")

#define SCALE(x)	MulDiv(x, ydpi, 96)

CFunctionsView::CFunctionsView(HWND hParent) 
: m_context(nullptr)
, m_biosDebugInfoProvider(nullptr)
{
	if(!DoesWindowClassExist(CLSNAME))
	{
		WNDCLASSEX wc;
		memset(&wc, 0, sizeof(WNDCLASSEX));
		wc.cbSize			= sizeof(WNDCLASSEX);
		wc.hCursor			= LoadCursor(NULL, IDC_ARROW);
		wc.hbrBackground	= (HBRUSH)(COLOR_WINDOW); 
		wc.hInstance		= GetModuleHandle(NULL);
		wc.lpszClassName	= CLSNAME;
		wc.lpfnWndProc		= CWindow::WndProc;
		RegisterClassEx(&wc);
	}

	unsigned long windowStyle = WS_CLIPCHILDREN | WS_THICKFRAME | WS_CAPTION | WS_SYSMENU | WS_MAXIMIZEBOX;
#ifndef FUNCTIONSVIEW_STANDALONE
	windowStyle |= WS_CHILD;
#endif
	int ydpi = GetDeviceCaps(GetDC(NULL), LOGPIXELSY);
	Create(NULL, CLSNAME, _T("Functions"), windowStyle, Framework::Win32::CRect(0, 0, SCALE(320), SCALE(240)), hParent, NULL);
	SetClassPtr();

	m_pList = new Framework::Win32::CListView(m_hWnd, Framework::Win32::CRect(0, 0, 0, 0), LVS_REPORT | LVS_SINGLESEL | LVS_SORTASCENDING | LVS_SHOWSELALWAYS);
	m_pList->SetExtendedListViewStyle(m_pList->GetExtendedListViewStyle() | LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);

	CreateListColumns();

	m_pNew		= new Framework::Win32::CButton(_T("New..."), m_hWnd, Framework::Win32::CRect(0, 0, 0, 0));
	m_pRename	= new Framework::Win32::CButton(_T("Rename..."), m_hWnd, Framework::Win32::CRect(0, 0, 0, 0));
	m_pDelete	= new Framework::Win32::CButton(_T("Delete"), m_hWnd, Framework::Win32::CRect(0, 0, 0, 0));
	m_pImport	= new Framework::Win32::CButton(_T("Load ELF symbols"), m_hWnd, Framework::Win32::CRect(0, 0, 0, 0));

	Framework::FlatLayoutPtr pSubLayout0 = Framework::CHorizontalLayout::Create();
	pSubLayout0->InsertObject(Framework::CLayoutStretch::Create());
	pSubLayout0->InsertObject(Framework::Win32::CLayoutWindow::CreateButtonBehavior(SCALE(100), SCALE(23), m_pNew));
	pSubLayout0->InsertObject(Framework::Win32::CLayoutWindow::CreateButtonBehavior(SCALE(100), SCALE(23), m_pRename));
	pSubLayout0->InsertObject(Framework::Win32::CLayoutWindow::CreateButtonBehavior(SCALE(100), SCALE(23), m_pDelete));
	pSubLayout0->InsertObject(Framework::Win32::CLayoutWindow::CreateButtonBehavior(SCALE(100), SCALE(23), m_pImport));

	m_pLayout = Framework::CVerticalLayout::Create();
	m_pLayout->InsertObject(Framework::Win32::CLayoutWindow::CreateCustomBehavior(1, 1, 1, 1, m_pList));
	m_pLayout->InsertObject(pSubLayout0);

	SetSize(SCALE(469), SCALE(612));

	RefreshLayout();
}

CFunctionsView::~CFunctionsView()
{

}

void CFunctionsView::Refresh()
{
	RefreshList();
}

long CFunctionsView::OnSize(unsigned int nX, unsigned int nY, unsigned int nType)
{
	RefreshLayout();
	return TRUE;
}

long CFunctionsView::OnNotify(WPARAM wParam, NMHDR* pH)
{
	if(pH->hwndFrom == m_pList->m_hWnd)
	{
		NMLISTVIEW* pN = reinterpret_cast<NMLISTVIEW*>(pH);
		switch(pN->hdr.code)
		{
		case NM_DBLCLK:
			OnListDblClick();
			break;
		}
	}

	return FALSE;
}

long CFunctionsView::OnCommand(unsigned short nID, unsigned short nCmd, HWND hSender)
{
	if(hSender == m_pNew->m_hWnd)
	{
		OnNewClick();
	}
	if(hSender == m_pRename->m_hWnd)
	{
		OnRenameClick();
	}
	if(hSender == m_pDelete->m_hWnd)
	{
		OnDeleteClick();
	}
	if(hSender == m_pImport->m_hWnd)
	{
		OnImportClick();
	}
	return TRUE;
}

long CFunctionsView::OnSysCommand(unsigned int nCmd, LPARAM lParam)
{
	switch(nCmd)
	{
	case SC_CLOSE:
		Show(SW_HIDE);
		return FALSE;
	}
	return TRUE;
}

void CFunctionsView::CreateListColumns()
{
	LVCOLUMN col;

	memset(&col, 0, sizeof(LVCOLUMN));
	col.pszText		= _T("Name");
	col.mask		= LVCF_TEXT;
	m_pList->InsertColumn(0, col);

	memset(&col, 0, sizeof(LVCOLUMN));
	col.pszText		= _T("Address");
	col.mask		= LVCF_TEXT;
	m_pList->InsertColumn(1, col);
}

void CFunctionsView::ResizeListColumns()
{
	RECT rc = m_pList->GetClientRect();

	m_pList->SetColumnWidth(0, rc.right * 2 / 3);
	m_pList->SetColumnWidth(1, rc.right * 1 / 3);
}

void CFunctionsView::RefreshLayout()
{
	if(m_pLayout == NULL) return;

	RECT rc = GetClientRect();

	SetRect(&rc, rc.left + 10, rc.top + 10, rc.right - 10, rc.bottom - 10);

	m_pLayout->SetRect(rc.left, rc.top, rc.right, rc.bottom);
	m_pLayout->RefreshGeometry();

	Redraw();

	ResizeListColumns();
}

void CFunctionsView::RefreshList()
{
	m_pList->SetRedraw(false);
	m_pList->DeleteAllItems();

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
	else
	{
		m_pList->EnableGroupView(false);
	}

	for(auto itTag(m_context->m_Functions.GetTagsBegin());
		itTag != m_context->m_Functions.GetTagsEnd(); itTag++)
	{
		LVITEM it;

		std::tstring sTag(string_cast<std::tstring>(itTag->second));

		memset(&it, 0, sizeof(LVITEM));
		it.pszText	= const_cast<LPTSTR>(sTag.c_str());
		it.lParam	= itTag->first;
		it.mask		= LVIF_PARAM | LVIF_TEXT;
		if(groupingEnabled)
		{
			it.iGroupId = GetFunctionGroupId(itTag->first);
			it.mask |= LVIF_GROUPID;
		}
		m_pList->InsertItem(it);
	}

	unsigned int nCount = m_pList->GetItemCount();

	for(unsigned int i = 0; i < nCount; i++)
	{
		uint32 nAddress = m_pList->GetItemData(i);
		m_pList->SetItemText(i, 1, (_T("0x") + lexical_cast_hex<std::tstring>(nAddress, 8)).c_str());
	}

	m_pList->SetRedraw(true);
}

void CFunctionsView::InitializeModuleGrouper()
{
	m_pList->RemoveAllGroups();
	m_pList->EnableGroupView(true);
	m_pList->InsertGroup(DEFAULT_GROUPNAME, DEFAULT_GROUPID);
	for(auto moduleIterator(std::begin(m_modules));
		std::end(m_modules) != moduleIterator; moduleIterator++)
	{
		const auto& module(*moduleIterator);
		m_pList->InsertGroup(
			string_cast<std::tstring>(module.name.c_str()).c_str(),
			module.begin);
	}
}

uint32 CFunctionsView::GetFunctionGroupId(uint32 address)
{
	for(auto moduleIterator(std::begin(m_modules));
		std::end(m_modules) != moduleIterator; moduleIterator++)
	{
		const auto& module(*moduleIterator);
		if(address >= module.begin && address < module.end) return module.begin;
	}
	return DEFAULT_GROUPID;
}

void CFunctionsView::SetContext(CMIPS* context, CBiosDebugInfoProvider* biosDebugInfoProvider)
{
	if(m_functionTagsChangeConnection.connected())
	{
		m_functionTagsChangeConnection.disconnect();
	}

	m_context = context;
	m_biosDebugInfoProvider = biosDebugInfoProvider;

	m_functionTagsChangeConnection = m_context->m_Functions.OnTagListChange.connect(
		boost::bind(&CFunctionsView::RefreshList, this));
	RefreshList();
}

void CFunctionsView::OnListDblClick()
{
	int nItem = m_pList->GetSelection();
	if(nItem == -1) return;
	uint32 nAddress = (uint32)m_pList->GetItemData(nItem);
	OnFunctionDblClick(nAddress);
}

void CFunctionsView::OnNewClick()
{
	if(!m_context) return;

	TCHAR sNameX[256];
	uint32 nAddress = 0;

	{
		Framework::Win32::CInputBox inputBox(_T("New Function"), _T("New Function Name:"), _T(""));
		const TCHAR* sValue = inputBox.GetValue(m_hWnd);
		if(sValue == NULL) return;
		_tcsncpy(sNameX, sValue, 255);	
	}

	{
		Framework::Win32::CInputBox inputBox(_T("New Function"), _T("New Function Address:"), _T("00000000"));
		const TCHAR* sValue = inputBox.GetValue(m_hWnd);
		if(sValue == NULL) return;
		if(sValue != NULL)
		{
			_stscanf(sValue, _T("%x"), &nAddress);
			if((nAddress & 0x3) != 0x0)
			{
				MessageBox(m_hWnd, _T("Invalid address."), NULL, 16);
				return;
			}
		}
	}

	m_context->m_Functions.InsertTag(nAddress, string_cast<std::string>(sNameX).c_str());

	RefreshList();
	OnFunctionsStateChange();
}

void CFunctionsView::OnRenameClick()
{
	if(!m_context) return;

	int nItem = m_pList->GetSelection();
	if(nItem == -1) return;
	
	uint32 nAddress = m_pList->GetItemData(nItem);
	const char* sName = m_context->m_Functions.Find(nAddress);

	if(sName == NULL)
	{
		//WTF?
		return;
	}

	Framework::Win32::CInputBox RenameInput(_T("Rename Function"), _T("New Function Name:"), string_cast<std::tstring>(sName).c_str());
	const TCHAR* sNewNameX = RenameInput.GetValue(m_hWnd);
	
	if(sNewNameX == NULL) return;

	m_context->m_Functions.InsertTag(nAddress, string_cast<std::string>(sNewNameX).c_str());
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

	printf("FunctionsView: Symbolic information found and loaded.\r\n");
}

void CFunctionsView::OnDeleteClick()
{
	if(!m_context) return;

	int nItem = m_pList->GetSelection();
	if(nItem == -1) return;
	if(MessageBox(m_hWnd, _T("Delete this function?"), NULL, MB_ICONQUESTION | MB_YESNO) != IDYES)
	{
		return;
	}

	uint32 nAddress = m_pList->GetItemData(nItem);
	m_context->m_Functions.InsertTag(nAddress, NULL);
	RefreshList();

	OnFunctionsStateChange();
}
