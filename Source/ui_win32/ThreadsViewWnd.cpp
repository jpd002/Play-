#include <boost/lexical_cast.hpp>
#include "lexical_cast_ex.h"
#include "string_cast.h"
#include "ThreadsViewWnd.h"
#include "ThreadCallStackViewWnd.h"
#include "win32/Rect.h"
#include "win32/DefaultWndClass.h"
#include "win32/DpiUtils.h"
#include "../PS2VM.h"
#include "resource.h"
#include "DebugUtils.h"

#define WND_STYLE (WS_CLIPCHILDREN | WS_THICKFRAME | WS_CAPTION | WS_SYSMENU | WS_CHILD | WS_MAXIMIZEBOX)

CThreadsViewWnd::CThreadsViewWnd(HWND parentWnd, CVirtualMachine& virtualMachine)
: m_listView(nullptr)
, m_context(nullptr)
, m_biosDebugInfoProvider(nullptr)
{
	auto windowRect = Framework::Win32::PointsToPixels(Framework::Win32::CRect(0, 0, 700, 300));

	Create(NULL, Framework::Win32::CDefaultWndClass().GetName(), _T("Threads"), WND_STYLE, windowRect, parentWnd, NULL);
	SetClassPtr();

	m_listView = Framework::Win32::CListView(m_hWnd, Framework::Win32::CRect(0, 0, 1, 1), LVS_REPORT);
	m_listView.SetExtendedListViewStyle(m_listView.GetExtendedListViewStyle() | LVS_EX_FULLROWSELECT);

	CreateColumns();

	SetSize(windowRect.Width(), windowRect.Height());
	RefreshLayout();

	virtualMachine.OnMachineStateChange.connect(boost::bind(&CThreadsViewWnd::Update, this));
	virtualMachine.OnRunningStateChange.connect(boost::bind(&CThreadsViewWnd::Update, this));
}

CThreadsViewWnd::~CThreadsViewWnd()
{

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
	RECT rc = m_listView.GetClientRect();

	memset(&col, 0, sizeof(LVCOLUMN));
	col.pszText = _T("Id");
	col.mask	= LVCF_TEXT;
	m_listView.InsertColumn(0, col);

	memset(&col, 0, sizeof(LVCOLUMN));
	col.pszText = _T("Priority");
	col.mask	= LVCF_TEXT;
	m_listView.InsertColumn(1, col);

	memset(&col, 0, sizeof(LVCOLUMN));
	col.pszText = _T("Location");
	col.mask	= LVCF_TEXT;
	m_listView.InsertColumn(2, col);

	memset(&col, 0, sizeof(LVCOLUMN));
	col.pszText = _T("State");
	col.mask	= LVCF_TEXT;
	m_listView.InsertColumn(3, col);
}

void CThreadsViewWnd::RefreshLayout()
{
	if(m_listView.m_hWnd != NULL)
	{
		{
			auto rc = GetClientRect();
			m_listView.SetSizePosition(rc);
		}

		{
			auto rc = m_listView.GetClientRect();

			{
				unsigned int colSize = Framework::Win32::PointsToPixels(50);
				m_listView.SetColumnWidth(0, colSize);
				rc.SetRight(rc.Right() - colSize);
			}

			{
				unsigned int colSize = Framework::Win32::PointsToPixels(50);
				m_listView.SetColumnWidth(1, colSize);
				rc.SetRight(rc.Right() - colSize);
			}

			{
				unsigned int colSize = Framework::Win32::PointsToPixels(400);
				m_listView.SetColumnWidth(2, colSize);
				rc.SetRight(rc.Right() - colSize);
			}

			m_listView.SetColumnWidth(3, rc.Right());
		}
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
	if(CWindow::IsNotifySource(&m_listView, pHdr))
	{
		switch(pHdr->code)
		{
		case NM_DBLCLK:
			OnListDblClick();
			break;
		}
	}
	return FALSE;
}

void CThreadsViewWnd::Update()
{
	m_listView.DeleteAllItems();

	if(!m_biosDebugInfoProvider) return;

	auto threadInfos = m_biosDebugInfoProvider->GetThreadsDebugInfo();
	auto moduleInfos = m_biosDebugInfoProvider->GetModulesDebugInfo();

	for(auto threadInfoIterator(std::begin(threadInfos));
		threadInfoIterator != std::end(threadInfos); threadInfoIterator++)
	{
		const auto& threadInfo = *threadInfoIterator;

		LVITEM item;
		memset(&item, 0, sizeof(LVITEM));
		item.iItem		= m_listView.GetItemCount();
		item.lParam		= threadInfo.id;
		item.mask		= LVIF_PARAM;
		int itemIndex = m_listView.InsertItem(item);

		m_listView.SetItemText(itemIndex, 0, boost::lexical_cast<std::tstring>(threadInfo.id).c_str());
		m_listView.SetItemText(itemIndex, 1, boost::lexical_cast<std::tstring>(threadInfo.priority).c_str());

		std::tstring locationString = DebugUtils::PrintAddressLocation(threadInfo.pc, m_context, moduleInfos);
		m_listView.SetItemText(itemIndex, 2, locationString.c_str());

		m_listView.SetItemText(itemIndex, 3, string_cast<std::tstring>(threadInfo.stateDescription).c_str());
	}
}

void CThreadsViewWnd::OnListDblClick()
{
	int nSelection = m_listView.GetSelection();
	if(nSelection == -1) return;

	uint32 threadId = m_listView.GetItemData(nSelection);

	auto threadInfos = m_biosDebugInfoProvider->GetThreadsDebugInfo();

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
		threadCallStackViewWnd.SetItems(m_context, callStackItems, m_biosDebugInfoProvider->GetModulesDebugInfo());
		threadCallStackViewWnd.DoModal();

		if(threadCallStackViewWnd.HasSelection())
		{
			OnGotoAddress(threadCallStackViewWnd.GetSelectedAddress());
		}
	}
}
