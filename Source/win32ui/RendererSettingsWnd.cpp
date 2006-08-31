#include "PtrMacro.h"
#include "RendererSettingsWnd.h"
#include "HorizontalLayout.h"
#include "LayoutStretch.h"
#include "win32/LayoutWindow.h"
#include "win32/Static.h"
#include "../Config.h"

#define CLSNAME			_X("RendererSettingsWnd")
#define WNDSTYLE		(WS_CAPTION | WS_POPUP | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_SYSMENU)
#define WNDSTYLEEX		(WS_EX_DLGMODALFRAME)

using namespace Framework;

CRendererSettingsWnd::CRendererSettingsWnd(HWND hParent, CGSH_OpenGL* pRenderer) :
CModalWindow(hParent)
{
	RECT rc;
	CHorizontalLayout* pSubLayout0;

	m_pRenderer = pRenderer;

	if(!DoesWindowClassExist(CLSNAME))
	{
		WNDCLASSEX w;
		memset(&w, 0, sizeof(WNDCLASSEX));
		w.cbSize		= sizeof(WNDCLASSEX);
		w.lpfnWndProc	= CWindow::WndProc;
		w.lpszClassName	= CLSNAME;
		w.hbrBackground	= (HBRUSH)GetSysColorBrush(COLOR_BTNFACE);
		w.hInstance		= GetModuleHandle(NULL);
		w.hCursor		= LoadCursor(NULL, IDC_ARROW);
		w.style			= CS_HREDRAW | CS_VREDRAW;
		RegisterClassEx(&w);
	}

	SetRect(&rc, 0, 0, 400, 300);

	Create(WNDSTYLEEX, CLSNAME, _X("Renderer Settings"), WNDSTYLE, &rc, hParent, NULL);
	SetClassPtr();

	SetRect(&rc, 0, 0, 1, 1);

	m_pOk		= new Win32::CButton(_X("OK"), m_hWnd, &rc);
	m_pCancel	= new Win32::CButton(_X("Cancel"), m_hWnd, &rc);

	pSubLayout0 = new CHorizontalLayout;
	pSubLayout0->InsertObject(new CLayoutStretch);
	pSubLayout0->InsertObject(CLayoutWindow::CreateButtonBehavior(100, 23, m_pOk));
	pSubLayout0->InsertObject(CLayoutWindow::CreateButtonBehavior(100, 23, m_pCancel));
	pSubLayout0->SetVerticalStretch(0);

	m_nLinesAsQuads				= CConfig::GetInstance()->GetPreferenceBoolean(PREF_CGSH_OPENGL_LINEASQUADS);
	m_nForceBilinearTextures	= CConfig::GetInstance()->GetPreferenceBoolean(PREF_CGSH_OPENGL_FORCEBILINEARTEXTURES);
	m_nForceFlippingVSync		= CConfig::GetInstance()->GetPreferenceBoolean(PREF_CGSH_OPENGL_FORCEFLIPPINGVSYNC);

	m_pLineCheck = new Win32::CButton(_X("Render lines using quads"), m_hWnd, &rc, BS_CHECKBOX);
	m_pLineCheck->SetCheck(m_nLinesAsQuads);

	m_pForceBilinearCheck = new Win32::CButton(_X("Force bilinear texture sampling"), m_hWnd, &rc, BS_CHECKBOX);
	m_pForceBilinearCheck->SetCheck(m_nForceBilinearTextures);

	m_pForceFlippingCheck = new Win32::CButton(_T("Force buffer flipping at V-Sync"), m_hWnd, &rc, BS_CHECKBOX);
	m_pForceFlippingCheck->SetCheck(m_nForceFlippingVSync);

	m_pExtList = new CListView(m_hWnd, &rc, LVS_REPORT | LVS_SORTASCENDING | LVS_NOSORTHEADER);
	m_pExtList->SetExtendedListViewStyle(m_pExtList->GetExtendedListViewStyle() | LVS_EX_FULLROWSELECT);

	m_pLayout = new CVerticalLayout;
	m_pLayout->InsertObject(CLayoutWindow::CreateTextBoxBehavior(100, 15, m_pLineCheck));
	m_pLayout->InsertObject(CLayoutWindow::CreateTextBoxBehavior(100, 15, m_pForceBilinearCheck));
	m_pLayout->InsertObject(CLayoutWindow::CreateTextBoxBehavior(100, 15, m_pForceFlippingCheck));
	m_pLayout->InsertObject(CLayoutWindow::CreateTextBoxBehavior(100, 2, new Win32::CStatic(m_hWnd, &rc, SS_ETCHEDHORZ)));
	m_pLayout->InsertObject(CLayoutWindow::CreateTextBoxBehavior(100, 15, new Win32::CStatic(m_hWnd, _X("OpenGL extension availability report:"))));
	m_pLayout->InsertObject(new CLayoutWindow(1, 1, 1, 1, m_pExtList));
	m_pLayout->InsertObject(CLayoutWindow::CreateTextBoxBehavior(100, 30, new Win32::CStatic(m_hWnd, _X("For more information about the consequences of the absence of an extension, please consult the documentation."), SS_LEFT)));
	m_pLayout->InsertObject(pSubLayout0);

	RefreshLayout();

	CreateExtListColumns();
	UpdateExtList();
}

CRendererSettingsWnd::~CRendererSettingsWnd()
{
	DELETEPTR(m_pLayout);
}

long CRendererSettingsWnd::OnCommand(unsigned short nID, unsigned short nCmd, HWND hSender)
{
	if(hSender == m_pOk->m_hWnd)
	{
		Save();
		Destroy();
		return TRUE;
	}
	else if(hSender == m_pCancel->m_hWnd)
	{
		Destroy();
		return TRUE;
	}
	else if(ProcessCheckBoxMessage(hSender, m_pLineCheck, &m_nLinesAsQuads)) return TRUE;
	else if(ProcessCheckBoxMessage(hSender, m_pForceBilinearCheck, &m_nForceBilinearTextures)) return TRUE;
	else if(ProcessCheckBoxMessage(hSender, m_pForceFlippingCheck, &m_nForceFlippingVSync)) return TRUE;

	return FALSE;
}

void CRendererSettingsWnd::RefreshLayout()
{
	RECT rc;

	GetClientRect(&rc);

	SetRect(&rc, rc.left + 10, rc.top + 10, rc.right - 10, rc.bottom - 10);

	m_pLayout->SetRect(rc.left, rc.top, rc.right, rc.bottom);
	m_pLayout->RefreshGeometry();

	Redraw();
}

void CRendererSettingsWnd::CreateExtListColumns()
{
	LVCOLUMN col;
	RECT rc;

	m_pExtList->GetClientRect(&rc);

	memset(&col, 0, sizeof(LVCOLUMN));
	col.pszText = _X("Extension");
	col.mask	= LVCF_TEXT | LVCF_WIDTH;
	col.cx		= rc.right * 3 / 4;
	m_pExtList->InsertColumn(0, &col);

	memset(&col, 0, sizeof(LVCOLUMN));
	col.pszText = _X("Availability");
	col.mask	= LVCF_TEXT | LVCF_WIDTH;
	col.cx		= rc.right / 4;
	m_pExtList->InsertColumn(1, &col);
}

void CRendererSettingsWnd::UpdateExtList()
{
	unsigned int i;
	LVITEM itm;

	memset(&itm, 0, sizeof(LVITEM));
	itm.mask		= LVIF_TEXT;
	itm.pszText		= _X("glColorTable function");
	i = m_pExtList->InsertItem(&itm);

	m_pExtList->SetItemText(i, 1, m_pRenderer->IsColorTableExtSupported() ? _X("Present") : _X("Absent"));

	memset(&itm, 0, sizeof(LVITEM));
	itm.mask		= LVIF_TEXT;
	itm.pszText		= _X("glBlendColor function");
	i = m_pExtList->InsertItem(&itm);

	m_pExtList->SetItemText(i, 1, m_pRenderer->IsBlendColorExtSupported() ? _X("Present") : _X("Absent"));

	memset(&itm, 0, sizeof(LVITEM));
	itm.mask		= LVIF_TEXT;
	itm.pszText		= _X("GL_UNSIGNED_SHORT_1_5_5_5_REV texture format");
	i = m_pExtList->InsertItem(&itm);

	m_pExtList->SetItemText(i, 1, m_pRenderer->IsRGBA5551ExtSupported() ? _X("Present") : _X("Absent"));

	memset(&itm, 0, sizeof(LVITEM));
	itm.mask		= LVIF_TEXT;
	itm.pszText		= _X("glBlendEquation function");
	i = m_pExtList->InsertItem(&itm);

	m_pExtList->SetItemText(i, 1, m_pRenderer->IsBlendEquationExtSupported() ? _X("Present") : _X("Absent"));

	memset(&itm, 0, sizeof(LVITEM));
	itm.mask		= LVIF_TEXT;
	itm.pszText		= _X("glFogCoordf function");
	i = m_pExtList->InsertItem(&itm);

	m_pExtList->SetItemText(i, 1, m_pRenderer->IsFogCoordfExtSupported() ? _X("Present") : _X("Absent"));
}

void CRendererSettingsWnd::Save()
{
	CConfig::GetInstance()->SetPreferenceBoolean(PREF_CGSH_OPENGL_LINEASQUADS, m_nLinesAsQuads);
	CConfig::GetInstance()->SetPreferenceBoolean(PREF_CGSH_OPENGL_FORCEBILINEARTEXTURES, m_nForceBilinearTextures);
	CConfig::GetInstance()->SetPreferenceBoolean(PREF_CGSH_OPENGL_FORCEFLIPPINGVSYNC, m_nForceFlippingVSync);
}

bool CRendererSettingsWnd::ProcessCheckBoxMessage(HWND hSender, Win32::CButton* pCheckBox, bool* pFlag)
{
	if(pCheckBox == NULL) return false;
	if(pFlag == NULL) return false;
	if(hSender != pCheckBox->m_hWnd) return false;

	(*pFlag) = !(*pFlag);
	pCheckBox->SetCheck(*pFlag);

	return true;
}
