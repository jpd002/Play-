#include "SpuRegViewPanel.h"
#include "win32/Rect.h"
#include "resource.h"

CSpuRegViewPanel::CSpuRegViewPanel(HWND parentWnd, const TCHAR* title)
: Framework::Win32::CDialog(MAKEINTRESOURCE(IDD_SPUREGVIEW), parentWnd)
{
	SetClassPtr();

	m_regView = new CSpuRegView(m_hWnd, title);

	RefreshLayout();
}

CSpuRegViewPanel::~CSpuRegViewPanel()
{
	delete m_regView;
}

void CSpuRegViewPanel::SetSpu(Iop::CSpuBase* spu)
{
	m_regView->SetSpu(spu);
}

long CSpuRegViewPanel::OnSize(unsigned int, unsigned int, unsigned int)
{
	RefreshLayout();
	return TRUE;
}

void CSpuRegViewPanel::RefreshLayout()
{
	Framework::Win32::CRect clientRect(GetClientRect());
	m_regView->SetSizePosition(clientRect);
}
