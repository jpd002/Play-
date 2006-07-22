#include <boost/filesystem/operations.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include "SaveView.h"
#include "string_cast.h"
#include "win32/Static.h"

#define CLSNAME _X("CSaveView")

using namespace Framework;
using namespace boost;
using namespace std;

CSaveView::CSaveView(HWND hParent)
{
	RECT rc;

	m_pNormalIcon = NULL;
	m_pDeletingIcon = NULL;
	m_pCopyingIcon = NULL;

	if(!DoesWindowClassExist(CLSNAME))
	{
		WNDCLASSEX wc;
		memset(&wc, 0, sizeof(WNDCLASSEX));
		wc.cbSize			= sizeof(WNDCLASSEX);
		wc.hCursor			= LoadCursor(NULL, IDC_ARROW);
		wc.hbrBackground	= (HBRUSH)GetSysColorBrush(COLOR_BTNFACE); 
		wc.hInstance		= GetModuleHandle(NULL);
		wc.lpszClassName	= CLSNAME;
		wc.lpfnWndProc		= CWindow::WndProc;
		wc.style			= CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
		RegisterClassEx(&wc);
	}

	Create(NULL, CLSNAME, _X(""), WS_VISIBLE | WS_CLIPCHILDREN | WS_CHILD, &rc, hParent, NULL);
	SetClassPtr();

	GetClientRect(&rc);

	m_pNameLine1	= new Win32::CEdit(m_hWnd, &rc, _X(""), ES_READONLY, 0);
	m_pNameLine2	= new Win32::CEdit(m_hWnd, &rc, _X(""), ES_READONLY, 0);
	m_pSize			= new Win32::CEdit(m_hWnd, &rc, _X(""), ES_READONLY, 0);
	m_pId			= new Win32::CEdit(m_hWnd, &rc, _X(""), ES_READONLY, 0);
	m_pLastModified	= new Win32::CEdit(m_hWnd, &rc, _X(""), ES_READONLY, 0);
	m_pOpenFolder	= new Win32::CButton(_X("Open folder..."), m_hWnd, &rc);
	m_pExport		= new Win32::CButton(_X("Export..."), m_hWnd, &rc);
	m_pDelete		= new Win32::CButton(_X("Delete"), m_hWnd, &rc);
	m_pNormalIcon	= new Win32::CButton(_X("Normal Icon"), m_hWnd, &rc, BS_PUSHLIKE | BS_CHECKBOX);
	m_pCopyingIcon	= new Win32::CButton(_X("Copying Icon"), m_hWnd, &rc, BS_PUSHLIKE | BS_CHECKBOX);
	m_pDeletingIcon	= new Win32::CButton(_X("Deleting Icon"), m_hWnd, &rc, BS_PUSHLIKE | BS_CHECKBOX);
	m_pIconViewWnd	= new CIconViewWnd(m_hWnd, &rc);

	CHorizontalLayout* pSubLayout0;
	{
		pSubLayout0 = new CHorizontalLayout();
		pSubLayout0->InsertObject(CLayoutWindow::CreateTextBoxBehavior(300, 23, m_pId));
		pSubLayout0->InsertObject(CLayoutWindow::CreateButtonBehavior(100, 23, m_pOpenFolder));
	}

	CGridLayout* pSubLayout1;
	{
		pSubLayout1 = new CGridLayout(2, 5);

		pSubLayout1->SetObject(0, 0, CLayoutWindow::CreateTextBoxBehavior(100, 23, new Win32::CStatic(m_hWnd, _X("Name:"))));
		pSubLayout1->SetObject(0, 2, CLayoutWindow::CreateTextBoxBehavior(100, 23, new Win32::CStatic(m_hWnd, _X("Size:"))));
		pSubLayout1->SetObject(0, 3, CLayoutWindow::CreateTextBoxBehavior(100, 23, new Win32::CStatic(m_hWnd, _X("Id:"))));
		pSubLayout1->SetObject(0, 4, CLayoutWindow::CreateTextBoxBehavior(100, 23, new Win32::CStatic(m_hWnd, _X("Last Modified:")))); 

		pSubLayout1->SetObject(1, 0, CLayoutWindow::CreateTextBoxBehavior(300, 23, m_pNameLine1));
		pSubLayout1->SetObject(1, 1, CLayoutWindow::CreateTextBoxBehavior(300, 23, m_pNameLine2));
		pSubLayout1->SetObject(1, 2, CLayoutWindow::CreateTextBoxBehavior(300, 23, m_pSize));
		pSubLayout1->SetObject(1, 3, pSubLayout0);
		pSubLayout1->SetObject(1, 4, CLayoutWindow::CreateTextBoxBehavior(300, 23, m_pLastModified));

		pSubLayout1->SetVerticalStretch(0);
	}

	CHorizontalLayout* pSubLayout2;
	{
		pSubLayout2 = new CHorizontalLayout();
		pSubLayout2->InsertObject(CLayoutWindow::CreateButtonBehavior(100, 23, m_pExport));
		pSubLayout2->InsertObject(CLayoutWindow::CreateButtonBehavior(100, 23, m_pDelete));
		pSubLayout2->SetVerticalStretch(0);
	}

	CHorizontalLayout* pSubLayout3;
	{
		pSubLayout3 = new CHorizontalLayout();
		pSubLayout3->InsertObject(new CLayoutWindow(50, 50, 1, 1, m_pIconViewWnd));
		pSubLayout3->SetVerticalStretch(1);
	}

	CHorizontalLayout* pSubLayout4;
	{
		pSubLayout4 = new CHorizontalLayout();
		pSubLayout4->InsertObject(new CLayoutWindow(50, 23, 1, 0, m_pNormalIcon));
		pSubLayout4->InsertObject(new CLayoutWindow(50, 23, 1, 0, m_pCopyingIcon));
		pSubLayout4->InsertObject(new CLayoutWindow(50, 23, 1, 0, m_pDeletingIcon));
		pSubLayout4->SetVerticalStretch(0);
	}

	SetIconType(ICON_NORMAL);

	m_pLayout = new CVerticalLayout();
	m_pLayout->InsertObject(pSubLayout1);
	m_pLayout->InsertObject(pSubLayout2);
	m_pLayout->InsertObject(pSubLayout3);
	m_pLayout->InsertObject(pSubLayout4);

	RefreshLayout();
}

CSaveView::~CSaveView()
{
	delete m_pLayout;
}

void CSaveView::SetSave(const CSave* pSave)
{
	m_pSave = pSave;

	if(m_pSave != NULL)
	{
		wstring sName(m_pSave->GetName());

		m_pNameLine1->SetText(sName.substr(0, m_pSave->GetSecondLineStartPosition()).c_str());
		m_pNameLine2->SetText(sName.substr(m_pSave->GetSecondLineStartPosition()).c_str());
		m_pSize->SetText((lexical_cast<xstring>(m_pSave->GetSize()) + _X(" bytes")).c_str());
		m_pId->SetText(string_cast<xstring>(m_pSave->GetId()).c_str());
		m_pLastModified->SetText(
			string_cast<xstring>(
			posix_time::to_simple_string(
			posix_time::from_time_t(m_pSave->GetLastModificationTime()))).c_str());
	}
	else
	{
		m_pNameLine1->SetText(_X("--"));
		m_pNameLine2->SetText(_X(""));
		m_pSize->SetText(_X("--"));
		m_pId->SetText(_X("--"));
		m_pLastModified->SetText(_X("--"));
	}

	m_pIconViewWnd->SetSave(pSave);
	m_pOpenFolder->Enable(m_pSave != NULL);
	m_pNormalIcon->Enable(m_pSave != NULL);
	m_pDeletingIcon->Enable(m_pSave != NULL);
	m_pCopyingIcon->Enable(m_pSave != NULL);
}

long CSaveView::OnSize(unsigned int nX, unsigned int nY, unsigned int nType)
{
	RefreshLayout();
	return TRUE;
}

long CSaveView::OnCommand(unsigned short nCmd, unsigned short nId, HWND hWndFrom)
{
	if(m_pOpenFolder != NULL)
	{
		if(m_pOpenFolder->m_hWnd == hWndFrom) OpenSaveFolder();
	}
	if(m_pNormalIcon != NULL)
	{
		if(m_pNormalIcon->m_hWnd == hWndFrom) SetIconType(ICON_NORMAL);
	}
	if(m_pCopyingIcon != NULL)
	{
		if(m_pCopyingIcon->m_hWnd == hWndFrom) SetIconType(ICON_COPYING);
	}
	if(m_pDeletingIcon != NULL)
	{
		if(m_pDeletingIcon->m_hWnd == hWndFrom) SetIconType(ICON_DELETING);
	}
	return TRUE;
}

void CSaveView::RefreshLayout()
{
	RECT rc;

	GetClientRect(&rc);

	SetRect(&rc, rc.left + 10, rc.top + 10, rc.right - 10, rc.bottom - 10);

	m_pLayout->SetRect(rc.left, rc.top, rc.right, rc.bottom);
	m_pLayout->RefreshGeometry();

	Redraw();
}

void CSaveView::SetIconType(ICONTYPE nIconType)
{
	m_nIconType = nIconType;

	m_pIconViewWnd->SetIconType(nIconType);

	m_pNormalIcon->SetCheck(m_nIconType == ICON_NORMAL);
	m_pDeletingIcon->SetCheck(m_nIconType == ICON_DELETING);
	m_pCopyingIcon->SetCheck(m_nIconType == ICON_COPYING);
}

void CSaveView::OpenSaveFolder()
{
	if(m_pSave == NULL) return;

	filesystem::path Path(filesystem::complete(m_pSave->GetPath()));

	ShellExecuteA(m_hWnd, "open", Path.string().c_str(), NULL, NULL, SW_SHOW);
}
