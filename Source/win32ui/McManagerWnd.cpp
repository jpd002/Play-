#include <boost/filesystem/operations.hpp>
#include <boost/bind.hpp>
#include <fstream>
#include "McManagerWnd.h"
#include "win32/Static.h"
#include "win32/FileDialog.h"
#include "../Config.h"

#define CLSNAME			_X("CMcManagerWnd")
#define WNDSTYLE		(WS_CAPTION | WS_POPUP | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_SYSMENU)
#define WNDSTYLEEX		(WS_EX_DLGMODALFRAME)

using namespace Framework;
using namespace boost;
using namespace std;

CMcManagerWnd::CMcManagerWnd(HWND hParent) :
CModalWindow(hParent),
m_MemoryCard0(filesystem::path(CConfig::GetInstance()->GetPreferenceString("ps2.mc0.directory"), filesystem::native)),
m_MemoryCard1(filesystem::path(CConfig::GetInstance()->GetPreferenceString("ps2.mc1.directory"), filesystem::native))
{
	RECT rc;

	m_pImportButton		= NULL;
	m_pCloseButton		= NULL;
	m_pMemoryCardList	= NULL;

	m_pMemoryCard[0] = &m_MemoryCard0;
	m_pMemoryCard[1] = &m_MemoryCard1;

	m_pCurrentMemoryCard = NULL;

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

	SetRect(&rc, 0, 0, 600, 500);
	
	Create(WNDSTYLEEX, CLSNAME, _X("Memory Card Manager"), WNDSTYLE, &rc, hParent, NULL);
	SetClassPtr();

	GetClientRect(&rc);

	m_pMemoryCardList	= new Win32::CComboBox(m_hWnd, &rc, CBS_DROPDOWNLIST | WS_VSCROLL);
	m_pImportButton		= new Win32::CButton(_X("Import Save(s)..."), m_hWnd, &rc);
	m_pCloseButton		= new Win32::CButton(_X("Close"), m_hWnd, &rc);
	m_pMemoryCardView	= new CMemoryCardView(m_hWnd, &rc);
	m_pSaveView			= new CSaveView(m_hWnd);

	m_pMemoryCardView->m_OnSelectionChange.InsertHandler(new CEventHandlerMethod<CSaveView, const CSave*>(m_pSaveView, &CSaveView::SetSave));

	m_pMemoryCardList->SetItemData(m_pMemoryCardList->AddString(_X("Memory Card Slot 0 (mc0)")), 0);
	m_pMemoryCardList->SetItemData(m_pMemoryCardList->AddString(_X("Memory Card Slot 1 (mc1)")), 1);
	m_pMemoryCardList->SetSelection(0);

	CHorizontalLayout* pSubLayout0;

	pSubLayout0 = new CHorizontalLayout();
	pSubLayout0->InsertObject(CLayoutWindow::CreateButtonBehavior(200, 23, m_pMemoryCardList));
	pSubLayout0->InsertObject(CLayoutWindow::CreateButtonBehavior(100, 23, m_pImportButton));
	pSubLayout0->InsertObject(new CLayoutStretch());

	CHorizontalLayout* pSubLayout1;

	pSubLayout1 = new CHorizontalLayout();
	pSubLayout1->InsertObject(CLayoutWindow::CreateButtonBehavior(130, 23, m_pMemoryCardView));
	pSubLayout1->InsertObject(new CLayoutWindow(100, 100, 1, 1, m_pSaveView));
	pSubLayout1->SetVerticalStretch(1);

	CHorizontalLayout* pSubLayout2;

	pSubLayout2 = new CHorizontalLayout();
	pSubLayout2->InsertObject(new CLayoutStretch());
	pSubLayout2->InsertObject(CLayoutWindow::CreateButtonBehavior(100, 23, m_pCloseButton));

	m_pLayout = new CVerticalLayout();
	m_pLayout->InsertObject(pSubLayout0);
	m_pLayout->InsertObject(CLayoutWindow::CreateButtonBehavior(200, 2, new Win32::CStatic(m_hWnd, &rc, SS_ETCHEDFRAME)));
	m_pLayout->InsertObject(pSubLayout1);
	m_pLayout->InsertObject(CLayoutWindow::CreateButtonBehavior(200, 2, new Win32::CStatic(m_hWnd, &rc, SS_ETCHEDFRAME)));
	m_pLayout->InsertObject(pSubLayout2);

	RefreshLayout();

	UpdateMemoryCardSelection();

	Center();
	Show(SW_SHOW);
}

CMcManagerWnd::~CMcManagerWnd()
{
	delete m_pLayout;
}

void CMcManagerWnd::RefreshLayout()
{
	RECT rc;

	GetClientRect(&rc);

	SetRect(&rc, rc.left + 10, rc.top + 10, rc.right - 10, rc.bottom - 10);

	m_pLayout->SetRect(rc.left, rc.top, rc.right, rc.bottom);
	m_pLayout->RefreshGeometry();

	m_pMemoryCardList->FixHeight(100);

	Redraw();
}

long CMcManagerWnd::OnDrawItem(unsigned int nId, LPDRAWITEMSTRUCT pDrawItem)
{
/*
	if(m_pMemoryCardView != NULL)
	{
		if(pDrawItem->hwndItem == (*m_pMemoryCardView)) return m_pMemoryCardView->OnDrawItem(nId, pDrawItem);
	}
*/
	return FALSE;
}

long CMcManagerWnd::OnCommand(unsigned short nId, unsigned short nCmd, HWND hWndFrom)
{
	if(m_pImportButton != NULL)
	{
		if(m_pImportButton->m_hWnd == hWndFrom)
		{
			Import();
		}
	}
	if(m_pCloseButton != NULL)
	{
		if(m_pCloseButton->m_hWnd == hWndFrom)
		{
			Destroy();
		}
	}
	if(m_pMemoryCardList != NULL)
	{
		if(m_pMemoryCardList->m_hWnd == hWndFrom)
		{
			switch(nCmd)
			{
			case CBN_SELCHANGE:
				UpdateMemoryCardSelection();
				break;
			}
		}
	}
	return TRUE;
}

void CMcManagerWnd::Import()
{
	unsigned int nRet;

	Win32::CFileDialog FileDialog;
	FileDialog.m_OFN.lpstrFilter = _X("All supported types\0*.psu;*.xps\0EMS Memory Adapter Save Dumps (*.psu)\0*.psu\0X-Port Save Dumps(*.xps)\0*.xps\0All files (*.*)\0*.*\0");

	Enable(FALSE);
	nRet = FileDialog.Summon(m_hWnd);
	Enable(TRUE);

	if(nRet == 0) return;

	ifstream Input(xfopen(FileDialog.m_sFile, _X("rb")));

	try
	{
		CSaveImporter::ImportSave(Input, m_pCurrentMemoryCard->GetBasePath(), bind(&CMcManagerWnd::OnImportOverwrite, this, _1));
	}
	catch(const exception& Exception)
	{
		char sMessage[256];
		sprintf(sMessage, "Couldn't import save(s):\r\n\r\n%s", Exception.what());
		MessageBoxA(m_hWnd, sMessage, NULL, 16);
		return;
	}

	m_pMemoryCardView->SetMemoryCard(NULL);
	m_pCurrentMemoryCard->RefreshContents();
	m_pMemoryCardView->SetMemoryCard(m_pCurrentMemoryCard);

	MessageBox(m_hWnd, _X("Save imported successfully."), NULL, MB_ICONINFORMATION); 
}

CSaveImporter::OVERWRITE_PROMPT_RETURN CMcManagerWnd::OnImportOverwrite(const string& sFilePath)
{
	string sMessage;
	int nReturn;
	sMessage = "File '" + sFilePath + "' already exists.\r\n\r\nOverwrite?";

	nReturn = MessageBoxA(m_hWnd, sMessage.c_str(), NULL, MB_YESNO | MB_ICONQUESTION);

	return (nReturn == IDYES) ? CSaveImporter::OVERWRITE_YES : CSaveImporter::OVERWRITE_NO;
}

void CMcManagerWnd::UpdateMemoryCardSelection()
{
	unsigned int nIndex;

	m_pMemoryCardView->SetMemoryCard(NULL);

	nIndex = m_pMemoryCardList->GetItemData(m_pMemoryCardList->GetSelection());
	m_pCurrentMemoryCard = m_pMemoryCard[nIndex];
	m_pMemoryCardView->SetMemoryCard(m_pCurrentMemoryCard);
}
