#include <boost/filesystem.hpp>
#include <functional>
#include "string_cast.h"
#include "win32/Static.h"
#include "win32/FileDialog.h"
#include "win32/Rect.h"
#include "WinUtils.h"
#include "placeholder_def.h"
#include "StdStreamUtils.h"
#include "McManagerWnd.h"
#include "../AppConfig.h"
#include "../AppDef.h"

#define CLSNAME			_T("CMcManagerWnd")
#define WNDSTYLE		(WS_CAPTION | WS_POPUP | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_SYSMENU)
#define WNDSTYLEEX		(WS_EX_DLGMODALFRAME)
#define SCALE(x)		MulDiv(x, ydpi, 96)

namespace filesystem = boost::filesystem;

CMcManagerWnd::CMcManagerWnd(HWND hParent) 
: CModalWindow(hParent)
, m_MemoryCard0(CAppConfig::GetInstance().GetPreferenceString("ps2.mc0.directory"))
, m_MemoryCard1(CAppConfig::GetInstance().GetPreferenceString("ps2.mc1.directory"))
{
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

	int ydpi = GetDeviceCaps(GetDC(NULL), LOGPIXELSY);

	Create(WNDSTYLEEX, CLSNAME, _T("Memory Card Manager"), WNDSTYLE, Framework::Win32::CRect(0, 0, SCALE(600), SCALE(500)), hParent, NULL);
	SetClassPtr();

	RECT rc = GetClientRect();

	m_pMemoryCardList	= new Framework::Win32::CComboBox(m_hWnd, rc, CBS_DROPDOWNLIST | WS_VSCROLL);
	m_pImportButton		= new Framework::Win32::CButton(_T("Import Save(s)..."), m_hWnd, rc);
	m_pCloseButton		= new Framework::Win32::CButton(_T("Close"), m_hWnd, rc);
	m_pMemoryCardView	= new CMemoryCardView(m_hWnd, rc);
	m_pSaveView			= new CSaveView(m_hWnd);

	m_pSaveView->OnDeleteClick.connect(bind(&CMcManagerWnd::Delete, this, PLACEHOLDER_1));
	m_pMemoryCardView->OnSelectionChange.connect(bind(&CSaveView::SetSave, m_pSaveView, PLACEHOLDER_1));

	m_pMemoryCardList->SetItemData(m_pMemoryCardList->AddString(_T("Memory Card Slot 0 (mc0)")), 0);
	m_pMemoryCardList->SetItemData(m_pMemoryCardList->AddString(_T("Memory Card Slot 1 (mc1)")), 1);
	m_pMemoryCardList->SetSelection(0);

	Framework::FlatLayoutPtr pSubLayout0 = Framework::CHorizontalLayout::Create();
	pSubLayout0->InsertObject(Framework::Win32::CLayoutWindow::CreateButtonBehavior(SCALE(200), SCALE(23), m_pMemoryCardList));
	pSubLayout0->InsertObject(Framework::Win32::CLayoutWindow::CreateButtonBehavior(SCALE(100), SCALE(23), m_pImportButton));
	pSubLayout0->InsertObject(Framework::CLayoutStretch::Create());

	Framework::FlatLayoutPtr pSubLayout1 = Framework::CHorizontalLayout::Create();
	pSubLayout1->InsertObject(Framework::Win32::CLayoutWindow::CreateButtonBehavior(SCALE(130), SCALE(23), m_pMemoryCardView));
	pSubLayout1->InsertObject(Framework::Win32::CLayoutWindow::CreateCustomBehavior(SCALE(100), SCALE(100), 1, 1, m_pSaveView));
	pSubLayout1->SetVerticalStretch(1);

	Framework::FlatLayoutPtr pSubLayout2 = Framework::CHorizontalLayout::Create();
	pSubLayout2->InsertObject(Framework::CLayoutStretch::Create());
	pSubLayout2->InsertObject(Framework::Win32::CLayoutWindow::CreateButtonBehavior(SCALE(100), SCALE(23), m_pCloseButton));

	m_pLayout = Framework::CVerticalLayout::Create();
	m_pLayout->InsertObject(pSubLayout0);
	m_pLayout->InsertObject(Framework::Win32::CLayoutWindow::CreateButtonBehavior(SCALE(200), 2, new Framework::Win32::CStatic(m_hWnd, rc, SS_ETCHEDFRAME)));
	m_pLayout->InsertObject(pSubLayout1);
	m_pLayout->InsertObject(Framework::Win32::CLayoutWindow::CreateButtonBehavior(SCALE(200), 2, new Framework::Win32::CStatic(m_hWnd, rc, SS_ETCHEDFRAME)));
	m_pLayout->InsertObject(pSubLayout2);

	RefreshLayout();

	UpdateMemoryCardSelection();

	Center();
	Show(SW_SHOW);
}

CMcManagerWnd::~CMcManagerWnd()
{

}

void CMcManagerWnd::RefreshLayout()
{
	RECT rc = GetClientRect();

	SetRect(&rc, rc.left + 10, rc.top + 10, rc.right - 10, rc.bottom - 10);

	m_pLayout->SetRect(rc.left, rc.top, rc.right, rc.bottom);
	m_pLayout->RefreshGeometry();

	m_pMemoryCardList->FixHeight(100);

	Redraw();
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
	Framework::Win32::CFileDialog FileDialog;
	FileDialog.m_OFN.lpstrFilter = 
		_T("All supported types\0*.psu;*.sps;*.xps;*.max\0")
		_T("EMS Memory Adapter Save Dumps (*.psu)\0*.psu\0")
		_T("Sharkport/X-Port Save Dumps (*.sps; *.xps)\0*.sps;*.xps\0")
		_T("Action Replay MAX Save Dumps (*.max)\0*.max\0")
		_T("All files (*.*)\0*.*\0");

	Enable(FALSE);
	unsigned int nRet = FileDialog.SummonOpen(m_hWnd);
	Enable(TRUE);

	if(nRet == 0) return;

	try
	{
		auto input(Framework::CreateInputStdStream(std::tstring(FileDialog.GetPath())));
		CSaveImporter::ImportSave(input, m_pCurrentMemoryCard->GetBasePath(), 
			std::bind(&CMcManagerWnd::OnImportOverwrite, this, PLACEHOLDER_1));
	}
	catch(const std::exception& Exception)
	{
		char sMessage[256];
		sprintf(sMessage, "Couldn't import save(s):\r\n\r\n%s", Exception.what());
		MessageBoxA(m_hWnd, sMessage, NULL, 16);
		return;
	}

	m_pMemoryCardView->SetMemoryCard(NULL);
	m_pCurrentMemoryCard->RefreshContents();
	m_pMemoryCardView->SetMemoryCard(m_pCurrentMemoryCard);

	MessageBox(m_hWnd, _T("Save imported successfully."), APP_NAME, MB_ICONINFORMATION); 
}

void CMcManagerWnd::Delete(const CSave* pSave)
{
	int nReturn = MessageBox(m_hWnd, _T("Are you sure you want to delete the currently selected entry?"), NULL, MB_YESNO | MB_ICONQUESTION);

	if(nReturn == IDNO) return;

	std::tstring sPath = string_cast<std::tstring>(filesystem::absolute(pSave->GetPath()).native());
	m_pMemoryCardView->SetMemoryCard(NULL);

	transform(sPath.begin(), sPath.end(), sPath.begin(), WinUtils::FixSlashes);

	//Construct the file list
	TCHAR* sFromList = (TCHAR*)_alloca((sPath.size() + 2) * sizeof(TCHAR));
	_tcscpy(sFromList, sPath.c_str());
	sFromList[sPath.size() + 1] = 0;

	//Construct the FILEOP structure
	SHFILEOPSTRUCT FileOp;
	memset(&FileOp, 0, sizeof(SHFILEOPSTRUCT));
	FileOp.hwnd		= m_hWnd;
	FileOp.wFunc	= FO_DELETE;
	FileOp.fFlags	= FOF_NOCONFIRMATION;
	FileOp.pFrom	= sFromList;
	SHFileOperation(&FileOp);

	m_pCurrentMemoryCard->RefreshContents();
	m_pMemoryCardView->SetMemoryCard(m_pCurrentMemoryCard);
}

CSaveImporterBase::OVERWRITE_PROMPT_RETURN CMcManagerWnd::OnImportOverwrite(const boost::filesystem::path& filePath)
{
	std::string fileName = filePath.leaf().string();
	std::string message = "File '" + fileName + "' already exists.\r\n\r\nOverwrite?";
	int result = MessageBoxA(m_hWnd, message.c_str(), NULL, MB_YESNO | MB_ICONQUESTION);
	return (result == IDYES) ? CSaveImporterBase::OVERWRITE_YES : CSaveImporterBase::OVERWRITE_NO;
}

void CMcManagerWnd::UpdateMemoryCardSelection()
{
	m_pMemoryCardView->SetMemoryCard(NULL);

	unsigned int nIndex = m_pMemoryCardList->GetItemData(m_pMemoryCardList->GetSelection());
	m_pCurrentMemoryCard = m_pMemoryCard[nIndex];
	m_pMemoryCardView->SetMemoryCard(m_pCurrentMemoryCard);
}
