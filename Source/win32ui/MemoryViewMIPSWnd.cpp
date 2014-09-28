#include "MemoryViewMIPSWnd.h"
#include "PtrMacro.h"
#include "lexical_cast_ex.h"

#define CLSNAME		_T("MemoryViewMIPSWnd")
#define SCALE(x)	MulDiv(x, ydpi, 96)

CMemoryViewMIPSWnd::CMemoryViewMIPSWnd(HWND hParent, CVirtualMachine& virtualMachine, CMIPS* pCtx)
: m_memoryView(nullptr)
, m_addressEdit(nullptr)
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

	int ydpi = GetDeviceCaps(GetDC(NULL), LOGPIXELSY);

	Create(NULL, CLSNAME, _T("Memory"), WS_CLIPCHILDREN | WS_THICKFRAME | WS_CAPTION | WS_SYSMENU | WS_CHILD | WS_MAXIMIZEBOX, Framework::Win32::CRect(0, 0, SCALE(320), SCALE(240)), hParent, NULL);
	SetClassPtr();

	m_addressEdit = new Framework::Win32::CEdit(m_hWnd, Framework::Win32::CRect(0, 0, SCALE(320), SCALE(240)), _T(""), ES_READONLY);

	m_memoryView = new CMemoryViewMIPS(m_hWnd, Framework::Win32::CRect(0, 0, SCALE(320), SCALE(240)), virtualMachine, pCtx);
	m_memoryView->OnSelectionChange.connect(boost::bind(&CMemoryViewMIPSWnd::OnMemoryViewSelectionChange, this, _1));

	UpdateStatusBar();
	RefreshLayout();
}

CMemoryViewMIPSWnd::~CMemoryViewMIPSWnd()
{
	delete m_addressEdit;
	delete m_memoryView;
}

long CMemoryViewMIPSWnd::OnSize(unsigned int nType, unsigned int nCX, unsigned int nCY)
{
	RefreshLayout();
	return TRUE;
}

long CMemoryViewMIPSWnd::OnSetFocus()
{
	m_memoryView->SetFocus();
	return FALSE;
}

long CMemoryViewMIPSWnd::OnSysCommand(unsigned int nCmd, LPARAM lParam)
{
	switch(nCmd)
	{
	case SC_CLOSE:
		Show(SW_HIDE);
		return FALSE;
	}
	return TRUE;
}

void CMemoryViewMIPSWnd::UpdateStatusBar()
{
	std::tstring sCaption = _T("Address : 0x") + lexical_cast_hex<std::tstring>(m_memoryView->GetSelection(), 8);
	m_addressEdit->SetText(sCaption.c_str());
}

void CMemoryViewMIPSWnd::RefreshLayout()
{
	RECT rc = GetClientRect();

	int ydpi = GetDeviceCaps(GetDC(NULL), LOGPIXELSY);
	const int addressEditHeight = SCALE(21);

	m_addressEdit->SetSize(rc.right, addressEditHeight);
	m_memoryView->SetPosition(0, addressEditHeight);
	m_memoryView->SetSize(rc.right, rc.bottom - addressEditHeight);
}

void CMemoryViewMIPSWnd::OnMemoryViewSelectionChange(uint32 nAddress)
{
	UpdateStatusBar();
}
