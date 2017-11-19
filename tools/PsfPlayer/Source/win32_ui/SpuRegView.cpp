#include "SpuRegView.h"
#include "win32/Rect.h"
#include <chrono>

#define WNDSTYLE						(WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_TABSTOP)
#define WNDSTYLEEX						(0)

CSpuRegView::CSpuRegView(HWND parentWnd, const RECT& rect, const TCHAR* title)
: m_spu(nullptr)
, m_title(title)
{
	Create(WNDSTYLEEX, Framework::Win32::CDefaultWndClass::GetName(), _T(""), WS_OVERLAPPED | WNDSTYLE, rect, parentWnd, this);

	int ydpi = GetDeviceCaps(GetDC(NULL), LOGPIXELSY);
	int lineHeight = MulDiv(20, ydpi, 96);

	m_listView = new Framework::Win32::CListView(m_hWnd, Framework::Win32::CRect(0, 0, 1, 1), WS_VSCROLL | LVS_REPORT);
	m_extrainfo = new Framework::Win32::CStatic(m_hWnd, Framework::Win32::CRect(0, 0, 1, 1));
	m_pLayout = Framework::CVerticalLayout::Create();

	m_pLayout->InsertObject(Framework::Win32::CLayoutWindow::CreateCustomBehavior(100, 100, 1, 1, m_listView));
	m_pLayout->InsertObject(Framework::Win32::CLayoutWindow::CreateTextBoxBehavior(100, lineHeight*2, m_extrainfo));
	CreateColumns();
	m_listView->SetExtendedListViewStyle(m_listView->GetExtendedListViewStyle() | LVS_EX_FULLROWSELECT);

	m_running = true;
	m_thread = new std::thread([&]() {
		while (m_running)
		{
			if (IsWindowVisible(m_hWnd))
			{
				std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now() + std::chrono::milliseconds(16);
				Refresh();
				std::this_thread::sleep_until(end);
			}
		}
	});
}

CSpuRegView::~CSpuRegView()
{
	m_running = false;
	if (m_thread->joinable()) m_thread->join();
	delete m_thread;
}

long CSpuRegView::OnSize(unsigned int type, unsigned int x, unsigned int y)
{
	if (m_pLayout != nullptr)
	{
		RECT rc = GetClientRect();
		SetSizePosition(rc);

		m_pLayout->SetRect(rc.left, rc.top, rc.right, rc.bottom);
		m_pLayout->RefreshGeometry();
		Redraw();

		return Framework::Win32::CWindow::OnSize(type, x, y);
	}

	return false;
}

void CSpuRegView::SetSpu(Iop::CSpuBase* spu)
{
	m_spu = spu;
}

void CSpuRegView::Refresh()
{
	if(m_spu != nullptr)
	{
		TCHAR channelStatus[Iop::CSpuBase::MAX_CHANNEL + 1];

		for(unsigned int i = 0; i < Iop::CSpuBase::MAX_CHANNEL; i++)
		{
			unsigned int index = m_listView->FindItemData(i);
			if(index == -1)
			{
				TCHAR temp[5];
				_stprintf(temp, _T("CH%0.2i"), i);

				LVITEM itm;
				memset(&itm, 0, sizeof(LVITEM));
				itm.mask		= LVIF_TEXT | LVIF_PARAM;
				itm.lParam		= i;
				itm.iItem		= i;
				itm.pszText		= const_cast<TCHAR*>(temp);

				index = m_listView->InsertItem(itm);
			}

			Iop::CSpuBase::CHANNEL& channel(m_spu->GetChannel(i));

			{
				TCHAR temp[5];
				_stprintf(temp, _T("%0.4X"), channel.volumeLeft);
				m_listView->SetItemText(index, 1, temp);
			}

			{
				TCHAR temp[5];
				_stprintf(temp, _T("%0.4X"), channel.volumeRight);
				m_listView->SetItemText(index, 2, temp);
			}

			{
				TCHAR temp[5];
				_stprintf(temp, _T("%0.4X"), channel.pitch);
				m_listView->SetItemText(index, 3, temp);
			}

			{
				TCHAR temp[7];
				_stprintf(temp, _T("%0.6X"), channel.address);
				m_listView->SetItemText(index, 4, temp);
			}

			{
				TCHAR temp[5];
				_stprintf(temp, _T("%0.4X"), channel.adsrLevel);
				m_listView->SetItemText(index, 5, temp);
			}

			{
				TCHAR temp[5];
				_stprintf(temp, _T("%0.4X"), channel.adsrRate);
				m_listView->SetItemText(index, 6, temp);
			}

			{
				TCHAR temp[9];
				_stprintf(temp, _T("%0.8X"), channel.adsrVolume);
				m_listView->SetItemText(index, 7, temp);
			}

			{
				TCHAR temp[7];
				_stprintf(temp, _T("%0.6X"), channel.repeat);
				m_listView->SetItemText(index, 8, temp);
			}


			TCHAR status = _T('0');
			switch(channel.status)
			{
			case Iop::CSpuBase::ATTACK:
			case Iop::CSpuBase::KEY_ON:
				status = _T('A');
				break;
			case Iop::CSpuBase::DECAY:
				status = _T('D');
				break;
			case Iop::CSpuBase::SUSTAIN:
				status = _T('S');
				break;
			case Iop::CSpuBase::RELEASE:
				status = _T('R');
				break;
			}

			channelStatus[i] = status;
		}


		channelStatus[Iop::CSpuBase::MAX_CHANNEL] = 0;

		{
			TCHAR revbStat[Iop::CSpuBase::MAX_CHANNEL + 1];

			uint32 stat = m_spu->GetChannelReverb().f;
			for(unsigned int i = 0; i < Iop::CSpuBase::MAX_CHANNEL; i++)
			{
				revbStat[i] = (stat & (1 << i)) ? _T('1') : _T('0');
			}
			revbStat[Iop::CSpuBase::MAX_CHANNEL] = 0;

			TCHAR temp[256];
			_stprintf(temp, _T("CH_STAT: %s\nCH_REVB: %s"), channelStatus, revbStat);
			m_extrainfo->SetText(temp);
		}

	}
}

void CSpuRegView::CreateColumns()
{
	LVCOLUMN col;
	RECT rc = GetClientRect();

	memset(&col, 0, sizeof(LVCOLUMN));
	col.pszText = const_cast<TCHAR*>(m_title.c_str());
	col.mask	= LVCF_TEXT;
	m_listView->InsertColumn(0, col);

	memset(&col, 0, sizeof(LVCOLUMN));
	col.pszText = _T("VLEFT");
	col.mask	= LVCF_TEXT;
	m_listView->InsertColumn(1, col);

	memset(&col, 0, sizeof(LVCOLUMN));
	col.pszText = _T("VRIGH");
	col.mask	= LVCF_TEXT;
	m_listView->InsertColumn(2, col);

	memset(&col, 0, sizeof(LVCOLUMN));
	col.pszText = _T("PITCH");
	col.mask	= LVCF_TEXT;
	m_listView->InsertColumn(3, col);

	memset(&col, 0, sizeof(LVCOLUMN));
	col.pszText = _T("ADDRE");
	col.mask	= LVCF_TEXT;
	m_listView->InsertColumn(4, col);

	memset(&col, 0, sizeof(LVCOLUMN));
	col.pszText = _T("ADSRL");
	col.mask	= LVCF_TEXT;
	m_listView->InsertColumn(5, col);

	memset(&col, 0, sizeof(LVCOLUMN));
	col.pszText = _T("ADSRR");
	col.mask	= LVCF_TEXT;
	m_listView->InsertColumn(6, col);

	memset(&col, 0, sizeof(LVCOLUMN));
	col.pszText = _T("ADSRVOLU");
	col.mask	= LVCF_TEXT;
	m_listView->InsertColumn(7, col);

	memset(&col, 0, sizeof(LVCOLUMN));
	col.pszText = _T("REPEA");
	col.mask	= LVCF_TEXT;
	m_listView->InsertColumn(8, col);

	m_listView->SetColumnWidth(0, rc.right / 10);
	m_listView->SetColumnWidth(1, rc.right / 10);
	m_listView->SetColumnWidth(2, rc.right / 10);

	m_listView->SetColumnWidth(3, rc.right / 10);
	m_listView->SetColumnWidth(4, rc.right / 8);
	m_listView->SetColumnWidth(5, rc.right / 10);
	m_listView->SetColumnWidth(6, rc.right / 10);
	m_listView->SetColumnWidth(7, rc.right / 7);
	m_listView->SetColumnWidth(8, rc.right / 8);

}
