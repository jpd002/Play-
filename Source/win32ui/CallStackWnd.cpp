#include "CallStackWnd.h"
#include "PtrMacro.h"
#include "../MIPS.h"
#include "../PS2VM.h"

#define CLSNAME		_X("CallStackWnd")

using namespace Framework;

CCallStackWnd::CCallStackWnd(HWND hParent, CMIPS* pCtx)
{
	RECT rc;

	m_pCtx = pCtx;

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
	
	SetRect(&rc, 0, 0, 320, 240);

	Create(NULL, CLSNAME, _X("Call Stack"), WS_CLIPCHILDREN | WS_THICKFRAME | WS_CAPTION | WS_SYSMENU | WS_CHILD | WS_MAXIMIZEBOX, &rc, hParent, NULL);
	SetClassPtr();

	SetRect(&rc, 0, 0, 1, 1);

	m_pList = new CListView(m_hWnd, &rc, LVS_REPORT);
	m_pList->SetExtendedListViewStyle(m_pList->GetExtendedListViewStyle() | LVS_EX_FULLROWSELECT);

	m_pOnMachineStateChangeHandler = new CEventHandlerMethod<CCallStackWnd, int>(this, &CCallStackWnd::OnMachineStateChange);
	m_pOnRunningStateChangeHandler = new CEventHandlerMethod<CCallStackWnd, int>(this, &CCallStackWnd::OnRunningStateChange);

	CPS2VM::m_OnMachineStateChange.InsertHandler(m_pOnMachineStateChangeHandler);
	CPS2VM::m_OnRunningStateChange.InsertHandler(m_pOnRunningStateChangeHandler);

	CreateColumns();

	RefreshLayout();
	Update();
}

CCallStackWnd::~CCallStackWnd()
{
	DELETEPTR(m_pList);
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
	RECT rc;

	GetClientRect(&rc);

	if(m_pList != NULL)
	{
		m_pList->SetSize(rc.right, rc.bottom);
	}

	m_pList->GetClientRect(&rc);

	m_pList->SetColumnWidth(0, rc.right / 2);
	m_pList->SetColumnWidth(1, rc.right / 2);
}

void CCallStackWnd::CreateColumns()
{
	LVCOLUMN col;
	RECT rc;

	m_pList->GetClientRect(&rc);

	memset(&col, 0, sizeof(LVCOLUMN));
	col.pszText = _X("Function");
	col.mask	= LVCF_TEXT;
	m_pList->InsertColumn(0, &col);

	memset(&col, 0, sizeof(LVCOLUMN));
	col.pszText = _X("Caller");
	col.mask	= LVCF_TEXT;
	m_pList->InsertColumn(1, &col);
}

void CCallStackWnd::Update()
{
	uint32 nPC, nRA, nSP;
	MIPSSUBROUTINE* pRoutine;
	LVITEM item;
	unsigned int i;
	const char* sName;
	xchar sAddress[256];
	xchar sConvert[256];

	nPC = m_pCtx->m_State.nPC;
	nRA = m_pCtx->m_State.nGPR[CMIPS::RA].nV[0];
	nSP = m_pCtx->m_State.nGPR[CMIPS::SP].nV[0];

	m_pList->SetRedraw(false);

	m_pList->DeleteAllItems();

	pRoutine = m_pCtx->m_pAnalysis->FindSubroutine(nPC);
	if(pRoutine == NULL)
	{
		//Cannot go further
		memset(&item, 0, sizeof(LVITEM));
		item.pszText	= _X("Call stack unavailable at this state.");
		item.mask		= LVIF_TEXT | LVIF_PARAM;
		item.lParam		= MIPS_INVALID_PC;
		m_pList->InsertItem(&item);

		m_pList->SetRedraw(true);
		return;
	}

	//We need to get to a state where we're ready to dig into the previous function's
	//stack

	//Check if we need to check into the stack to get the RA
	if(m_pCtx->m_pAnalysis->FindSubroutine(nRA) == pRoutine)
	{
		nRA = m_pCtx->m_pMemoryMap->GetWord(nSP + pRoutine->nReturnAddrPos);
		nSP += pRoutine->nStackSize;
	}
	else
	{
		//We haven't called a sub routine yet... The RA is good, but we
		//don't know wether stack memory has been allocated or not
		
		//ADDIU SP, SP, 0x????
		//If the PC is after this instruction, then, we've allocated stack

		if(nPC > pRoutine->nStackAllocStart)
		{
			if(nPC <= pRoutine->nStackAllocEnd)
			{
				nSP += pRoutine->nStackSize;
			}
		}

	}

	while(1)
	{
		//Add the current function
		memset(&item, 0, sizeof(LVITEM));
		item.pszText	= _X("");
		item.iItem		= m_pList->GetItemCount();
		item.mask		= LVIF_TEXT | LVIF_PARAM;
		item.lParam		= pRoutine->nStart;
		i = m_pList->InsertItem(&item);

		sName = m_pCtx->m_Functions.Find(pRoutine->nStart);

		if(sName == NULL)
		{
			xsnprintf(sAddress, countof(sAddress), _X("0x%0.8X"), pRoutine->nStart);
		}
		else
		{
			xconvert(sConvert, sName, 256);
			xsnprintf(sAddress, countof(sAddress), _X("0x%0.8X (%s)"), pRoutine->nStart, sConvert);
		}
		
		m_pList->SetItemText(i, 0, sAddress);

		xsnprintf(sAddress, countof(sAddress), _X("0x%0.8X"), nRA - 4);
		m_pList->SetItemText(i, 1, sAddress);

		//Go to previous routine
		nPC = nRA - 4;

		//Check if we can go on...
		pRoutine = m_pCtx->m_pAnalysis->FindSubroutine(nPC);
		if(pRoutine == NULL) break;

		//Get the next RA
		nRA = m_pCtx->m_pMemoryMap->GetWord(nSP + pRoutine->nReturnAddrPos);
		nSP += pRoutine->nStackSize;
	}

	m_pList->SetRedraw(true);
}

void CCallStackWnd::OnListDblClick()
{
	int nSelection;
	uint32 nAddress;

	nSelection = m_pList->GetSelection();
	if(nSelection != -1)
	{
		nAddress = m_pList->GetItemData(nSelection);
		if(nAddress != MIPS_INVALID_PC)
		{
			m_OnFunctionDblClick.Notify(nAddress);
		}
	}
}

void CCallStackWnd::OnMachineStateChange(int nNothing)
{
	Update();
}

void CCallStackWnd::OnRunningStateChange(int nNothing)
{
	Update();
}
