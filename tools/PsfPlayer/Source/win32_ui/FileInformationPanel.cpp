#include "FileInformationPanel.h"
#include "win32/Rect.h"
#include "win32/Static.h"
#include "layout/LayoutEngine.h"
#include "string_cast.h"
#include "resource.h"

CFileInformationPanel::CFileInformationPanel(HWND parentWnd)
    : Framework::Win32::CDialog(MAKEINTRESOURCE(IDD_FILEINFORMATIONPANEL), parentWnd)
    , m_title(NULL)
    , m_artist(NULL)
    , m_game(NULL)
    , m_year(NULL)
    , m_genre(NULL)
    , m_comment(NULL)
    , m_copyright(NULL)
    , m_psfBy(NULL)
    , m_rawTags(NULL)
{
	SetClassPtr();

	m_title = new Framework::Win32::CEdit(GetItem(IDC_TITLE_EDIT));
	m_artist = new Framework::Win32::CEdit(GetItem(IDC_ARTIST_EDIT));
	m_game = new Framework::Win32::CEdit(GetItem(IDC_GAME_EDIT));
	m_year = new Framework::Win32::CEdit(GetItem(IDC_YEAR_EDIT));
	m_genre = new Framework::Win32::CEdit(GetItem(IDC_GENRE_EDIT));
	m_comment = new Framework::Win32::CEdit(GetItem(IDC_COMMENT_EDIT));
	m_copyright = new Framework::Win32::CEdit(GetItem(IDC_COPYRIGHT_EDIT));
	m_psfBy = new Framework::Win32::CEdit(GetItem(IDC_PSFBY_EDIT));
	m_rawTags = new Framework::Win32::CEdit(GetItem(IDC_RAWTAGS_EDIT));

	RECT columnEditBoxSize;
	SetRect(&columnEditBoxSize, 0, 0, 168, 12);
	MapDialogRect(m_hWnd, &columnEditBoxSize);
	unsigned int columnEditBoxWidth = columnEditBoxSize.right - columnEditBoxSize.left;
	unsigned int columnEditBoxHeight = columnEditBoxSize.bottom - columnEditBoxSize.top;

	RECT columnLabelSize;
	SetRect(&columnLabelSize, 0, 0, 70, 8);
	MapDialogRect(m_hWnd, &columnLabelSize);
	unsigned int columnLabelWidth = columnLabelSize.right - columnLabelSize.left;
	unsigned int columnLabelHeight = columnLabelSize.bottom - columnLabelSize.top;

	m_layout =
	    Framework::VerticalLayoutContainer(
	        Framework::HorizontalLayoutContainer(
	            Framework::LayoutExpression(Framework::Win32::CLayoutWindow::CreateButtonBehavior(columnLabelWidth, columnLabelHeight, new Framework::Win32::CStatic(GetItem(IDC_TITLE_LABEL)))) +
	            Framework::LayoutExpression(Framework::Win32::CLayoutWindow::CreateTextBoxBehavior(columnEditBoxWidth, columnEditBoxHeight, m_title))) +
	        Framework::HorizontalLayoutContainer(
	            Framework::LayoutExpression(Framework::Win32::CLayoutWindow::CreateButtonBehavior(columnLabelWidth, columnLabelHeight, new Framework::Win32::CStatic(GetItem(IDC_ARTIST_LABEL)))) +
	            Framework::LayoutExpression(Framework::Win32::CLayoutWindow::CreateTextBoxBehavior(columnEditBoxWidth, columnEditBoxHeight, m_artist))) +
	        Framework::HorizontalLayoutContainer(
	            Framework::LayoutExpression(Framework::Win32::CLayoutWindow::CreateButtonBehavior(columnLabelWidth, columnLabelHeight, new Framework::Win32::CStatic(GetItem(IDC_GAME_LABEL)))) +
	            Framework::LayoutExpression(Framework::Win32::CLayoutWindow::CreateTextBoxBehavior(columnEditBoxWidth, columnEditBoxHeight, m_game))) +
	        Framework::HorizontalLayoutContainer(
	            Framework::LayoutExpression(Framework::Win32::CLayoutWindow::CreateButtonBehavior(columnLabelWidth, columnLabelHeight, new Framework::Win32::CStatic(GetItem(IDC_YEAR_LABEL)))) +
	            Framework::LayoutExpression(Framework::Win32::CLayoutWindow::CreateTextBoxBehavior(columnEditBoxWidth, columnEditBoxHeight, m_year))) +
	        Framework::HorizontalLayoutContainer(
	            Framework::LayoutExpression(Framework::Win32::CLayoutWindow::CreateButtonBehavior(columnLabelWidth, columnLabelHeight, new Framework::Win32::CStatic(GetItem(IDC_GENRE_LABEL)))) +
	            Framework::LayoutExpression(Framework::Win32::CLayoutWindow::CreateTextBoxBehavior(columnEditBoxWidth, columnEditBoxHeight, m_genre))) +
	        Framework::HorizontalLayoutContainer(
	            Framework::LayoutExpression(Framework::Win32::CLayoutWindow::CreateButtonBehavior(columnLabelWidth, columnLabelHeight, new Framework::Win32::CStatic(GetItem(IDC_COMMENT_LABEL)))) +
	            Framework::LayoutExpression(Framework::Win32::CLayoutWindow::CreateTextBoxBehavior(columnEditBoxWidth, columnEditBoxHeight, m_comment))) +
	        Framework::HorizontalLayoutContainer(
	            Framework::LayoutExpression(Framework::Win32::CLayoutWindow::CreateButtonBehavior(columnLabelWidth, columnLabelHeight, new Framework::Win32::CStatic(GetItem(IDC_COPYRIGHT_LABEL)))) +
	            Framework::LayoutExpression(Framework::Win32::CLayoutWindow::CreateTextBoxBehavior(columnEditBoxWidth, columnEditBoxHeight, m_copyright))) +
	        Framework::HorizontalLayoutContainer(
	            Framework::LayoutExpression(Framework::Win32::CLayoutWindow::CreateButtonBehavior(columnLabelWidth, columnLabelHeight, new Framework::Win32::CStatic(GetItem(IDC_PSFBY_LABEL)))) +
	            Framework::LayoutExpression(Framework::Win32::CLayoutWindow::CreateTextBoxBehavior(columnEditBoxWidth, columnEditBoxHeight, m_psfBy))) +
	        Framework::LayoutExpression(Framework::Win32::CLayoutWindow::CreateTextBoxBehavior(100, 2, new Framework::Win32::CStatic(GetItem(IDC_VERTICAL_SEPARATOR)))) +
	        Framework::LayoutExpression(Framework::Win32::CLayoutWindow::CreateTextBoxBehavior(columnLabelWidth, columnLabelHeight, new Framework::Win32::CStatic(GetItem(IDC_RAWTAGS_LABEL)))) +
	        Framework::LayoutExpression(Framework::Win32::CLayoutWindow::CreateCustomBehavior(100, 20, 1, 1, m_rawTags)));

	RefreshLayout();
}

CFileInformationPanel::~CFileInformationPanel()
{
}

void CFileInformationPanel::SetTags(const CPsfTags& tags)
{
	m_tags = tags;
	UpdateFields();
}

void CFileInformationPanel::RefreshLayout()
{
	RECT rc = GetClientRect();
	m_layout->SetRect(rc.left + 10, rc.top + 10, rc.right - 10, rc.bottom - 10);
	m_layout->RefreshGeometry();
}

long CFileInformationPanel::OnSize(unsigned int, unsigned int, unsigned int)
{
	RefreshLayout();
	return TRUE;
}

void CFileInformationPanel::UpdateFields()
{
	m_title->SetText(string_cast<std::tstring>(m_tags.GetTagValue("title")).c_str());
	m_artist->SetText(string_cast<std::tstring>(m_tags.GetTagValue("artist")).c_str());
	m_game->SetText(string_cast<std::tstring>(m_tags.GetTagValue("game")).c_str());
	m_year->SetText(string_cast<std::tstring>(m_tags.GetTagValue("year")).c_str());
	m_genre->SetText(string_cast<std::tstring>(m_tags.GetTagValue("genre")).c_str());
	m_comment->SetText(string_cast<std::tstring>(m_tags.GetTagValue("comment")).c_str());
	m_copyright->SetText(string_cast<std::tstring>(m_tags.GetTagValue("copyright")).c_str());
	m_psfBy->SetText(string_cast<std::tstring>(m_tags.GetTagValue("psfby")).c_str());

	std::tstring rawTagsString;
	for(CPsfTags::ConstTagIterator tagIterator(m_tags.GetTagsBegin());
	    tagIterator != m_tags.GetTagsEnd(); tagIterator++)
	{
		rawTagsString +=
		    string_cast<std::tstring>(tagIterator->first) +
		    _T("=") +
		    string_cast<std::tstring>(m_tags.DecodeTagValue(tagIterator->second.c_str())) + _T("\r\n");
	}
	m_rawTags->SetText(rawTagsString.c_str());
}
