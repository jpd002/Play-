#include <boost/lexical_cast.hpp>
#include "lexical_cast_ex.h"
#include "string_cast.h"
#include "ThreadsViewWnd.h"
#include "ThreadCallStackViewWnd.h"
#include "win32/Rect.h"
#include "../PS2VM.h"
#include "resource.h"
#include "DebugUtils.h"

#define CLSNAME		_T("ThreadsViewWnd")

CThreadsViewWnd::CThreadsViewWnd(HWND hParent, CVirtualMachine& virtualMachine)
: m_listView(nullptr)
, m_context(nullptr)
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

	Create(NULL, CLSNAME, _T("Threads"), WS_CLIPCHILDREN | WS_THICKFRAME | WS_CAPTION | WS_SYSMENU | WS_CHILD | WS_MAXIMIZEBOX, 
		Framework::Win32::CRect(0, 0, 320, 240), hParent, NULL);
	SetClassPtr();

	m_listView = new Framework::Win32::CListView(m_hWnd, Framework::Win32::CRect(0, 0, 1, 1), LVS_REPORT);
	m_listView->SetExtendedListViewStyle(m_listView->GetExtendedListViewStyle() | LVS_EX_FULLROWSELECT);

	CreateColumns();

	SetSize(700, 300);
	RefreshLayout();

	virtualMachine.OnMachineStateChange.connect(boost::bind(&CThreadsViewWnd::Update, this));
	virtualMachine.OnRunningStateChange.connect(boost::bind(&CThreadsViewWnd::Update, this));
}

CThreadsViewWnd::~CThreadsViewWnd()
{
	delete m_listView;
}

void CThreadsViewWnd::SetContext(CMIPS* context, CBiosDebugInfoProvider* biosDebugInfoProvider)
{
	m_context = context;
	m_biosDebugInfoProvider = biosDebugInfoProvider;

	Update();
}

void CThreadsViewWnd::CreateColumns()
{
	LVCOLUMN col;
	RECT rc;

	m_listView->GetClientRect(&rc);

	memset(&col, 0, sizeof(LVCOLUMN));
	col.pszText = _T("Id");
	col.mask	= LVCF_TEXT;
	m_listView->InsertColumn(0, &col);

	memset(&col, 0, sizeof(LVCOLUMN));
	col.pszText = _T("Priority");
	col.mask	= LVCF_TEXT;
	m_listView->InsertColumn(1, &col);

	memset(&col, 0, sizeof(LVCOLUMN));
	col.pszText = _T("Location");
	col.mask	= LVCF_TEXT;
	m_listView->InsertColumn(2, &col);

	memset(&col, 0, sizeof(LVCOLUMN));
	col.pszText = _T("State");
	col.mask	= LVCF_TEXT;
	m_listView->InsertColumn(3, &col);
}

void CThreadsViewWnd::RefreshLayout()
{
	if(m_listView != NULL)
	{
		RECT rc;

		GetClientRect(&rc);

		m_listView->SetPosition(0, 0);
		m_listView->SetSize(rc.right, rc.bottom);

		m_listView->GetClientRect(&rc);

		m_listView->SetColumnWidth(0, 50);
		rc.right -= 50;

		m_listView->SetColumnWidth(1, 50);
		rc.right -= 50;

		m_listView->SetColumnWidth(2, 400);
		rc.right -= 400;

		m_listView->SetColumnWidth(3, rc.right);
	}
}

long CThreadsViewWnd::OnSize(unsigned int nType, unsigned int nX, unsigned int nY)
{
	RefreshLayout();
	return TRUE;
}

long CThreadsViewWnd::OnSysCommand(unsigned int nCmd, LPARAM lParam)
{
	switch(nCmd)
	{
	case SC_CLOSE:
		Show(SW_HIDE);
		return FALSE;
	}
	return TRUE;
}

long CThreadsViewWnd::OnNotify(WPARAM wParam, NMHDR* pHdr)
{
	if(m_listView != NULL)
	{
		if(CWindow::IsNotifySource(m_listView, pHdr))
		{
			switch(pHdr->code)
			{
			case NM_DBLCLK:
				OnListDblClick();
				break;
			}
		}
	}
	return FALSE;
}

void CThreadsViewWnd::Update()
{
	m_listView->DeleteAllItems();

	if(!m_biosDebugInfoProvider) return;

	auto threadInfos = m_biosDebugInfoProvider->GetThreadInfos();
	auto moduleInfos = m_biosDebugInfoProvider->GetModuleInfos();

	for(auto threadInfoIterator(std::begin(threadInfos));
		threadInfoIterator != std::end(threadInfos); threadInfoIterator++)
	{
		const auto& threadInfo = *threadInfoIterator;

		LVITEM item;
		memset(&item, 0, sizeof(LVITEM));
		item.iItem		= m_listView->GetItemCount();
		item.lParam		= threadInfo.id;
		item.mask		= LVIF_PARAM;
		int itemIndex = m_listView->InsertItem(item);

		m_listView->SetItemText(itemIndex, 0, boost::lexical_cast<std::tstring>(threadInfo.id).c_str());
		m_listView->SetItemText(itemIndex, 1, boost::lexical_cast<std::tstring>(threadInfo.priority).c_str());

		std::tstring locationString = DebugUtils::PrintAddressLocation(threadInfo.pc, m_context, moduleInfos);
		m_listView->SetItemText(itemIndex, 2, locationString.c_str());

		m_listView->SetItemText(itemIndex, 3, string_cast<std::tstring>(threadInfo.stateDescription).c_str());
	}
}

INT_PTR WINAPI DialogProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	return FALSE;
}

void CThreadsViewWnd::OnListDblClick()
{
	int nSelection = m_listView->GetSelection();
	if(nSelection == -1) return;

	uint32 threadId = m_listView->GetItemData(nSelection);

	auto threadInfos = m_biosDebugInfoProvider->GetThreadInfos();

	auto threadInfoIterator = std::find_if(std::begin(threadInfos), std::end(threadInfos), 
		[&] (const BIOS_DEBUG_THREAD_INFO& threadInfo) { return threadInfo.id == threadId; });
	if(threadInfoIterator == std::end(threadInfos)) return;

	const auto& threadInfo(*threadInfoIterator);

	auto callStackItems = CMIPSAnalysis::GetCallStack(m_context, threadInfo.pc, threadInfo.sp, threadInfo.ra);
	if(callStackItems.size() <= 1)
	{
		OnGotoAddress(threadInfo.pc);
	}
	else
	{
		CThreadCallStackViewWnd threadCallStackViewWnd(m_hWnd);
		threadCallStackViewWnd.SetItems(m_context, callStackItems, m_biosDebugInfoProvider->GetModuleInfos());
		threadCallStackViewWnd.DoModal();

		if(threadCallStackViewWnd.HasSelection())
		{
			OnGotoAddress(threadCallStackViewWnd.GetSelectedAddress());
		}
	}
}
