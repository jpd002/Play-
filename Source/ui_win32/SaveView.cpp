#include <boost/filesystem/operations.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include "../saves/SaveExporter.h"
#include "../AppDef.h"
#include "SaveView.h"
#include "string_cast.h"
#include "win32/Static.h"
#include "win32/FileDialog.h"
#include "win32/Rect.h"
#include "StdStream.h"
#include "StdStreamUtils.h"

#define CLSNAME _T("CSaveView")

namespace filesystem = boost::filesystem;

CSaveView::CSaveView(HWND hParent)
{
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

	Create(NULL, CLSNAME, _T(""), WS_VISIBLE | WS_CLIPCHILDREN | WS_CHILD, Framework::Win32::CRect(0, 0, 10, 10), hParent, NULL);
	SetClassPtr();

	RECT rc = GetClientRect();

	m_pNameLine1	= new Framework::Win32::CEdit(m_hWnd, rc, _T(""), ES_READONLY, 0);
	m_pNameLine2	= new Framework::Win32::CEdit(m_hWnd, rc, _T(""), ES_READONLY, 0);
	m_pSize			= new Framework::Win32::CEdit(m_hWnd, rc, _T(""), ES_READONLY, 0);
	m_pId			= new Framework::Win32::CEdit(m_hWnd, rc, _T(""), ES_READONLY, 0);
	m_pLastModified	= new Framework::Win32::CEdit(m_hWnd, rc, _T(""), ES_READONLY, 0);
	m_pOpenFolder	= new Framework::Win32::CButton(_T("Open folder..."), m_hWnd, rc);
	m_pExport		= new Framework::Win32::CButton(_T("Export..."), m_hWnd, rc);
	m_pDelete		= new Framework::Win32::CButton(_T("Delete"), m_hWnd, rc);
	m_pNormalIcon	= new Framework::Win32::CButton(_T("Normal Icon"), m_hWnd, rc, BS_PUSHLIKE | BS_CHECKBOX);
	m_pCopyingIcon	= new Framework::Win32::CButton(_T("Copying Icon"), m_hWnd, rc, BS_PUSHLIKE | BS_CHECKBOX);
	m_pDeletingIcon	= new Framework::Win32::CButton(_T("Deleting Icon"), m_hWnd, rc, BS_PUSHLIKE | BS_CHECKBOX);
	m_pIconViewWnd	= new CSaveIconView(m_hWnd, rc);

	m_CommandSink.RegisterCallback(m_pOpenFolder->m_hWnd,	std::tr1::bind(&CSaveView::OpenSaveFolder, this));
	m_CommandSink.RegisterCallback(m_pNormalIcon->m_hWnd,	std::tr1::bind(&CSaveView::SetIconType, this, CSave::ICON_NORMAL));
	m_CommandSink.RegisterCallback(m_pCopyingIcon->m_hWnd,	std::tr1::bind(&CSaveView::SetIconType, this, CSave::ICON_COPYING));
	m_CommandSink.RegisterCallback(m_pDeletingIcon->m_hWnd,	std::tr1::bind(&CSaveView::SetIconType, this, CSave::ICON_DELETING));
	m_CommandSink.RegisterCallback(m_pExport->m_hWnd,		std::tr1::bind(&CSaveView::Export, this));
	m_CommandSink.RegisterCallback(m_pDelete->m_hWnd,		std::tr1::bind(&CSaveView::Delete, this));

	Framework::FlatLayoutPtr pSubLayout0 = Framework::CHorizontalLayout::Create();
	{
		pSubLayout0->InsertObject(Framework::Win32::CLayoutWindow::CreateTextBoxBehavior(300, 23, m_pId));
		pSubLayout0->InsertObject(Framework::Win32::CLayoutWindow::CreateButtonBehavior(100, 23, m_pOpenFolder));
	}

    Framework::GridLayoutPtr pSubLayout1 = Framework::CGridLayout::Create(2, 5);
	{
		pSubLayout1->SetObject(0, 0, Framework::Win32::CLayoutWindow::CreateTextBoxBehavior(100, 23, new Framework::Win32::CStatic(m_hWnd, _T("Name:"))));
		pSubLayout1->SetObject(0, 2, Framework::Win32::CLayoutWindow::CreateTextBoxBehavior(100, 23, new Framework::Win32::CStatic(m_hWnd, _T("Size:"))));
		pSubLayout1->SetObject(0, 3, Framework::Win32::CLayoutWindow::CreateTextBoxBehavior(100, 23, new Framework::Win32::CStatic(m_hWnd, _T("Id:"))));
		pSubLayout1->SetObject(0, 4, Framework::Win32::CLayoutWindow::CreateTextBoxBehavior(100, 23, new Framework::Win32::CStatic(m_hWnd, _T("Last Modified:")))); 

		pSubLayout1->SetObject(1, 0, Framework::Win32::CLayoutWindow::CreateTextBoxBehavior(300, 23, m_pNameLine1));
		pSubLayout1->SetObject(1, 1, Framework::Win32::CLayoutWindow::CreateTextBoxBehavior(300, 23, m_pNameLine2));
		pSubLayout1->SetObject(1, 2, Framework::Win32::CLayoutWindow::CreateTextBoxBehavior(300, 23, m_pSize));
		pSubLayout1->SetObject(1, 3, pSubLayout0);
		pSubLayout1->SetObject(1, 4, Framework::Win32::CLayoutWindow::CreateTextBoxBehavior(300, 23, m_pLastModified));

		pSubLayout1->SetVerticalStretch(0);
	}

    Framework::FlatLayoutPtr pSubLayout2 = Framework::CHorizontalLayout::Create();
	{
		pSubLayout2->InsertObject(Framework::Win32::CLayoutWindow::CreateButtonBehavior(100, 23, m_pExport));
		pSubLayout2->InsertObject(Framework::Win32::CLayoutWindow::CreateButtonBehavior(100, 23, m_pDelete));
		pSubLayout2->SetVerticalStretch(0);
	}

    Framework::FlatLayoutPtr pSubLayout3 = Framework::CHorizontalLayout::Create();
	{
        pSubLayout3->InsertObject(Framework::Win32::CLayoutWindow::CreateCustomBehavior(50, 50, 1, 1, m_pIconViewWnd));
		pSubLayout3->SetVerticalStretch(1);
	}

    Framework::FlatLayoutPtr pSubLayout4 = Framework::CHorizontalLayout::Create();
    {
        pSubLayout4->InsertObject(Framework::Win32::CLayoutWindow::CreateCustomBehavior(50, 23, 1, 0, m_pNormalIcon));
        pSubLayout4->InsertObject(Framework::Win32::CLayoutWindow::CreateCustomBehavior(50, 23, 1, 0, m_pCopyingIcon));
        pSubLayout4->InsertObject(Framework::Win32::CLayoutWindow::CreateCustomBehavior(50, 23, 1, 0, m_pDeletingIcon));
        pSubLayout4->SetVerticalStretch(0);
    }

	SetIconType(CSave::ICON_NORMAL);

    m_pLayout = Framework::CVerticalLayout::Create();
	m_pLayout->InsertObject(pSubLayout1);
	m_pLayout->InsertObject(pSubLayout2);
	m_pLayout->InsertObject(pSubLayout3);
	m_pLayout->InsertObject(pSubLayout4);

	RefreshLayout();
}

CSaveView::~CSaveView()
{

}

void CSaveView::SetSave(const CSave* pSave)
{
	m_pSave = pSave;

	if(m_pSave != NULL)
	{
		std::wstring sName(m_pSave->GetName());

		m_pNameLine1->SetText(sName.substr(0, m_pSave->GetSecondLineStartPosition()).c_str());
		m_pNameLine2->SetText(sName.substr(m_pSave->GetSecondLineStartPosition()).c_str());
		m_pSize->SetText((boost::lexical_cast<std::tstring>(m_pSave->GetSize()) + _T(" bytes")).c_str());
		m_pId->SetText(string_cast<std::tstring>(m_pSave->GetId()).c_str());
		m_pLastModified->SetText(
			string_cast<std::tstring>(
			boost::posix_time::to_simple_string(
			boost::posix_time::from_time_t(m_pSave->GetLastModificationTime()))).c_str());
	}
	else
	{
		m_pNameLine1->SetText(_T("--"));
		m_pNameLine2->SetText(_T(""));
		m_pSize->SetText(_T("--"));
		m_pId->SetText(_T("--"));
		m_pLastModified->SetText(_T("--"));
	}

	m_pIconViewWnd->SetSave(pSave);
	m_pOpenFolder->Enable(m_pSave != NULL);
	m_pNormalIcon->Enable(m_pSave != NULL);
	m_pDeletingIcon->Enable(m_pSave != NULL);
	m_pCopyingIcon->Enable(m_pSave != NULL);
	m_pExport->Enable(m_pSave != NULL);
	m_pDelete->Enable(m_pSave != NULL);
}

long CSaveView::OnSize(unsigned int nX, unsigned int nY, unsigned int nType)
{
	RefreshLayout();
	return TRUE;
}

long CSaveView::OnCommand(unsigned short nCmd, unsigned short nId, HWND hWndFrom)
{
	return m_CommandSink.OnCommand(nCmd, nId, hWndFrom);
}

void CSaveView::RefreshLayout()
{
	RECT rc = GetClientRect();

	SetRect(&rc, rc.left + 10, rc.top + 10, rc.right - 10, rc.bottom - 10);

	m_pLayout->SetRect(rc.left, rc.top, rc.right, rc.bottom);
	m_pLayout->RefreshGeometry();

	Redraw();
}

long CSaveView::SetIconType(CSave::ICONTYPE nIconType)
{
	m_nIconType = nIconType;

	m_pIconViewWnd->SetIconType(nIconType);

	m_pNormalIcon->SetCheck(m_nIconType == CSave::ICON_NORMAL);
	m_pDeletingIcon->SetCheck(m_nIconType == CSave::ICON_DELETING);
	m_pCopyingIcon->SetCheck(m_nIconType == CSave::ICON_COPYING);

	return FALSE;
}

long CSaveView::OpenSaveFolder()
{
	if(m_pSave == NULL) return FALSE;

	filesystem::path savePath(filesystem::absolute(m_pSave->GetPath()));
	ShellExecute(m_hWnd, _T("open"), savePath.native().c_str(), NULL, NULL, SW_SHOW);

	return FALSE;
}

long CSaveView::Export()
{
	if(m_pSave == NULL) return FALSE;

	Framework::Win32::CFileDialog FileDialog;
	FileDialog.m_OFN.lpstrFilter = _T("EMS Memory Adapter Save Dumps (*.psu)\0*.psu\0");

	EnableWindow(GetParent(), FALSE);
	unsigned int nRet = FileDialog.SummonSave(m_hWnd);
	EnableWindow(GetParent(), TRUE);

	if(nRet == 0) return FALSE;

	try
	{
		auto output(Framework::CreateOutputStdStream(std::tstring(FileDialog.GetPath())));
		CSaveExporter::ExportPSU(output, m_pSave->GetPath());
	}
	catch(const std::exception& Exception)
	{
		std::string sMessage;
		sMessage  = "Couldn't export save:\r\n\r\n";
		sMessage += Exception.what();
		MessageBoxA(m_hWnd, sMessage.c_str(), NULL, 16);
		return FALSE;
	}

	MessageBox(m_hWnd, _T("Save exported successfully."), APP_NAME, MB_ICONINFORMATION); 

	return FALSE;
}

long CSaveView::Delete()
{
	if(m_pSave == NULL) return FALSE;

	OnDeleteClick(m_pSave);

	return FALSE;
}
