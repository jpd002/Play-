#include <boost/filesystem/operations.hpp>
#include <boost/lexical_cast.hpp>
#include "SaveView.h"
#include "win32/LayoutWindow.h"
#include "win32/Static.h"
#include "LayoutStretch.h"
#include "HorizontalLayout.h"

#define CLSNAME _X("CSaveView")

using namespace Framework;
using namespace boost;
using namespace std;

CSaveView::CSaveView(HWND hParent)
{
	RECT rc;

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
	m_pOpenFolder	= new Win32::CButton(_X("Open folder..."), m_hWnd, &rc);

	CHorizontalLayout* pSubLayout0;
	pSubLayout0 = new CHorizontalLayout();
	pSubLayout0->InsertObject(CLayoutWindow::CreateTextBoxBehavior(300, 20, m_pId));
	pSubLayout0->InsertObject(CLayoutWindow::CreateButtonBehavior(100, 20, m_pOpenFolder));

	m_pLayout = new CGridLayout(2, 5);

	m_pLayout->SetObject(0, 0, CLayoutWindow::CreateTextBoxBehavior(100, 20, new Win32::CStatic(m_hWnd, _X("Name:"))));
	m_pLayout->SetObject(0, 2, CLayoutWindow::CreateTextBoxBehavior(100, 20, new Win32::CStatic(m_hWnd, _X("Size:"))));
	m_pLayout->SetObject(0, 3, CLayoutWindow::CreateTextBoxBehavior(100, 20, new Win32::CStatic(m_hWnd, _X("Id:"))));
	m_pLayout->SetObject(0, 4, new CLayoutStretch());

	m_pLayout->SetObject(1, 0, CLayoutWindow::CreateTextBoxBehavior(300, 20, m_pNameLine1));
	m_pLayout->SetObject(1, 1, CLayoutWindow::CreateTextBoxBehavior(300, 20, m_pNameLine2));
	m_pLayout->SetObject(1, 2, CLayoutWindow::CreateTextBoxBehavior(300, 20, m_pSize));
	m_pLayout->SetObject(1, 3, pSubLayout0);
	m_pLayout->SetObject(1, 4, new CLayoutStretch());

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
		m_pId->SetText(lexical_cast<xstring>(m_pSave->GetId()).c_str());
	}
	else
	{
		m_pNameLine1->SetText(_X("--"));
		m_pNameLine2->SetText(_X(""));
		m_pSize->SetText(_X("--"));
		m_pId->SetText(_X("--"));
	}

	m_pOpenFolder->Enable(m_pSave != NULL);
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

void CSaveView::OpenSaveFolder()
{
	if(m_pSave == NULL) return;

	filesystem::path Path(filesystem::complete(m_pSave->GetPath()));

	ShellExecuteA(m_hWnd, "open", Path.string().c_str(), NULL, NULL, SW_SHOW);
}
