#include "FileInformationWindow.h"
#include "win32/Rect.h"
#include "win32/Static.h"
#include "layout/LayoutEngine.h"
#include "string_cast.h"

#define CLSNAME		_T("FileInformationWindow")
#define WNDSTYLE	(WS_CAPTION | WS_POPUP | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_SYSMENU)
#define WNDSTYLEEX	(WS_EX_DLGMODALFRAME)

#define LABEL_COLUMN_WIDTH	(50)

using namespace Framework;
using namespace std;

CFileInformationWindow::CFileInformationWindow(HWND parent, const CPsfTags& tags) :
CModalWindow(parent),
m_tags(tags)
{
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
		RegisterClassEx(&w);
	}

	Create(WNDSTYLEEX, CLSNAME, _T("File Information"), WNDSTYLE, Win32::CRect(0, 0, 400, 400), parent, NULL);
	SetClassPtr();

	m_title		= new Win32::CEdit(m_hWnd, Win32::CRect(0, 0, 0, 0), _T(""), ES_READONLY);
	m_artist	= new Win32::CEdit(m_hWnd, Win32::CRect(0, 0, 0, 0), _T(""), ES_READONLY);
	m_game		= new Win32::CEdit(m_hWnd, Win32::CRect(0, 0, 0, 0), _T(""), ES_READONLY);
	m_year		= new Win32::CEdit(m_hWnd, Win32::CRect(0, 0, 0, 0), _T(""), ES_READONLY);
	m_genre		= new Win32::CEdit(m_hWnd, Win32::CRect(0, 0, 0, 0), _T(""), ES_READONLY);
	m_comment	= new Win32::CEdit(m_hWnd, Win32::CRect(0, 0, 0, 0), _T(""), ES_READONLY);
	m_copyright	= new Win32::CEdit(m_hWnd, Win32::CRect(0, 0, 0, 0), _T(""), ES_READONLY);
	m_psfBy		= new Win32::CEdit(m_hWnd, Win32::CRect(0, 0, 0, 0), _T(""), ES_READONLY);
	m_rawTags	= new Win32::CEdit(m_hWnd, Win32::CRect(0, 0, 0, 0), _T(""), ES_READONLY | ES_MULTILINE | WS_VSCROLL);

	m_layout = 
		VerticalLayoutContainer
		(
			HorizontalLayoutContainer
			(
				LayoutExpression(Win32::CLayoutWindow::CreateButtonBehavior(LABEL_COLUMN_WIDTH, 12, new Win32::CStatic(m_hWnd, _T("Title:")))) +
				LayoutExpression(Win32::CLayoutWindow::CreateTextBoxBehavior(100, 20, m_title))
			) +
			HorizontalLayoutContainer
			(
				LayoutExpression(Win32::CLayoutWindow::CreateButtonBehavior(LABEL_COLUMN_WIDTH, 12, new Win32::CStatic(m_hWnd, _T("Artist:")))) +
				LayoutExpression(Win32::CLayoutWindow::CreateTextBoxBehavior(100, 20, m_artist))
			) +
			HorizontalLayoutContainer
			(
				LayoutExpression(Win32::CLayoutWindow::CreateButtonBehavior(LABEL_COLUMN_WIDTH, 12, new Win32::CStatic(m_hWnd, _T("Game:")))) +
				LayoutExpression(Win32::CLayoutWindow::CreateTextBoxBehavior(100, 20, m_game))
			) +
			HorizontalLayoutContainer
			(
				LayoutExpression(Win32::CLayoutWindow::CreateButtonBehavior(LABEL_COLUMN_WIDTH, 12, new Win32::CStatic(m_hWnd, _T("Year:")))) +
				LayoutExpression(Win32::CLayoutWindow::CreateTextBoxBehavior(100, 20, m_year))
			) +
			HorizontalLayoutContainer
			(
				LayoutExpression(Win32::CLayoutWindow::CreateButtonBehavior(LABEL_COLUMN_WIDTH, 12, new Win32::CStatic(m_hWnd, _T("Genre:")))) +
				LayoutExpression(Win32::CLayoutWindow::CreateTextBoxBehavior(100, 20, m_genre))
			) +
			HorizontalLayoutContainer
			(
				LayoutExpression(Win32::CLayoutWindow::CreateButtonBehavior(LABEL_COLUMN_WIDTH, 12, new Win32::CStatic(m_hWnd, _T("Comment:")))) +
				LayoutExpression(Win32::CLayoutWindow::CreateTextBoxBehavior(100, 20, m_comment))
			) +
			HorizontalLayoutContainer
			(
				LayoutExpression(Win32::CLayoutWindow::CreateButtonBehavior(LABEL_COLUMN_WIDTH, 12, new Win32::CStatic(m_hWnd, _T("Copyright:")))) +
				LayoutExpression(Win32::CLayoutWindow::CreateTextBoxBehavior(100, 20, m_copyright))
			) +
			HorizontalLayoutContainer
			(
				LayoutExpression(Win32::CLayoutWindow::CreateButtonBehavior(LABEL_COLUMN_WIDTH, 12, new Win32::CStatic(m_hWnd, _T("Psf by:")))) +
				LayoutExpression(Win32::CLayoutWindow::CreateTextBoxBehavior(100, 20, m_psfBy))
			) +
			LayoutExpression(Win32::CLayoutWindow::CreateTextBoxBehavior(100, 2, new Win32::CStatic(m_hWnd, Win32::CRect(0, 0, 0, 0), SS_ETCHEDVERT))) +
			LayoutExpression(Win32::CLayoutWindow::CreateTextBoxBehavior(100, 15, new Win32::CStatic(m_hWnd, _T("Raw Tags View:")))) +
			LayoutExpression(Win32::CLayoutWindow::CreateCustomBehavior(100, 20, 1, 1, m_rawTags))
		);

	RefreshLayout();
	UpdateFields();
}

CFileInformationWindow::~CFileInformationWindow()
{

}

void CFileInformationWindow::RefreshLayout()
{
	RECT rc = GetClientRect();
	m_layout->SetRect(rc.left + 10, rc.top + 10, rc.right - 10, rc.bottom - 10);
	m_layout->RefreshGeometry();
}

void CFileInformationWindow::UpdateFields()
{
	m_title->SetText(string_cast<tstring>(m_tags.GetTagValue("title")).c_str());
	m_artist->SetText(string_cast<tstring>(m_tags.GetTagValue("artist")).c_str());
	m_game->SetText(string_cast<tstring>(m_tags.GetTagValue("game")).c_str());
	m_year->SetText(string_cast<tstring>(m_tags.GetTagValue("year")).c_str());
	m_genre->SetText(string_cast<tstring>(m_tags.GetTagValue("genre")).c_str());
	m_comment->SetText(string_cast<tstring>(m_tags.GetTagValue("comment")).c_str());
	m_copyright->SetText(string_cast<tstring>(m_tags.GetTagValue("copyright")).c_str());
	m_psfBy->SetText(string_cast<tstring>(m_tags.GetTagValue("psfby")).c_str());

	tstring rawTagsString;
	for(CPsfTags::ConstTagIterator tagIterator(m_tags.GetTagsBegin());
		tagIterator != m_tags.GetTagsEnd(); tagIterator++)
	{
		rawTagsString += 
			string_cast<tstring>(tagIterator->first) + 
			_T("=") + 
			string_cast<tstring>(m_tags.DecodeTagValue(tagIterator->second.c_str())) + _T("\r\n");
	}
	m_rawTags->SetText(rawTagsString.c_str());
}
