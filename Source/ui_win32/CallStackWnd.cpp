#include "CallStackWnd.h"
#include "PtrMacro.h"
#include "string_cast.h"
#include "lexical_cast_ex.h"
#include "DebugUtils.h"
#include "../MIPS.h"

#define CLSNAME		_T("CallStackWnd")

CCallStackWnd::CCallStackWnd(HWND hParent, CVirtualMachine& virtualMachine, CMIPS* context, CBiosDebugInfoProvider* biosDebugInfoProvider)
: m_virtualMachine(virtualMachine)
, m_context(context)
, m_list(nullptr)
, m_biosDebugInfoProvider(biosDebugInfoProvider)
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
	
	Create(NULL, CLSNAME, _T("Call Stack"), WS_CLIPCHILDREN | WS_THICKFRAME | WS_CAPTION | WS_SYSMENU | WS_CHILD | WS_MAXIMIZEBOX, Framework::Win32::CRect(0, 0, 320, 240), hParent, NULL);
	SetClassPtr();

	m_list = new Framework::Win32::CListView(m_hWnd, Framework::Win32::CRect(0, 0, 1, 1), LVS_REPORT);
	m_list->SetExtendedListViewStyle(m_list->GetExtendedListViewStyle() | LVS_EX_FULLROWSELECT);

	m_virtualMachine.OnMachineStateChange.connect(boost::bind(&CCallStackWnd::Update, this));
	m_virtualMachine.OnRunningStateChange.connect(boost::bind(&CCallStackWnd::Update, this));

	CreateColumns();

	RefreshLayout();
	Update();
}

CCallStackWnd::~CCallStackWnd()
{
	delete m_list;
}

long CCallStackWnd::OnSize(unsigned int nType, unsigned int nX, unsigned int nY)
{
	RefreshLayout();
	return TRUE;
}

long CCallStackWnd::OnSysCommand(unsigned int nCmd, LPARAM lParam)
{
	switch(nCmd)
	{
	case SC_CLOSE:
		Show(SW_HIDE);
		return FALSE;
	}
	return TRUE;
}

long CCallStackWnd::OnNotify(WPARAM wParam, NMHDR* pHDR)
{
	switch(pHDR->code)
	{
	case NM_DBLCLK:
		OnListDblClick();
		return FALSE;
		break;
	}

	return FALSE;
}

void CCallStackWnd::RefreshLayout()
{
	if(m_list != NULL)
	{
		{
			RECT rc = GetClientRect();
			m_list->SetSize(rc.right, rc.bottom);
		}

		{
			RECT rc = m_list->GetClientRect();
			m_list->SetColumnWidth(0, rc.right);
		}
	}
}

void CCallStackWnd::CreateColumns()
{
	LVCOLUMN col;

	RECT rc = m_list->GetClientRect();

	memset(&col, 0, sizeof(LVCOLUMN));
	col.pszText = _T("Function");
	col.mask	= LVCF_TEXT;
	m_list->InsertColumn(0, col);
}

void CCallStackWnd::Update()
{
	uint32 nPC = m_context->m_State.nPC;
	uint32 nRA = m_context->m_State.nGPR[CMIPS::RA].nV[0];
	uint32 nSP = m_context->m_State.nGPR[CMIPS::SP].nV[0];

	m_list->SetRedraw(false);

	m_list->DeleteAllItems();

	auto callStackItems = CMIPSAnalysis::GetCallStack(m_context, nPC, nSP, nRA);

	if(callStackItems.size() == 0)
	{
		//Cannot go further
		LVITEM item;
		memset(&item, 0, sizeof(LVITEM));
		item.pszText	= _T("Call stack unavailable at this state.");
		item.mask		= LVIF_TEXT | LVIF_PARAM;
		item.lParam		= MIPS_INVALID_PC;
		m_list->InsertItem(item);

		m_list->SetRedraw(true);
		return;
	}

	auto modules = m_biosDebugInfoProvider ? m_biosDebugInfoProvider->GetModulesDebugInfo() : BiosDebugModuleInfoArray();

	for(auto itemIterator(std::begin(callStackItems));
		itemIterator != std::end(callStackItems); itemIterator++)
	{
		const auto& callStackItem(*itemIterator);

		//Add the current function
		LVITEM item;
		memset(&item, 0, sizeof(LVITEM));
		item.pszText	= _T("");
		item.iItem		= m_list->GetItemCount();
		item.mask		= LVIF_TEXT | LVIF_PARAM;
		item.lParam		= callStackItem;
		unsigned int i = m_list->InsertItem(item);

		std::tstring locationString = DebugUtils::PrintAddressLocation(callStackItem, m_context, modules);
		m_list->SetItemText(i, 0, locationString.c_str());
	}

	m_list->SetRedraw(true);
}

void CCallStackWnd::OnListDblClick()
{
	int nSelection = m_list->GetSelection();
	if(nSelection != -1)
	{
		uint32 nAddress = m_list->GetItemData(nSelection);
		if(nAddress != MIPS_INVALID_PC)
		{
			OnFunctionDblClick(nAddress);
		}
	}
}
